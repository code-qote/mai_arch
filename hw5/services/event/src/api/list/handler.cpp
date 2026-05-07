#include "handler.hpp"

#include <string>
#include <userver/formats/json.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/logging/log.hpp>

namespace event_service {

ListEventsHandler::ListEventsHandler(const userver::components::ComponentConfig& config,
                                     const userver::components::ComponentContext& context)
    : userver::server::handlers::HttpHandlerBase(config, context),
      db_(context.FindComponent<userver::components::Postgres>("postgres-db").GetCluster()),
      redis_cache_(context.FindComponent<common::cache::RedisCacheComponent>("redis-cache")),
      rate_limiter_(context.FindComponent<common::cache::RateLimiterComponent>("rate-limiter")) {}

std::string ListEventsHandler::HandleRequest(userver::server::http::HttpRequest& request,
                                             userver::server::request::RequestContext&) const {
    request.GetHttpResponse().SetContentType(userver::http::content_type::kApplicationJson);

    if (request.GetMethod() != userver::server::http::HttpMethod::kPost) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kMethodNotAllowed);
        return userver::formats::json::ToString(userver::formats::json::MakeObject(
            "error", "Method not allowed. Use POST to list events."
        ));
    }

    // Apply rate limiting using a simple identifier
    std::string rate_limit_key = "list-events:" + request.GetHost();
    auto rate_limit_info = rate_limiter_.IsAllowed(rate_limit_key, "list-events", kRateLimitMaxRequests, kRateLimitWindowSeconds);
    
    // Set rate limit headers
    auto& response = request.GetHttpResponse();
    response.SetContentType(userver::http::content_type::kApplicationJson);
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

        ListEventsRequest list_request;
        list_request.id = request_body["id"].As<std::string>("");
        list_request.name = request_body["name"].As<std::string>("");
        list_request.organizer_id = request_body["organizer_id"].As<std::string>("");
        list_request.country = request_body["country"].As<std::string>("");
        list_request.city = request_body["city"].As<std::string>("");
        list_request.date_from = request_body["date_from"].As<std::string>("");
        list_request.date_to = request_body["date_to"].As<std::string>("");

        const std::string cache_key = list_request.GetCacheKey();
        
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
        auto events = db_.FindEvents(list_request.ToFilter());

        userver::formats::json::ValueBuilder response_builder;
        userver::formats::json::ValueBuilder events_array(userver::formats::common::Type::kArray);
        for (const auto& event : events) {
            events_array.PushBack(common::database::Serialize(
                event,
                userver::formats::serialize::To<userver::formats::json::Value>{}
            ));
        }
        response_builder["events"] = events_array.ExtractValue();

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
