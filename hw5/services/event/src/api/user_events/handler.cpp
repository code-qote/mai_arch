#include "handler.hpp"

#include <string>
#include <userver/formats/json.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/logging/log.hpp>

namespace event_service {

GetUserEventsHandler::GetUserEventsHandler(const userver::components::ComponentConfig& config,
                                           const userver::components::ComponentContext& context)
    : userver::server::handlers::HttpHandlerBase(config, context),
      db_(context.FindComponent<userver::components::Postgres>("postgres-db").GetCluster()),
      redis_cache_(context.FindComponent<common::cache::RedisCacheComponent>("redis-cache")),
      rate_limiter_(context.FindComponent<common::cache::RateLimiterComponent>("rate-limiter")) {}

std::string GetUserEventsHandler::HandleRequest(userver::server::http::HttpRequest& request,
                                                 userver::server::request::RequestContext&) const {
    request.GetHttpResponse().SetContentType(userver::http::content_type::kApplicationJson);

    if (request.GetMethod() != userver::server::http::HttpMethod::kPost) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kMethodNotAllowed);
        return userver::formats::json::ToString(userver::formats::json::MakeObject(
            "error", "Method not allowed. Use POST to get user events."
        ));
    }

    try {
        const auto request_body = userver::formats::json::FromString(request.RequestBody());

        GetUserEventsRequest user_events_request;
        user_events_request.user_id = request_body["user_id"].As<std::string>("");

        if (user_events_request.user_id.empty()) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return userver::formats::json::ToString(userver::formats::json::MakeObject(
                "error", "user_id is required"
            ));
        }
    
        // Apply rate limiting using user_id as identifier
        std::string rate_limit_key = "get-user-events:" + user_events_request.user_id;
        auto rate_limit_info = rate_limiter_.IsAllowed(rate_limit_key, "get-user-events", kRateLimitMaxRequests, kRateLimitWindowSeconds);
        
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
    
        const std::string cache_key = user_events_request.GetCacheKey();
        
        // Try to get from cache first
        try {
            auto cached_result = redis_cache_.Get(cache_key);
            if (cached_result) {
                LOG_INFO() << "Cache hit for key: " << cache_key;
                return *cached_result;
            }
        } catch (const std::exception& ex) {
            LOG_WARNING() << "Cache read failed: " << ex.what();
            // Continue to database query on cache failure
        }

        // Cache miss - query database
        LOG_INFO() << "Cache miss for key: " << cache_key;
        
        common::database::BookingFilter filter;
        filter.user_id = user_events_request.user_id;
        filter.status = common::database::BookingStatus::kConfirmed;
        filter.filter_by_status = true;
        
        auto bookings = db_.FindBookings(filter);

        userver::formats::json::ValueBuilder response_builder;
        userver::formats::json::ValueBuilder bookings_array(userver::formats::common::Type::kArray);
        for (const auto& booking : bookings) {
            bookings_array.PushBack(common::database::Serialize(
                booking,
                userver::formats::serialize::To<userver::formats::json::Value>{}
            ));
        }
        response_builder["bookings"] = bookings_array.ExtractValue();
        response_builder["count"] = static_cast<int>(bookings.size());

        auto response_json = userver::formats::json::ToString(response_builder.ExtractValue());

        // Store in cache asynchronously
        try {
            redis_cache_.Set(cache_key, response_json, std::chrono::duration_cast<std::chrono::seconds>(kCacheTTL).count());
            LOG_INFO() << "Cached response for key: " << cache_key;
        } catch (const std::exception& ex) {
            LOG_WARNING() << "Cache write failed: " << ex.what();
            // Don't fail the request if cache write fails
        }

        return response_json;
    } catch (const std::exception& ex) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(userver::formats::json::MakeObject(
            "error", std::string("Invalid request format: ") + ex.what()
        ));
    }
}

}  // namespace event_service
