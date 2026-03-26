#include "handler.hpp"

#include <userver/formats/json.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/server/http/http_status.hpp>

namespace user_service {

LoginResponse ProcessLogin(
    const LoginRequest& request,
    common::database::InMemoryDb& db,
    const common::JwtManager& jwt_manager) {
    
    LoginResponse response;
    
    auto result = db.LoginUser(request.username, request.password);

    if (!result.success) {
        response.success = false;
        response.error = result.error;
        return response;
    }

    response.success = true;
    response.token = jwt_manager.GenerateToken(
        result.user.id,
        result.user.role,
        24 * 7  // one week
    );
    
    return response;
}

LoginHandler::LoginHandler(const userver::components::ComponentConfig& config,
                                 const userver::components::ComponentContext& context)
    : userver::server::handlers::HttpHandlerBase(config, context),
      jwt_component_(context.FindComponent<common::JwtComponent>()) {}

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

        auto response = ProcessLogin(login_request, common::database::InMemoryDb::Instance(), jwt_component_.GetManager());

        if (!response.success) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
            return userver::formats::json::ToString(userver::formats::json::MakeObject(
                "error", "Invalid credentials"
            ));
        }

        request.SetResponseStatus(userver::server::http::HttpStatus::kOk);
        return userver::formats::json::ToString(userver::formats::json::MakeObject("token", response.token));

    } catch (const std::exception& ex) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(userver::formats::json::MakeObject(
            "error", std::string("Invalid request format: ") + ex.what()
        ));
    }
}

}  // namespace user_service
