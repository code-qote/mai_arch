#pragma once

#include <userver/components/component_list.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/formats/json/value.hpp>

#include <common/database/db.hpp>
#include <common/database/event.hpp>

namespace event_service {

struct ListEventsRequest {
    std::string id;
    std::string name;
    std::string organizer_id;
    std::string country;
    std::string city;

    common::database::EventFilter ToFilter() const {
        return {id, name, organizer_id, country, city};
    };
};

struct ListEventsResponse {
    std::vector<common::database::Event> events;
};

ListEventsResponse ProcessListEvents(
    const ListEventsRequest& request,
    common::database::InMemoryDb& db);

class ListEventsHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-list-events";

    ListEventsHandler(const userver::components::ComponentConfig& config,
                      const userver::components::ComponentContext& context);

    std::string HandleRequest(userver::server::http::HttpRequest& request,
                              userver::server::request::RequestContext& context) const override;
};

}  // namespace event_service
