#include "handler.hpp"

#include <userver/formats/json.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/server/http/http_status.hpp>

#include <common/middleware/auth_middleware.hpp>

namespace event_service {

CancelPlacesResponse ProcessPlacesCancellation(
    const CancelPlacesRequest& request,
    const std::string& participant_id,
    const std::string& role,
    common::database::InMemoryDb& db) {
    
    CancelPlacesResponse response;
    
    if (role != common::kParticipantRole && role != common::kAdminRole) {
        response.success = false;
        response.error = "Only participants and admins can cancel bookings";
        return response;
    }
    
    auto result = db.CancelBooking(request.event_id, participant_id);

    if (!result.success) {
        response.success = false;
        response.error = result.error;
        return response;
    }

    response.success = true;
    response.event = result.event;
    
    return response;
}

CancelPlacesHandler::CancelPlacesHandler(const userver::components::ComponentConfig& config,
                                         const userver::components::ComponentContext& context)
    : userver::server::handlers::HttpHandlerBase(config, context) {}

std::string CancelPlacesHandler::HandleRequest(
    userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context
) const {
    request.GetHttpResponse().SetContentType(userver::http::content_type::kApplicationJson);

    if (request.GetMethod() != userver::server::http::HttpMethod::kPost) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kMethodNotAllowed);
        return userver::formats::json::ToString(userver::formats::json::MakeObject(
            "error", "Method not allowed. Use POST for canceling bookings."
        ));
    }

    try {
        const auto request_body = userver::formats::json::FromString(request.RequestBody());

        CancelPlacesRequest cancel_request;
        cancel_request.event_id = request_body["event_id"].As<std::string>();

        auto user_id = context.GetData<std::string>(common::kUserIdKey);
        auto role = context.GetData<std::string>(common::kRoleKey);

        auto response = ProcessPlacesCancellation(cancel_request, user_id, role, common::database::InMemoryDb::Instance());

        if (!response.success) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return userver::formats::json::ToString(userver::formats::json::MakeObject(
                "error", response.error
            ));
        }

        request.SetResponseStatus(userver::server::http::HttpStatus::kOk);
        return userver::formats::json::ToString(userver::formats::json::MakeObject(
            "success", true,
            "event", Serialize(response.event, userver::formats::serialize::To<userver::formats::json::Value>{})
        ));

    } catch (const std::exception& ex) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(userver::formats::json::MakeObject(
            "error", std::string("Invalid request format: ") + ex.what()
        ));
    }
}

}  // namespace event_service
