#pragma once

#include <userver/components/component_list.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/storages/postgres/component.hpp>

#include <common/database/db.hpp>
#include <common/jwt/jwt_component.hpp>

namespace event_service {

struct BookPlacesRequest {
    std::string event_id;
    int places_count;
};

struct BookPlacesResponse {
    bool success;
    common::database::Event event;
    std::string error;
};

class BookPlacesHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-book-places";

    BookPlacesHandler(const userver::components::ComponentConfig& config,
                      const userver::components::ComponentContext& context);

    std::string HandleRequest(userver::server::http::HttpRequest& request,
                              userver::server::request::RequestContext& context) const override;

private:
    common::database::PostgresDb db_;
};

}  // namespace event_service
