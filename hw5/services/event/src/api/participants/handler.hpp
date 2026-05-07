#pragma once

#include <userver/components/component_list.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/storages/postgres/component.hpp>

#include <common/database/db.hpp>
#include <common/database/booking.hpp>
#include <common/cache/redis_cache_component.hpp>
#include <common/cache/rate_limiter.hpp>

#include <memory>

namespace event_service {

struct GetParticipantsRequest {
    std::string event_id;

    std::string GetCacheKey() const {
        return "events:participants:" + event_id;
    }
};

struct GetParticipantsResponse {
    std::vector<common::database::Booking> bookings;
};

class GetParticipantsHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-get-participants";

    GetParticipantsHandler(const userver::components::ComponentConfig& config,
                           const userver::components::ComponentContext& context);

    std::string HandleRequest(userver::server::http::HttpRequest& request,
                              userver::server::request::RequestContext& context) const override;

private:
    common::database::PostgresDb db_;
    common::cache::RedisCacheComponent& redis_cache_;
    common::cache::RateLimiterComponent& rate_limiter_;
    
    static constexpr std::chrono::seconds kCacheTTL{300}; // 5 minutes
    static constexpr int kRateLimitMaxRequests{1000}; // 1000 requests per window
    static constexpr int kRateLimitWindowSeconds{60}; // 60 seconds window
};

}  // namespace event_service
