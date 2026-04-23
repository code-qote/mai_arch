#include <userver/components/minimal_server_component_list.hpp>
#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component_list.hpp>
#include <userver/storages/mongo/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include "src/api/log_activity/handler.hpp"
#include "src/api/record_search/handler.hpp"
#include "src/api/event_stats/handler.hpp"

int main(int argc, char* argv[]) {
    auto component_list = userver::components::MinimalServerComponentList()
        .Append<userver::components::TestsuiteSupport>()
        .AppendComponentList(userver::clients::http::ComponentList())
        .Append<userver::clients::dns::Component>()
        .Append<userver::components::Mongo>("analytics-mongo")
        .Append<analytics_service::LogActivityHandler>()
        .Append<analytics_service::RecordSearchHandler>()
        .Append<analytics_service::EventStatsHandler>();

    return userver::utils::DaemonMain(argc, argv, component_list);
}
