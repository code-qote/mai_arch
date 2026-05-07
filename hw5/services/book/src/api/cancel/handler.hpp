#pragma once

#include <userver/components/component_list.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/storages/postgres/component.hpp>

#include <common/database/db.hpp>
#include <common/database/booking.hpp>
#include <common/jwt/jwt_component.hpp>

namespace book_service {

struct CancelBookingRequest {
    std::string booking_id;
    std::string event_id;
};

struct CancelBookingResponse {
    bool success;
    common::database::Booking booking;
    std::string error;
};

class CancelBookingHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-cancel-booking";

    CancelBookingHandler(const userver::components::ComponentConfig& config,
                         const userver::components::ComponentContext& context);

    std::string HandleRequest(userver::server::http::HttpRequest& request,
                              userver::server::request::RequestContext& context) const override;

    static userver::yaml_config::Schema GetStaticConfigSchema();

private:
    bool ReleasePlacesInEventService(const std::string& event_id,
                                     const std::string& auth_token) const;
    
    bool ProcessRefund(const std::string& user_id,
                       const std::string& booking_id,
                       double amount) const;
    
    bool SendCancellationEmail(const std::string& user_id,
                               const common::database::Booking& booking) const;

    userver::clients::http::Client& http_client_;
    std::string event_service_url_;
    mutable common::database::PostgresDb db_;
};

}  // namespace book_service
