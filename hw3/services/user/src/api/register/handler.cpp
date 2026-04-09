#include "handler.hpp"
#include "common/jwt/jwt.hpp"

#include <userver/formats/json.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/crypto/hash.hpp>

namespace user_service {

namespace {

std::string HashPassword(const std::string& password) {
    return userver::crypto::hash::Sha256(password);
}

}  // namespace

RegisterHandler::RegisterHandler(const userver::components::ComponentConfig& config,
                                 const userver::components::ComponentContext& context)
    : userver::server::handlers::HttpHandlerBase(config, context),
      jwt_component_(context.FindComponent<common::JwtComponent>()),
      db_(context.FindComponent<userver::components::Postgres>("postgres-db").GetCluster()) {}

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

        std::string password_hash = HashPassword(reg_request.password);

        auto result = db_.RegisterUser(
            reg_request.username,
            reg_request.email,
            reg_request.first_name,
            reg_request.last_name,
            password_hash,
            reg_request.role
        );

        if (!result.success) {
            if (result.error.find("already exists") != std::string::npos) {
                request.SetResponseStatus(userver::server::http::HttpStatus::kConflict);
            } else {
                request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            }
            return userver::formats::json::ToString(userver::formats::json::MakeObject(
                "error", result.error
            ));
        }

        auto token = jwt_component_.GetManager().GenerateToken(
            result.user.id,
            reg_request.role
        );

        request.SetResponseStatus(userver::server::http::HttpStatus::kCreated);
        return userver::formats::json::ToString(userver::formats::json::MakeObject("token", token));

    } catch (const std::exception& ex) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(userver::formats::json::MakeObject(
            "error", std::string("Invalid request format: ") + ex.what()
        ));
    }
}

}  // namespace user_service
