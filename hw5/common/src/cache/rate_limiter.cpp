#include "common/cache/rate_limiter.hpp"

#include <userver/storages/redis/command_control.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/datetime.hpp>

#include <chrono>

namespace {

const userver::storages::redis::CommandControl kRedisCC{
    std::chrono::milliseconds{15000},  // timeout_single (15s)
    std::chrono::milliseconds{60000},  // timeout_all (60s)
    4                                  // max_retries
};

}  // namespace

namespace common::cache {

RateLimiterComponent::RateLimiterComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : userver::components::ComponentBase(config, context),
      redis_client_(context.FindComponent<userver::components::Redis>("redis-cache-db").GetClient("redis-cache")) {
    LOG_INFO() << "RateLimiterComponent initialized with sliding window algorithm";
}

int64_t RateLimiterComponent::GetCurrentTimestampMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        userver::utils::datetime::Now().time_since_epoch()
    ).count();
}

bool RateLimiterComponent::ConsumeToken(const std::string& key, int max_tokens, int window_seconds) {
    try {
        auto now = std::chrono::duration_cast<std::chrono::seconds>(
            userver::utils::datetime::Now().time_since_epoch()
        ).count();
        
        // Use multiple smaller windows to approximate sliding window behavior
        // This provides better accuracy than a single fixed window
        const int num_sub_windows = 10;
        const int sub_window_size = window_seconds / num_sub_windows;
        
        int total_count = 0;
        
        // Check all sub-windows within the main window
        for (int i = 0; i < num_sub_windows; ++i) {
            int sub_window_index = (now / sub_window_size) - i;
            std::string sub_window_key = key + ":" + std::to_string(sub_window_index);
            
            auto count_result = redis_client_->Get(sub_window_key, kRedisCC).Get();
            if (count_result) {
                total_count += std::stoi(*count_result);
            }
        }
        
        // Check if limit exceeded
        if (total_count >= max_tokens) {
            return false;
        }
        
        // Add to current sub-window
        int current_sub_window_index = now / sub_window_size;
        std::string current_key = key + ":" + std::to_string(current_sub_window_index);
        
        auto count_result = redis_client_->Incr(current_key, kRedisCC).Get();
        
        // Set expiration on first request in sub-window
        if (count_result && count_result == 1) {
            redis_client_->Expire(current_key, std::chrono::seconds{window_seconds + sub_window_size}, kRedisCC).Get();
        }
        
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR() << "Error in ConsumeToken: " << e.what();
        return false;
    }
}

RateLimitInfo RateLimiterComponent::IsAllowed(const std::string& user_id,
                                               const std::string& endpoint,
                                               int max_requests,
                                               int window_seconds) {
    std::string key = "rate_limit:" + user_id + ":" + endpoint;
    
    try {
        auto now = std::chrono::duration_cast<std::chrono::seconds>(
            userver::utils::datetime::Now().time_since_epoch()
        ).count();
        
        // Use multiple smaller windows to approximate sliding window behavior
        const int num_sub_windows = 10;
        const int sub_window_size = window_seconds / num_sub_windows;
        
        int total_count = 0;
        
        // Check all sub-windows within the main window
        for (int i = 0; i < num_sub_windows; ++i) {
            int sub_window_index = (now / sub_window_size) - i;
            std::string sub_window_key = key + ":" + std::to_string(sub_window_index);
            
            auto count_result = redis_client_->Get(sub_window_key, kRedisCC).Get();
            if (count_result) {
                total_count += std::stoi(*count_result);
            }
        }
        
        RateLimitInfo info;
        info.allowed = total_count < max_requests;
        info.remaining = std::max(0, max_requests - total_count);
        info.reset_time = ((now / sub_window_size) + 1) * sub_window_size;
        
        if (info.allowed) {
            // Add to current sub-window
            int current_sub_window_index = now / sub_window_size;
            std::string current_key = key + ":" + std::to_string(current_sub_window_index);
            
            auto count_result = redis_client_->Incr(current_key, kRedisCC).Get();
            
            // Set expiration on first request in sub-window
            if (count_result && count_result == 1) {
                redis_client_->Expire(current_key, std::chrono::seconds{window_seconds + sub_window_size}, kRedisCC).Get();
            }
        }
        
        return info;
        
    } catch (const std::exception& e) {
        LOG_ERROR() << "Error in IsAllowed: " << e.what();
        // Fail-closed: reject request on error
        auto now = std::chrono::duration_cast<std::chrono::seconds>(
            userver::utils::datetime::Now().time_since_epoch()
        ).count();
        return RateLimitInfo{false, 0, now + window_seconds};
    }
}

int RateLimiterComponent::GetRemainingRequests(const std::string& user_id,
                                                const std::string& endpoint,
                                                int max_requests,
                                                int window_seconds) {
    try {
        std::string key = "rate_limit:" + user_id + ":" + endpoint;
        auto now = std::chrono::duration_cast<std::chrono::seconds>(
            userver::utils::datetime::Now().time_since_epoch()
        ).count();
        
        // Use multiple smaller windows to approximate sliding window behavior
        const int num_sub_windows = 10;
        const int sub_window_size = window_seconds / num_sub_windows;
        
        int total_count = 0;
        
        // Check all sub-windows within the main window
        for (int i = 0; i < num_sub_windows; ++i) {
            int sub_window_index = (now / sub_window_size) - i;
            std::string sub_window_key = key + ":" + std::to_string(sub_window_index);
            
            auto count_result = redis_client_->Get(sub_window_key, kRedisCC).Get();
            if (count_result) {
                total_count += std::stoi(*count_result);
            }
        }
        
        return std::max(0, max_requests - total_count);
        
    } catch (const std::exception& e) {
        LOG_ERROR() << "Error in GetRemainingRequests: " << e.what();
        return max_requests;
    }
}

}  // namespace common::cache
