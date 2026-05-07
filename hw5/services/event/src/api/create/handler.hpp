#pragma once

#include <userver/components/component_list.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/storages/postgres/component.hpp>

#include <common/database/db.hpp>
#include <common/jwt/jwt_component.hpp>
#include <common/cache/redis_cache_component.hpp>
#include <common/cache/rate_limiter.hpp>

#include <memory>

namespace event_service {

struct CreateEventRequest {
    std::string name;
    common::database::GeoPosition geo_position;
    int places_count;
    std::string event_time;
};

struct CreateEventResponse {
    bool success;
    common::database::Event event;
    std::string error;
};

class CreateEventHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-create-event";

    CreateEventHandler(const userver::components::ComponentConfig& config,
                       const userver::components::ComponentContext& context);

    std::string HandleRequest(userver::server::http::HttpRequest& request,
                              userver::server::request::RequestContext& context) const override;

private:
    common::database::PostgresDb db_;
    common::cache::RedisCacheComponent& redis_cache_;
    common::cache::RateLimiterComponent& rate_limiter_;
    
    static constexpr int kRateLimitMaxRequests{1000}; // 1000 requests per window
    static constexpr int kRateLimitWindowSeconds{60}; // 60 seconds window
};

}  // namespace event_service
