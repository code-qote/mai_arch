#pragma once

#include <userver/components/component_list.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/storages/mongo/pool.hpp>

namespace analytics_service {

class LogActivityHandler final : public userver::server::handlers::HttpHandlerJsonBase {
public:
    static constexpr std::string_view kName = "handler-log-activity";

    LogActivityHandler(const userver::components::ComponentConfig& config,
                      const userver::components::ComponentContext& context);

    userver::formats::json::Value HandleRequestJsonThrow(
        const userver::server::http::HttpRequest& request,
        const userver::formats::json::Value& json,
        userver::server::request::RequestContext&) const override;

private:
    userver::storages::mongo::PoolPtr mongo_pool_;
};

}  // namespace analytics_service
