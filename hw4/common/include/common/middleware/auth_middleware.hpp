#pragma once

#include <string>
#include <memory>
#include <unordered_set>

#include <userver/server/middlewares/http_middleware_base.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/server/request/request_context.hpp>
#include <userver/formats/json.hpp>
#include <userver/components/component_context.hpp>

#include <common/jwt/jwt_component.hpp>

namespace common {

inline const std::string kAuthorizationHeader = "Authorization";
inline const std::string kAuthorizationPrefix = "Bearer ";
inline const std::string kUserIdKey = "user_id";
inline const std::string kRoleKey = "role";

inline const std::unordered_set<std::string> kPublicPaths = {
    "/register",
    "/login",
    "/events/list"
};

class AuthMiddleware final : public userver::server::middlewares::HttpMiddlewareBase {
public:
    static constexpr std::string_view kName = "auth-middleware";

    explicit AuthMiddleware(const JwtComponent& jwt_component)
        : jwt_component_(jwt_component) {}

protected:
    void HandleRequest(
        userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context
    ) const override {
        std::string path(request.GetRequestPath());
        if (kPublicPaths.contains(path)) {
            Next(request, context);
            return;
        }

        auto auth_header = request.GetHeader(kAuthorizationHeader);
        
        if (auth_header.empty() || !auth_header.starts_with(kAuthorizationPrefix)) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
            request.GetHttpResponse().SetContentType("application/json");
            request.GetHttpResponse().SetData(
                userver::formats::json::ToString(
                    userver::formats::json::MakeObject("error", "Missing or invalid Authorization header")
                )
            );
            return;
        }

        auto token = auth_header.substr(kAuthorizationPrefix.size());
        auto payload = jwt_component_.GetManager().VerifyToken(token);
        
        if (!payload.has_value()) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
            request.GetHttpResponse().SetContentType("application/json");
            request.GetHttpResponse().SetData(
                userver::formats::json::ToString(
                    userver::formats::json::MakeObject("error", "Invalid or expired token")
                )
            );
            return;
        }

        context.SetData(kUserIdKey, payload->user_id);
        context.SetData(kRoleKey, payload->role);

        Next(request, context);
    }

private:
    const JwtComponent& jwt_component_;
};

class AuthMiddlewareFactory final : public userver::server::middlewares::HttpMiddlewareFactoryBase {
public:
    static constexpr std::string_view kName = AuthMiddleware::kName;

    AuthMiddlewareFactory(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    ) : HttpMiddlewareFactoryBase(config, context),
        jwt_component_(context.FindComponent<JwtComponent>()) {}

private:
    std::unique_ptr<userver::server::middlewares::HttpMiddlewareBase> Create(
        const userver::server::handlers::HttpHandlerBase&,
        userver::yaml_config::YamlConfig
    ) const override {
        return std::make_unique<AuthMiddleware>(jwt_component_);
    }

    const JwtComponent& jwt_component_;
};

}

template <>
inline constexpr bool userver::components::kHasValidate<common::AuthMiddlewareFactory> = true;

template <>
inline constexpr auto userver::components::kConfigFileMode<common::AuthMiddlewareFactory> = 
    userver::components::ConfigFileMode::kNotRequired;
