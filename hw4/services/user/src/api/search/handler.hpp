#pragma once

#include <userver/components/component_list.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/storages/postgres/component.hpp>

#include <common/database/db.hpp>
#include <common/database/user.hpp>
#include <common/middleware/auth_middleware.hpp>

namespace user_service {

struct SearchRequest {
    std::string id;
    std::string username;
    std::string email;
    std::string first_name;
    std::string last_name;

    common::database::UserFilter ToFilter() const {
        return {id, username, email, first_name, last_name};
    };
};

struct SearchResponse {
    std::vector<common::database::User> users;
};

class SearchHandler final : public userver::server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-search";

    SearchHandler(const userver::components::ComponentConfig& config,
                    const userver::components::ComponentContext& context);

    std::string HandleRequest(userver::server::http::HttpRequest& request,
                              userver::server::request::RequestContext& context) const override;

private:
    common::database::PostgresDb db_;
};

}  // namespace user_service
