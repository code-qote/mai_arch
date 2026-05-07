#include <userver/components/minimal_server_component_list.hpp>
#include <userver/clients/dns/component.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/redis/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include "api/create/handler.hpp"
#include "api/list/handler.hpp"
#include "api/book/handler.hpp"
#include "api/cancel/handler.hpp"
#include "api/participants/handler.hpp"
#include "api/user_events/handler.hpp"
#include <common/jwt/jwt_component.hpp>
#include <common/middleware/auth_middleware.hpp>
#include <common/cache/redis_cache_component.hpp>
#include <common/cache/rate_limiter.hpp>

int main(int argc, char* argv[]) {
    auto component_list = userver::components::MinimalServerComponentList()
        .Append<userver::components::TestsuiteSupport>()
        .Append<userver::clients::dns::Component>()
        .Append<userver::components::DefaultSecdistProvider>()
        .Append<userver::components::Secdist>()
        .Append<userver::components::Postgres>("postgres-db")
        .Append<userver::components::Redis>("redis-cache-db")
        .Append<common::cache::RedisCacheComponent>()
        .Append<common::cache::RateLimiterComponent>()
        .Append<common::JwtComponent>()
        .Append<common::AuthMiddlewareFactory>()
        .Append<event_service::CreateEventHandler>()
        .Append<event_service::ListEventsHandler>()
        .Append<event_service::BookPlacesHandler>()
        .Append<event_service::CancelPlacesHandler>()
        .Append<event_service::GetParticipantsHandler>()
        .Append<event_service::GetUserEventsHandler>();

    return userver::utils::DaemonMain(argc, argv, component_list);
}
