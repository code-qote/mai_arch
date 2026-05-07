#pragma once

#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/component.hpp>

#include <string>
#include <optional>
#include <vector>
#include <functional>

namespace common::cache {

class RedisCacheComponent : public userver::components::ComponentBase {
public:
    static constexpr const char* kName = "redis-cache";

    RedisCacheComponent(const userver::components::ComponentConfig& config,
                        const userver::components::ComponentContext& context);

    ~RedisCacheComponent() override = default;

    // Set a value with optional TTL (in seconds)
    bool Set(const std::string& key, const std::string& value,
             std::optional<int> ttl_seconds = std::nullopt);
    
    // Get a value
    std::optional<std::string> Get(const std::string& key);
    
    // Get a value with cache stampede protection
    // If cache miss, only one thread will fetch the data while others wait
    std::optional<std::string> GetWithLock(const std::string& key,
                                           std::function<std::string()> fetch_func,
                                           int ttl_seconds);
    
    // Delete a key
    bool Delete(const std::string& key);
    
    // Delete multiple keys by pattern
    bool DeletePattern(const std::string& pattern);
    
    // Get keys matching a pattern
    std::vector<std::string> Keys(const std::string& pattern);

    static userver::yaml_config::Schema GetStaticConfigSchema();

private:
    userver::storages::redis::ClientPtr redis_client_;
    
    // Try to acquire a distributed lock
    bool TryAcquireLock(const std::string& lock_key, int ttl_seconds);
    
    // Release a distributed lock
    void ReleaseLock(const std::string& lock_key);
};

} // namespace common::cache
