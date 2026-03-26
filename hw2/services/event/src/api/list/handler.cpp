#include "handler.hpp"
#include "common/database/db.hpp"

#include <string>
#include <userver/formats/json.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/server/http/http_status.hpp>

namespace event_service {

ListEventsResponse ProcessListEvents(
    const ListEventsRequest& request,
    common::database::InMemoryDb& db) {
    
    ListEventsResponse response;
    response.events = db.FindEvents(request.ToFilter());
    
    return response;
}

ListEventsHandler::ListEventsHandler(const userver::components::ComponentConfig& config,
                                     const userver::components::ComponentContext& context)
    : userver::server::handlers::HttpHandlerBase(config, context) {}

std::string ListEventsHandler::HandleRequest(userver::server::http::HttpRequest& request,
                                             userver::server::request::RequestContext&) const {
    request.GetHttpResponse().SetContentType(userver::http::content_type::kApplicationJson);

    if (request.GetMethod() != userver::server::http::HttpMethod::kPost) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kMethodNotAllowed);
        return userver::formats::json::ToString(userver::formats::json::MakeObject(
            "error", "Method not allowed. Use POST to list events."
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

        auto response = ProcessListEvents(list_request, common::database::InMemoryDb::Instance());

        userver::formats::json::ValueBuilder response_builder;
        userver::formats::json::ValueBuilder events_array(userver::formats::common::Type::kArray);
        for (const auto& event : response.events) {
            events_array.PushBack(common::database::Serialize(
                event,
                userver::formats::serialize::To<userver::formats::json::Value>{}
            ));
        }
        response_builder["events"] = events_array.ExtractValue();

        return userver::formats::json::ToString(response_builder.ExtractValue());
    } catch (const std::exception& ex) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(userver::formats::json::MakeObject(
            "error", std::string("Invalid request format: ") + ex.what()
        ));
    }
}

}  // namespace event_service
