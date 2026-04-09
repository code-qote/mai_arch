#include "handler.hpp"

#include <userver/formats/json.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/server/http/http_status.hpp>

#include <common/middleware/auth_middleware.hpp>

namespace user_service {

SearchHandler::SearchHandler(const userver::components::ComponentConfig& config,
                             const userver::components::ComponentContext& context)
    : userver::server::handlers::HttpHandlerBase(config, context),
      db_(context.FindComponent<userver::components::Postgres>("postgres-db").GetCluster()) {}

std::string SearchHandler::HandleRequest(
    userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context
) const {
    request.GetHttpResponse().SetContentType(userver::http::content_type::kApplicationJson);

    if (request.GetMethod() != userver::server::http::HttpMethod::kPost) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kMethodNotAllowed);
        return userver::formats::json::ToString(userver::formats::json::MakeObject(
            "error", "Method not allowed. Use POST for search."
        ));
    }

    auto role = context.GetData<std::string>(common::kRoleKey);
    if (role != common::kAdminRole) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
        return userver::formats::json::ToString(userver::formats::json::MakeObject(
            "error", "Access denied. Only admin can search users."
        ));
    }

    try {
        const auto request_body = userver::formats::json::FromString(request.RequestBody());

        SearchRequest search_request;
        search_request.id = request_body["id"].As<std::string>("");
        search_request.username = request_body["username"].As<std::string>("");
        search_request.email = request_body["email"].As<std::string>("");
        search_request.first_name = request_body["first_name"].As<std::string>("");
        search_request.last_name = request_body["last_name"].As<std::string>("");

        auto users = db_.FindUsers(search_request.ToFilter());

        userver::formats::json::ValueBuilder response_builder;
        userver::formats::json::ValueBuilder users_array(userver::formats::common::Type::kArray);
        for (const auto& user : users) {
            users_array.PushBack(common::database::Serialize(
                user,
                userver::formats::serialize::To<userver::formats::json::Value>{}
            ));
        }
        response_builder["users"] = users_array.ExtractValue();

        return userver::formats::json::ToString(response_builder.ExtractValue());

    } catch (const std::exception& ex) {
        request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
        return userver::formats::json::ToString(userver::formats::json::MakeObject(
            "error", std::string("Invalid request format: ") + ex.what()
        ));
    }
}

}  // namespace user_service
