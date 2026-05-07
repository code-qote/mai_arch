#include "handler.hpp"

#include <userver/formats/json.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/logging/log.hpp>

#include <common/middleware/auth_middleware.hpp>

namespace event_service {

CreateEventHandler::CreateEventHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : userver::server::handlers::HttpHandlerBase(config, context),
      db_(context.FindComponent<userver::components::Postgres>("postgres-db").GetCluster()),
      redis_cache_(context.FindComponent<common::cache::RedisCacheComponent>("redis-cache")),
      rate_limiter_(context.FindComponent<common::cache::RateLimiterComponent>("rate-limiter")) {}

std::string CreateEventHandler::HandleRequest(
    userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context
) const {
    request.GetHttpResponse().SetContentType(userver::http::content_type::kApplicationJson);

    if (request.GetMethod() != userver::server::http::HttpMethod::kPost) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kMethodNotAllowed);
        return userver::formats::json::ToString(userver::formats::json::MakeObject(
            "error", "Method not allowed. Use POST to create an event."
        ));
    }

    auto user_role = context.GetData<std::string>(common::kRoleKey);
    auto user_id = context.GetData<std::string>(common::kUserIdKey);

    if (user_role != common::kAdminRole && user_role != common::kOrganizerRole) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
        return userver::formats::json::ToString(userver::formats::json::MakeObject(
            "error", "Access denied. Only admin or organizer can create events."
        ));
    }

    // Apply rate limiting using user_id as identifier
    std::string rate_limit_key = "create-event:" + user_id;
    auto rate_limit_info = rate_limiter_.IsAllowed(rate_limit_key, "create-event", kRateLimitMaxRequests, kRateLimitWindowSeconds);
    
    // Set rate limit headers
    auto& response = request.GetHttpResponse();
    response.SetHeader(std::string_view{"X-RateLimit-Limit"}, std::to_string(kRateLimitMaxRequests));
    response.SetHeader(std::string_view{"X-RateLimit-Remaining"}, std::to_string(rate_limit_info.remaining));
    response.SetHeader(std::string_view{"X-RateLimit-Reset"}, std::to_string(rate_limit_info.reset_time));
    
    if (!rate_limit_info.allowed) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kTooManyRequests);
        return userver::formats::json::ToString(userver::formats::json::MakeObject(
            "error", "Rate limit exceeded. Please try again later."
        ));
    }

    try {
        const auto request_body = userver::formats::json::FromString(request.RequestBody());

        CreateEventRequest create_request;
        create_request.name = request_body["name"].As<std::string>();
        
        const auto& geo = request_body["geo_position"];
        create_request.geo_position.country = geo["country"].As<std::string>();
        create_request.geo_position.city = geo["city"].As<std::string>();
        create_request.geo_position.street = geo["street"].As<std::string>();
        
        create_request.places_count = request_body["places_count"].As<int>();
        create_request.event_time = request_body["event_time"].As<std::string>();

        auto result = db_.CreateEvent(
            create_request.name,
            create_request.geo_position,
            create_request.places_count,
            create_request.event_time,
            user_id
        );

        if (!result.success) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return userver::formats::json::ToString(userver::formats::json::MakeObject(
                "error", result.error
            ));
        }

        // Invalidate event list cache after successful creation
        try {
            redis_cache_.DeletePattern("events:list:*");
            LOG_INFO() << "Invalidated event list cache";
        } catch (const std::exception& ex) {
            LOG_WARNING() << "Failed to invalidate event list cache: " << ex.what();
        }

        request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
        return userver::formats::json::ToString(
            Serialize(result.event, userver::formats::serialize::To<userver::formats::json::Value>{})
        );

    } catch (const std::exception& ex) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(userver::formats::json::MakeObject(
            "error", std::string("Invalid request format: ") + ex.what()
        ));
    }
}

}  // namespace event_service
