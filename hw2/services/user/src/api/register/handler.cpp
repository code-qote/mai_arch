#include "handler.hpp"
#include "common/jwt/jwt.hpp"

#include <userver/formats/json.hpp>
#include <userver/http/common_headers.hpp>

namespace user_service {

RegisterResponse ProcessRegistration(
    const RegisterRequest& request,
    common::database::InMemoryDb& db,
    const common::JwtManager& jwt_manager) {
    
    RegisterResponse response;
    
    auto result = db.RegisterUser(
        request.username,
        request.email,
        request.first_name,
        request.last_name,
        request.password,
        request.role
    );

    if (!result.success) {
        response.success = false;
        response.error = result.error;
        return response;
    }

    response.success = true;
    response.token = jwt_manager.GenerateToken(
        result.user.id,
        request.role
    );
    
    return response;
}

RegisterHandler::RegisterHandler(const userver::components::ComponentConfig& config,
                                 const userver::components::ComponentContext& context)
    : userver::server::handlers::HttpHandlerBase(config, context),
      jwt_component_(context.FindComponent<common::JwtComponent>()) {}

std::string RegisterHandler::HandleRequest(
    userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& /*request_context*/
) const {
    request.GetHttpResponse().SetContentType(userver::http::content_type::kApplicationJson);

    if (request.GetMethod() != userver::server::http::HttpMethod::kPost) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kMethodNotAllowed);
        return userver::formats::json::ToString(userver::formats::json::MakeObject(
            "error", "Method not allowed. Use POST for registration."
        ));
    }

    try {
        const auto request_body = userver::formats::json::FromString(request.RequestBody());

        RegisterRequest reg_request;
        reg_request.username = request_body["username"].As<std::string>();
        reg_request.email = request_body["email"].As<std::string>();
        reg_request.first_name = request_body["first_name"].As<std::string>();
        reg_request.last_name = request_body["last_name"].As<std::string>();
        reg_request.password = request_body["password"].As<std::string>();
        reg_request.role = request_body["role"].As<std::string>();

        if (reg_request.role == common::kAdminRole) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return userver::formats::json::ToString(userver::formats::json::MakeObject(
                "error", "admin user cannot be registered"
            ));
        }

        auto response = ProcessRegistration(reg_request, common::database::InMemoryDb::Instance(), jwt_component_.GetManager());

        if (!response.success) {
            if (response.error.find("already exists") != std::string::npos) {
                request.SetResponseStatus(userver::server::http::HttpStatus::kConflict);
            } else {
                request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            }
            return userver::formats::json::ToString(userver::formats::json::MakeObject(
                "error", response.error
            ));
        }

        request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
        return userver::formats::json::ToString(userver::formats::json::MakeObject("token", response.token));

    } catch (const std::exception& ex) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(userver::formats::json::MakeObject(
            "error", std::string("Invalid request format: ") + ex.what()
        ));
    }
}

}  // namespace user_service
