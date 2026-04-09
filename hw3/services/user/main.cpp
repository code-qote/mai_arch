#include <userver/components/minimal_server_component_list.hpp>
#include <userver/clients/dns/component.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include "api/register/handler.hpp"
#include "api/login/handler.hpp"
#include "api/search/handler.hpp"
#include <common/jwt/jwt_component.hpp>
#include <common/middleware/auth_middleware.hpp>

int main(int argc, char* argv[]) {
    auto component_list = userver::components::MinimalServerComponentList()
        .Append<userver::components::TestsuiteSupport>()
        .Append<userver::clients::dns::Component>()
        .Append<userver::components::DefaultSecdistProvider>()
        .Append<userver::components::Secdist>()
        .Append<userver::components::Postgres>("postgres-db")
        .Append<common::JwtComponent>()
        .Append<common::AuthMiddlewareFactory>()
        .Append<user_service::RegisterHandler>()
        .Append<user_service::LoginHandler>()
        .Append<user_service::SearchHandler>();

    return userver::utils::DaemonMain(argc, argv, component_list);
}
