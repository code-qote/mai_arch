#pragma once

#include <userver/components/component_list.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/formats/json/value.hpp>

#include <common/database/db.hpp>
#include <common/jwt/jwt_component.hpp>

namespace user_service {

struct RegisterRequest {
    std::string username;
    std::string email;
    std::string first_name;
    std::string last_name;
    std::string password;
    std::string role;
};

struct RegisterResponse {
    bool success;
    std::string token;
    std::string error;
};

RegisterResponse ProcessRegistration(
    const RegisterRequest& request,
    common::database::InMemoryDb& db,
    const common::JwtManager& jwt_manager);

class RegisterHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-register";

    RegisterHandler(const userver::components::ComponentConfig& config,
                    const userver::components::ComponentContext& context);

    std::string HandleRequest(userver::server::http::HttpRequest& request,
                              userver::server::request::RequestContext&) const override;

private:
    const common::JwtComponent& jwt_component_;
};

}  // namespace user_service
