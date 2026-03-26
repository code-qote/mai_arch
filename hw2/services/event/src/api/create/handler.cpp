#include "handler.hpp"

#include <userver/formats/json.hpp>
#include <userver/http/common_headers.hpp>

#include <common/middleware/auth_middleware.hpp>

namespace event_service {

CreateEventResponse ProcessEventCreation(
    const CreateEventRequest& request,
    const std::string& organizer_id,
    common::database::InMemoryDb& db) {
    
    CreateEventResponse response;
    
    auto result = db.CreateEvent(
        request.name,
        request.geo_position,
        request.places_count,
        request.event_time,
        organizer_id
    );

    if (!result.success) {
        response.success = false;
        response.error = result.error;
        return response;
    }

    response.success = true;
    response.event = result.event;
    
    return response;
}

CreateEventHandler::CreateEventHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : userver::server::handlers::HttpHandlerBase(config, context) {}

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

        auto response = ProcessEventCreation(create_request, user_id, common::database::InMemoryDb::Instance());

        if (!response.success) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return userver::formats::json::ToString(userver::formats::json::MakeObject(
                "error", response.error
            ));
        }

        request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
        return userver::formats::json::ToString(
            Serialize(response.event, userver::formats::serialize::To<userver::formats::json::Value>{})
        );

    } catch (const std::exception& ex) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(userver::formats::json::MakeObject(
            "error", std::string("Invalid request format: ") + ex.what()
        ));
    }
}

}  // namespace event_service
