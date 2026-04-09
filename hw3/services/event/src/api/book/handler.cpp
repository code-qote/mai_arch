#include "handler.hpp"

#include <userver/formats/json.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/server/http/http_status.hpp>

#include <common/middleware/auth_middleware.hpp>

namespace event_service {

BookPlacesHandler::BookPlacesHandler(const userver::components::ComponentConfig& config,
                                     const userver::components::ComponentContext& context)
    : userver::server::handlers::HttpHandlerBase(config, context),
      db_(context.FindComponent<userver::components::Postgres>("postgres-db").GetCluster()) {}

std::string BookPlacesHandler::HandleRequest(
    userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context
) const {
    request.GetHttpResponse().SetContentType(userver::http::content_type::kApplicationJson);

    if (request.GetMethod() != userver::server::http::HttpMethod::kPost) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kMethodNotAllowed);
        return userver::formats::json::ToString(userver::formats::json::MakeObject(
            "error", "Method not allowed. Use POST for booking places."
        ));
    }

    try {
        const auto request_body = userver::formats::json::FromString(request.RequestBody());

        BookPlacesRequest book_request;
        book_request.event_id = request_body["event_id"].As<std::string>();
        book_request.places_count = request_body["places_count"].As<int>(1);

        auto user_id = context.GetData<std::string>(common::kUserIdKey);
        auto role = context.GetData<std::string>(common::kRoleKey);

        if (role != common::kParticipantRole && role != common::kAdminRole) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
            return userver::formats::json::ToString(userver::formats::json::MakeObject(
                "error", "Only participants and admins can book places"
            ));
        }

        auto result = db_.ReservePlaces(book_request.event_id, book_request.places_count);

        if (!result.success) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return userver::formats::json::ToString(userver::formats::json::MakeObject(
                "error", result.error
            ));
        }

        request.SetResponseStatus(userver::server::http::HttpStatus::kOk);
        return userver::formats::json::ToString(userver::formats::json::MakeObject(
            "success", true,
            "event", Serialize(result.event, userver::formats::serialize::To<userver::formats::json::Value>{})
        ));

    } catch (const std::exception& ex) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(userver::formats::json::MakeObject(
            "error", std::string("Invalid request format: ") + ex.what()
        ));
    }
}

}  // namespace event_service
