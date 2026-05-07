#include "handler.hpp"

#include <common/database/analytics_mongo.hpp>

#include <userver/components/component_context.hpp>
#include <userver/formats/json.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/storages/mongo/component.hpp>
#include <userver/utils/datetime.hpp>

namespace analytics_service {

namespace {

namespace formats = userver::formats;

}

LogActivityHandler::LogActivityHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerJsonBase(config, context),
      mongo_pool_(context.FindComponent<userver::components::Mongo>("analytics-mongo").GetPool()) {}

formats::json::Value LogActivityHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest& request,
    const formats::json::Value& json,
    userver::server::request::RequestContext&) const {
    
    if (request.GetMethod() != userver::server::http::HttpMethod::kPost) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kMethodNotAllowed);
        return formats::json::MakeObject("error", "Method not allowed. Use POST to log activity.");
    }

    try {
        common::database::ActivityLog log;
        log.user_id = json["user_id"].As<std::string>();
        log.activity_type = json["activity_type"].As<std::string>();
        log.timestamp = std::chrono::system_clock::now();
        
        log.event_id = json["event_id"].As<std::string>("");
        log.event_name = json["event_name"].As<std::string>("");
        log.booking_id = json["booking_id"].As<std::string>("");
        log.places_count = json["places_count"].As<int>(0);
        log.amount = json["amount"].As<double>(0.0);
        log.session_id = json["session_id"].As<std::string>("");
        log.ip_address = json["ip_address"].As<std::string>("");
        log.user_agent = json["user_agent"].As<std::string>("");

        common::database::AnalyticsMongo analytics_db(mongo_pool_);
        auto result = analytics_db.LogActivity(log);

        if (!result.success) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kInternalServerError);
            return formats::json::MakeObject("error", result.error);
        }

        request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
        return formats::json::MakeObject(
            "success", true,
            "message", "Activity logged successfully"
        );

    } catch (const std::exception& e) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return formats::json::MakeObject(
            "error", std::string("Invalid request format: ") + e.what()
        );
    }
}

}
