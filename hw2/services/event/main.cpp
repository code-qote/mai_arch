#include <userver/components/minimal_server_component_list.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/utils/daemon_run.hpp>

#include "api/create/handler.hpp"
#include "api/list/handler.hpp"
#include "api/book/handler.hpp"
#include "api/cancel/handler.hpp"
#include <common/jwt/jwt_component.hpp>
#include <common/middleware/auth_middleware.hpp>

int main(int argc, char* argv[]) {
    auto component_list = userver::components::MinimalServerComponentList()
        .Append<userver::components::DefaultSecdistProvider>()
        .Append<userver::components::Secdist>()
        .Append<common::JwtComponent>()
        .Append<common::AuthMiddlewareFactory>()
        .Append<event_service::CreateEventHandler>()
        .Append<event_service::ListEventsHandler>()
        .Append<event_service::BookPlacesHandler>()
        .Append<event_service::CancelPlacesHandler>();

    return userver::utils::DaemonMain(argc, argv, component_list);
}
