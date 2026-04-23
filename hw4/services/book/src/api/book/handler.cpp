#include "handler.hpp"
#include "common/jwt/jwt.hpp"

#include <userver/formats/json.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/clients/http/client.hpp>
#include <userver/logging/log.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <common/middleware/auth_middleware.hpp>

namespace book_service {

userver::yaml_config::Schema BookEventHandler::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<HttpHandlerBase>(R"(
type: object
description: Book event handler schema
additionalProperties: false
properties:
    event_service_url:
        type: string
        description: URL of the event service
        defaultDescription: http://localhost:8081
)");
}

BookEventHandler::BookEventHandler(const userver::components::ComponentConfig& config,
                                   const userver::components::ComponentContext& context)
    : userver::server::handlers::HttpHandlerBase(config, context),
      http_client_(context.FindComponent<userver::components::HttpClient>().GetHttpClient()),
      event_service_url_(config["event_service_url"].As<std::string>("http://localhost:8081")),
      db_(context.FindComponent<userver::components::Postgres>("postgres-db").GetCluster()) {}

bool BookEventHandler::ReservePlacesInEventService(const std::string& event_id,
                                                    int places_count,
                                                    const std::string& auth_token) const {
    try {
        auto request_body = userver::formats::json::MakeObject(
            "event_id", event_id,
            "places_count", places_count
        );

        auto response = http_client_.CreateRequest()
            .post(event_service_url_ + "/events/book")
            .headers({{"Authorization", "Bearer " + auth_token},
                      {"Content-Type", "application/json"}})
            .data(userver::formats::json::ToString(request_body))
            .timeout(std::chrono::seconds(5))
            .perform();

        if (response->status_code() == 200) {
            LOG_INFO() << "Successfully reserved places in event service for event: " << event_id;
            return true;
        } else {
            LOG_WARNING() << "Failed to reserve places in event service. Status: " 
                          << response->status_code() << ", Body: " << response->body();
            return false;
        }
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Exception while calling event service: " << ex.what();
        return false;
    }
}

bool BookEventHandler::ProcessPayment(const std::string& user_id,
                                       const std::string& booking_id,
                                       double amount) const {
    LOG_INFO() << "Processing payment for user: " << user_id
               << ", booking: " << booking_id
               << ", amount: " << amount;
    return true;
}

bool BookEventHandler::SendConfirmationEmail(const std::string& user_id,
                                              const common::database::Booking& booking) const {
    LOG_INFO() << "Sending confirmation email to user: " << user_id
               << " for booking: " << booking.id;
    return true;
}

std::string BookEventHandler::HandleRequest(
    userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context
) const {
    request.GetHttpResponse().SetContentType(userver::http::content_type::kApplicationJson);

    if (request.GetMethod() != userver::server::http::HttpMethod::kPost) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kMethodNotAllowed);
        return userver::formats::json::ToString(userver::formats::json::MakeObject(
            "error", "Method not allowed. Use POST for booking."
        ));
    }

    try {
        const auto request_body = userver::formats::json::FromString(request.RequestBody());

        BookEventRequest book_request;
        book_request.event_id = request_body["event_id"].As<std::string>();
        book_request.places_count = request_body["places_count"].As<int>(1);

        auto user_id = context.GetData<std::string>(common::kUserIdKey);
        auto role = context.GetData<std::string>(common::kRoleKey);

        if (role != common::kParticipantRole && role != common::kAdminRole) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
            return userver::formats::json::ToString(userver::formats::json::MakeObject(
                "error", "Only participants and admins can book events"
            ));
        }

        auto auth_header = request.GetHeader(common::kAuthorizationHeader);
        std::string auth_token;
        if (auth_header.starts_with(common::kAuthorizationPrefix)) {
            auth_token = auth_header.substr(common::kAuthorizationPrefix.size());
        }

        auto booking_result = db_.CreateBooking(book_request.event_id, user_id, book_request.places_count);
        if (!booking_result.success) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return userver::formats::json::ToString(userver::formats::json::MakeObject(
                "error", booking_result.error
            ));
        }

        auto booking = booking_result.booking;
        LOG_INFO() << "Created booking " << booking.id << " in pending state";

        if (!ReservePlacesInEventService(book_request.event_id, book_request.places_count, auth_token)) {
            db_.UpdateBookingStatus(booking.id, common::database::BookingStatus::kFailed);
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return userver::formats::json::ToString(userver::formats::json::MakeObject(
                "error", "Failed to reserve places in event service"
            ));
        }

        double payment_amount = book_request.places_count * 100.0;
        if (!ProcessPayment(user_id, booking.id, payment_amount)) {
            db_.UpdateBookingStatus(booking.id, common::database::BookingStatus::kFailed);
            request.SetResponseStatus(userver::server::http::HttpStatus::kPaymentRequired);
            return userver::formats::json::ToString(userver::formats::json::MakeObject(
                "error", "Payment processing failed"
            ));
        }

        auto update_result = db_.UpdateBookingStatus(booking.id, common::database::BookingStatus::kConfirmed);
        if (!update_result.success) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kInternalServerError);
            return userver::formats::json::ToString(userver::formats::json::MakeObject(
                "error", "Failed to update booking status"
            ));
        }
        booking = update_result.booking;

        SendConfirmationEmail(user_id, booking);

        request.SetResponseStatus(userver::server::http::HttpStatus::kOk);
        return userver::formats::json::ToString(userver::formats::json::MakeObject(
            "success", true,
            "booking", Serialize(booking, userver::formats::serialize::To<userver::formats::json::Value>{})
        ));

    } catch (const std::exception& ex) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(userver::formats::json::MakeObject(
            "error", std::string("Invalid request format: ") + ex.what()
        ));
    }
}

}  // namespace book_service
