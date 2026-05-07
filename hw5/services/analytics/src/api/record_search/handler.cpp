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

RecordSearchHandler::RecordSearchHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerJsonBase(config, context),
      mongo_pool_(context.FindComponent<userver::components::Mongo>("analytics-mongo").GetPool()) {}

formats::json::Value RecordSearchHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest& request,
    const formats::json::Value& json,
    userver::server::request::RequestContext&) const {
    
    if (request.GetMethod() != userver::server::http::HttpMethod::kPost) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kMethodNotAllowed);
        return formats::json::MakeObject("error", "Method not allowed. Use POST to record search.");
    }

    try {
        common::database::SearchHistory search;
        search.user_id = json["user_id"].As<std::string>();
        search.city = json["city"].As<std::string>("");
        search.country = json["country"].As<std::string>("");
        search.name_keyword = json["name_keyword"].As<std::string>("");
        search.results_count = json["results_count"].As<int>(0);
        search.conversion = json["conversion"].As<bool>(false);
        search.searched_at = std::chrono::system_clock::now();
        search.session_id = json["session_id"].As<std::string>("");

        if (json.HasMember("clicked_event_ids")) {
            for (const auto& event_id : json["clicked_event_ids"]) {
                search.clicked_event_ids.push_back(event_id.As<std::string>());
            }
        }

        if (json.HasMember("booked_event_id")) {
            search.booked_event_id = json["booked_event_id"].As<std::string>();
        }

        common::database::AnalyticsMongo analytics_db(mongo_pool_);
        auto result = analytics_db.RecordSearch(search);

        if (!result.success) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kInternalServerError);
            return formats::json::MakeObject("error", result.error);
        }

        request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
        return formats::json::MakeObject(
            "success", true,
            "message", "Search recorded successfully"
        );

    } catch (const std::exception& e) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return formats::json::MakeObject(
            "error", std::string("Invalid request format: ") + e.what()
        );
    }
}

}
