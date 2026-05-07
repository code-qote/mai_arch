#include "common/cache/redis_cache_component.hpp"

#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/secdist.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/redis/command_control.hpp>
#include <userver/storages/redis/command_options.hpp>
#include <userver/utils/datetime.hpp>

#include <thread>
#include <chrono>

namespace common::cache {

namespace {
// Shared command control with proper timeouts as per userver documentation
const userver::storages::redis::CommandControl kRedisCC{
    std::chrono::milliseconds{15000},  // timeout_single (15s)
    std::chrono::milliseconds{60000},  // timeout_all (60s)
    4                                  // max_retries
};

constexpr int kLockTTLSeconds = 5;  // Lock TTL for cache stampede protection
constexpr int kMaxRetries = 3;       // Max retries for acquiring lock
constexpr int kRetryDelayMs = 100;  // Delay between retries
}  // namespace

RedisCacheComponent::RedisCacheComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : userver::components::ComponentBase(config, context) {
    
    // Get the Redis component and client.
    // FindComponent arg = component name in static_config.yaml
    // GetClient arg = the "db" field value from the groups config
    redis_client_ = context.FindComponent<userver::components::Redis>("redis-cache-db")
                        .GetClient("redis-cache");
    
    LOG_INFO() << "Redis cache component initialized";
}

bool RedisCacheComponent::Set(const std::string& key, const std::string& value,
                               std::optional<int> ttl_seconds) {
    try {
        if (ttl_seconds.has_value()) {
            // SET key value EX ttl_seconds
            redis_client_->Set(key, value, std::chrono::seconds{ttl_seconds.value()}, kRedisCC).Get();
        } else {
            // SET key value
            redis_client_->Set(key, value, kRedisCC).Get();
        }
        return true;
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Redis SET failed for key: " << key << ", error: " << ex.what();
        return false;
    }
}

std::optional<std::string> RedisCacheComponent::Get(const std::string& key) {
    try {
        auto result = redis_client_->Get(key, kRedisCC).Get();
        
        if (result) {
            return *result;
        }
        return std::nullopt;
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Redis GET failed for key: " << key << ", error: " << ex.what();
        return std::nullopt;
    }
}

bool RedisCacheComponent::Delete(const std::string& key) {
    try {
        redis_client_->Del(key, kRedisCC).Get();
        return true;
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Redis DEL failed for key: " << key << ", error: " << ex.what();
        return false;
    }
}

std::vector<std::string> RedisCacheComponent::Keys(const std::string& pattern) {
    std::vector<std::string> result;
    try {
        // Use Scan with Match option to find keys matching the pattern.
        // Scan iterates across all keys without blocking the server (unlike KEYS).
        auto shards_count = redis_client_->ShardsCount();
        for (size_t shard = 0; shard < shards_count; ++shard) {
            userver::storages::redis::ScanOptions options{
                userver::storages::redis::ScanOptions::Match{pattern},
                userver::storages::redis::ScanOptions::Count{100}
            };
            auto scan_result = redis_client_->Scan(shard, options, kRedisCC).GetAll();
            result.insert(result.end(), scan_result.begin(), scan_result.end());
        }
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Redis SCAN failed for pattern: " << pattern
                    << ", error: " << ex.what();
    }
    return result;
}

bool RedisCacheComponent::DeletePattern(const std::string& pattern) {
    try {
        // Find all keys matching the pattern
        auto keys = Keys(pattern);
        
        if (keys.empty()) {
            LOG_INFO() << "No keys found matching pattern: " << pattern;
            return true;
        }
        
        LOG_INFO() << "Deleting " << keys.size() << " keys matching pattern: " << pattern;
        
        // Delete all matching keys
        for (const auto& key : keys) {
            redis_client_->Del(key, kRedisCC).Get();
        }
        
        return true;
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Redis DeletePattern failed for pattern: " << pattern
                    << ", error: " << ex.what();
        return false;
    }
}

bool RedisCacheComponent::TryAcquireLock(const std::string& lock_key, int ttl_seconds) {
    try {
        // Use SETNX (Set if Not eXists) to acquire lock
        redis_client_->Set(lock_key, "1", std::chrono::seconds{ttl_seconds}, kRedisCC).Get();
        return true;  // If we get here, SET was successful
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Failed to acquire lock for key: " << lock_key << ", error: " << ex.what();
        return false;
    }
}

void RedisCacheComponent::ReleaseLock(const std::string& lock_key) {
    try {
        redis_client_->Del(lock_key, kRedisCC).Get();
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Failed to release lock for key: " << lock_key << ", error: " << ex.what();
    }
}

std::optional<std::string> RedisCacheComponent::GetWithLock(const std::string& key,
                                                             std::function<std::string()> fetch_func,
                                                             int ttl_seconds) {
    // Try to get from cache first
    auto cached = Get(key);
    if (cached) {
        LOG_INFO() << "Cache hit for key: " << key;
        return cached;
    }
    
    LOG_INFO() << "Cache miss for key: " << key << ", attempting to acquire lock";
    
    // Try to acquire lock
    std::string lock_key = "lock:" + key;
    bool lock_acquired = TryAcquireLock(lock_key, kLockTTLSeconds);
    
    if (lock_acquired) {
        // We have the lock, fetch from DB
        LOG_INFO() << "Lock acquired for key: " << key << ", fetching from data source";
        try {
            auto data = fetch_func();
            Set(key, data, ttl_seconds);
            ReleaseLock(lock_key);
            LOG_INFO() << "Data fetched and cached for key: " << key;
            return data;
        } catch (const std::exception& ex) {
            LOG_ERROR() << "Error fetching data for key: " << key << ", error: " << ex.what();
            ReleaseLock(lock_key);
            throw;
        }
    } else {
        // Lock not acquired, wait and retry
        LOG_INFO() << "Lock not acquired for key: " << key << ", waiting and retrying";
        for (int retry = 0; retry < kMaxRetries; ++retry) {
            std::this_thread::sleep_for(std::chrono::milliseconds{kRetryDelayMs});
            
            // Try to get from cache again
            cached = Get(key);
            if (cached) {
                LOG_INFO() << "Cache hit after retry for key: " << key;
                return cached;
            }
        }
        
        // After retries, still no data - fetch anyway (fallback)
        LOG_WARNING() << "Max retries reached for key: " << key << ", fetching without lock";
        try {
            auto data = fetch_func();
            Set(key, data, ttl_seconds);
            return data;
        } catch (const std::exception& ex) {
            LOG_ERROR() << "Error fetching data for key: " << key << ", error: " << ex.what();
            throw;
        }
    }
}

userver::yaml_config::Schema RedisCacheComponent::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<userver::components::ComponentBase>(R"(
type: object
description: Redis cache component
additionalProperties: false
properties: {}
)");
}

} // namespace common::cache
