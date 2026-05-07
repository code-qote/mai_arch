#pragma once

#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/component.hpp>

#include <string>
#include <optional>
#include <chrono>

namespace common::cache {

struct RateLimitInfo {
    bool allowed;
    int remaining;
    int64_t reset_time;  // Unix timestamp when window resets
};

class RateLimiterComponent : public userver::components::ComponentBase {
public:
    static constexpr const char* kName = "rate-limiter";

    RateLimiterComponent(const userver::components::ComponentConfig& config,
                         const userver::components::ComponentContext& context);

    ~RateLimiterComponent() override = default;

    // Check if request is allowed using Sliding Window algorithm
    // Returns RateLimitInfo with allowed status and remaining requests
    RateLimitInfo IsAllowed(const std::string& user_id,
                           const std::string& endpoint,
                           int max_requests,
                           int window_seconds);

    // Get remaining requests for user
    int GetRemainingRequests(const std::string& user_id,
                             const std::string& endpoint,
                             int max_requests,
                             int window_seconds);

private:
    userver::storages::redis::ClientPtr redis_client_;
    
    // Sliding window implementation using Redis sorted sets
    bool ConsumeToken(const std::string& key, int max_tokens, int window_seconds);
    
    // Get current timestamp in milliseconds
    int64_t GetCurrentTimestampMs();
};

} // namespace common::cache
