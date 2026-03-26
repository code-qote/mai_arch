#pragma once

#include <userver/components/loggable_component_base.hpp>
#include <userver/components/component_context.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/formats/json/value.hpp>

#include <common/jwt/jwt.hpp>

namespace common {

class JwtComponent final : public userver::components::LoggableComponentBase {
public:
    static constexpr std::string_view kName = "jwt-component";

    JwtComponent(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context)
        : LoggableComponentBase(config, context),
          jwt_manager_(GetSecretFromSecdist(context)) {}

    const JwtManager& GetManager() const { return jwt_manager_; }

private:
    static std::string GetSecretFromSecdist(const userver::components::ComponentContext& context) {
        
        auto& secdist = context.FindComponent<userver::components::Secdist>();
        
        auto snapshot = secdist.Get();
        
        auto json = snapshot.Get<userver::formats::json::Value>();
        
        return json["jwt_secret"].As<std::string>();
    }

    JwtManager jwt_manager_;
};

} // namespace common
