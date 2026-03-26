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

userver::yaml_config::Schema CancelBookingHandler::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<HttpHandlerBase>(R"(
type: object
description: Cancel booking handler schema
additionalProperties: false
properties:
    event_service_url:
        type: string
        description: URL of the event service
        defaultDescription: http://localhost:8081
)");
}

CancelBookingHandler::CancelBookingHandler(const userver::components::ComponentConfig& config,
                                           const userver::components::ComponentContext& context)
    : userver::server::handlers::HttpHandlerBase(config, context),
      http_client_(context.FindComponent<userver::components::HttpClient>().GetHttpClient()),
      event_service_url_(config["event_service_url"].As<std::string>("http://localhost:8081")) {}

bool CancelBookingHandler::ReleasePlacesInEventService(const std::string& event_id,
                                                        const std::string& auth_token) const {
    try {
        auto request_body = userver::formats::json::MakeObject(
            "event_id", event_id
        );

        auto response = http_client_.CreateRequest()
            .post(event_service_url_ + "/events/cancel")
            .headers({{"Authorization", "Bearer " + auth_token},
                      {"Content-Type", "application/json"}})
            .data(userver::formats::json::ToString(request_body))
            .timeout(std::chrono::seconds(5))
            .perform();

        if (response->status_code() == 200) {
            LOG_INFO() << "Successfully released places in event service for event: " << event_id;
            return true;
        } else {
            LOG_WARNING() << "Failed to release places in event service. Status: "
                          << response->status_code() << ", Body: " << response->body();
            return false;
        }
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Exception while calling event service: " << ex.what();
        return false;
    }
}

bool CancelBookingHandler::ProcessRefund(const std::string& user_id,
                                          const std::string& booking_id,
                                          double amount) const {
    LOG_INFO() << "Processing refund for user: " << user_id
               << ", booking: " << booking_id
               << ", amount: " << amount;
    return true;
}

bool CancelBookingHandler::SendCancellationEmail(const std::string& user_id,
                                                  const common::database::Booking& booking) const {
    LOG_INFO() << "Sending cancellation email to user: " << user_id
               << " for booking: " << booking.id;
    return true;
}

std::string CancelBookingHandler::HandleRequest(
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

        CancelBookingRequest cancel_request;
        cancel_request.booking_id = request_body["booking_id"].As<std::string>();
        cancel_request.event_id = request_body["event_id"].As<std::string>();

        auto user_id = context.GetData<std::string>(common::kUserIdKey);
        auto role = context.GetData<std::string>(common::kRoleKey);

        if (role != common::kParticipantRole && role != common::kAdminRole) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
            return userver::formats::json::ToString(userver::formats::json::MakeObject(
                "error", "Only participants and admins can cancel bookings"
            ));
        }

        auto auth_header = request.GetHeader(common::kAuthorizationHeader);
        std::string auth_token;
        if (auth_header.starts_with(common::kAuthorizationPrefix)) {
            auth_token = auth_header.substr(common::kAuthorizationPrefix.size());
        }

        auto booking_result = common::database::InMemoryDb::Instance().GetBooking(cancel_request.booking_id);
        if (!booking_result.success) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
            return userver::formats::json::ToString(userver::formats::json::MakeObject(
                "error", "Booking not found"
            ));
        }

        auto booking = booking_result.booking;

        if (booking.user_id != user_id && role != common::kAdminRole) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
            return userver::formats::json::ToString(userver::formats::json::MakeObject(
                "error", "You can only cancel your own bookings"
            ));
        }

        if (booking.status == common::database::BookingStatus::kCancelled) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return userver::formats::json::ToString(userver::formats::json::MakeObject(
                "error", "Booking is already cancelled"
            ));
        }

        if (booking.status != common::database::BookingStatus::kConfirmed &&
            booking.status != common::database::BookingStatus::kPending) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return userver::formats::json::ToString(userver::formats::json::MakeObject(
                "error", "Booking cannot be cancelled in current state"
            ));
        }

        if (!ReleasePlacesInEventService(booking.event_id, auth_token)) {
            LOG_WARNING() << "Failed to release places in event service, but continuing with cancellation";
        }

        if (booking.status == common::database::BookingStatus::kConfirmed) {
            double refund_amount = booking.places_count * 100.0;
            ProcessRefund(user_id, booking.id, refund_amount);
        }

        auto update_result = common::database::InMemoryDb::Instance().UpdateBookingStatus(booking.id, common::database::BookingStatus::kCancelled);
        if (!update_result.success) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kInternalServerError);
            return userver::formats::json::ToString(userver::formats::json::MakeObject(
                "error", "Failed to update booking status"
            ));
        }
        booking = update_result.booking;

        SendCancellationEmail(user_id, booking);

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
