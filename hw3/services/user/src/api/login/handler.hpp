#pragma once

#include <userver/components/component_list.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/storages/postgres/component.hpp>

#include <common/database/db.hpp>
#include <common/jwt/jwt_component.hpp>

namespace user_service {

struct LoginRequest {
    std::string username;
    std::string password;
};

struct LoginResponse {
    bool success;
    std::string token;
    std::string error;
};

class LoginHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-login";

    LoginHandler(const userver::components::ComponentConfig& config,
                    const userver::components::ComponentContext& context);

    std::string HandleRequest(userver::server::http::HttpRequest& request,
                              userver::server::request::RequestContext&) const override;

private:
    const common::JwtComponent& jwt_component_;
    common::database::PostgresDb db_;
};

}  // namespace user_service
