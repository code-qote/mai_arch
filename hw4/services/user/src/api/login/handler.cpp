#include "handler.hpp"

#include <userver/formats/json.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/crypto/hash.hpp>

namespace user_service {

namespace {

std::string HashPassword(const std::string& password) {
    return userver::crypto::hash::Sha256(password);
}

}  // namespace

LoginHandler::LoginHandler(const userver::components::ComponentConfig& config,
                                 const userver::components::ComponentContext& context)
    : userver::server::handlers::HttpHandlerBase(config, context),
      jwt_component_(context.FindComponent<common::JwtComponent>()),
      db_(context.FindComponent<userver::components::Postgres>("postgres-db").GetCluster()) {}

std::string LoginHandler::HandleRequest(
    userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& /*request_context*/
) const {
    request.GetHttpResponse().SetContentType(userver::http::content_type::kApplicationJson);

    if (request.GetMethod() != userver::server::http::HttpMethod::kPost) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kMethodNotAllowed);
        return userver::formats::json::ToString(userver::formats::json::MakeObject(
            "error", "Method not allowed. Use POST for login."
        ));
    }

    try {
        const auto request_body = userver::formats::json::FromString(request.RequestBody());

        LoginRequest login_request;
        login_request.username = request_body["username"].As<std::string>();
        login_request.password = request_body["password"].As<std::string>();

        auto result = db_.LoginUser(login_request.username);

        if (!result.success) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
            return userver::formats::json::ToString(userver::formats::json::MakeObject(
                "error", "Invalid credentials"
            ));
        }

        std::string password_hash = HashPassword(login_request.password);
        if (result.user.password_hash != password_hash) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
            return userver::formats::json::ToString(userver::formats::json::MakeObject(
                "error", "Invalid credentials"
            ));
        }

        auto token = jwt_component_.GetManager().GenerateToken(
            result.user.id,
            result.user.role,
            24 * 7  // one week
        );

        request.SetResponseStatus(userver::server::http::HttpStatus::kOk);
        return userver::formats::json::ToString(userver::formats::json::MakeObject("token", token));

    } catch (const std::exception& ex) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(userver::formats::json::MakeObject(
            "error", std::string("Invalid request format: ") + ex.what()
        ));
    }
}

}  // namespace user_service
