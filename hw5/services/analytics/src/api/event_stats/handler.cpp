#include "handler.hpp"

#include <common/database/analytics_mongo.hpp>

#include <userver/components/component_context.hpp>
#include <userver/formats/json.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/storages/mongo/component.hpp>
#include <userver/utils/datetime.hpp>

namespace analytics_service {

namespace {

namespace formats = userver::formats;

std::string TimePointToString(const std::chrono::system_clock::time_point& tp) {
    return userver::utils::datetime::Timestring(tp, "UTC", "%Y-%m-%d %H:%M:%S");
}

}

EventStatsHandler::EventStatsHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerJsonBase(config, context),
      mongo_pool_(context.FindComponent<userver::components::Mongo>("analytics-mongo").GetPool()) {}

formats::json::Value EventStatsHandler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest& request,
    const formats::json::Value& json,
    userver::server::request::RequestContext&) const {
    
    common::database::AnalyticsMongo analytics_db(mongo_pool_);

    if (request.GetMethod() == userver::server::http::HttpMethod::kPost) {
        try {
            common::database::EventStatistics stats;
            stats.event_id = json["event_id"].As<std::string>();
            stats.event_name = json["event_name"].As<std::string>("");
            stats.organizer_id = json["organizer_id"].As<std::string>("");
            stats.total_bookings = json["total_bookings"].As<int>(0);
            stats.total_participants = json["total_participants"].As<int>(0);
            stats.total_places_booked = json["total_places_booked"].As<int>(0);
            stats.total_places_available = json["total_places_available"].As<int>(0);
            stats.occupancy_rate = json["occupancy_rate"].As<double>(0.0);
            stats.cancelled_bookings = json["cancelled_bookings"].As<int>(0);
            stats.cancellation_rate = json["cancellation_rate"].As<double>(0.0);
            stats.average_places_per_booking = json["average_places_per_booking"].As<double>(0.0);
            stats.total_revenue = json["total_revenue"].As<double>(0.0);
            stats.created_at = std::chrono::system_clock::now();
            stats.updated_at = std::chrono::system_clock::now();

            auto result = analytics_db.UpdateEventStatistics(stats);

            if (!result.success) {
                request.SetResponseStatus(userver::server::http::HttpStatus::kInternalServerError);
                return formats::json::MakeObject("error", result.error);
            }

            request.SetResponseStatus(userver::server::http::HttpStatus::kOk);
            return formats::json::MakeObject(
                "success", true,
                "message", "Statistics updated successfully"
            );

        } catch (const std::exception& e) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return formats::json::MakeObject(
                "error", std::string("Invalid request format: ") + e.what()
            );
        }
    }
    
    else if (request.GetMethod() == userver::server::http::HttpMethod::kGet) {
        try {
            if (json.HasMember("top") && json["top"].As<bool>(false)) {
                int limit = json["limit"].As<int>(10);
                auto top_events = analytics_db.GetTopEventsByOccupancy(limit);

                auto events_array = formats::json::ValueBuilder(formats::json::Type::kArray);
                for (const auto& stats : top_events) {
                    events_array.PushBack(formats::json::MakeObject(
                        "event_id", stats.event_id,
                        "event_name", stats.event_name,
                        "organizer_id", stats.organizer_id,
                        "total_bookings", stats.total_bookings,
                        "total_participants", stats.total_participants,
                        "total_places_booked", stats.total_places_booked,
                        "total_places_available", stats.total_places_available,
                        "occupancy_rate", stats.occupancy_rate,
                        "cancelled_bookings", stats.cancelled_bookings,
                        "cancellation_rate", stats.cancellation_rate,
                        "average_places_per_booking", stats.average_places_per_booking,
                        "total_revenue", stats.total_revenue,
                        "created_at", TimePointToString(stats.created_at),
                        "updated_at", TimePointToString(stats.updated_at)
                    ));
                }

                return formats::json::MakeObject("events", events_array.ExtractValue());
            }
            
            std::string event_id = json["event_id"].As<std::string>();
            auto stats_opt = analytics_db.GetEventStatistics(event_id);

            if (!stats_opt) {
                request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
                return formats::json::MakeObject("error", "Statistics not found for this event");
            }

            const auto& stats = *stats_opt;
            return formats::json::MakeObject(
                "event_id", stats.event_id,
                "event_name", stats.event_name,
                "organizer_id", stats.organizer_id,
                "total_bookings", stats.total_bookings,
                "total_participants", stats.total_participants,
                "total_places_booked", stats.total_places_booked,
                "total_places_available", stats.total_places_available,
                "occupancy_rate", stats.occupancy_rate,
                "cancelled_bookings", stats.cancelled_bookings,
                "cancellation_rate", stats.cancellation_rate,
                "average_places_per_booking", stats.average_places_per_booking,
                "total_revenue", stats.total_revenue,
                "created_at", TimePointToString(stats.created_at),
                "updated_at", TimePointToString(stats.updated_at)
            );

        } catch (const std::exception& e) {
            request.SetResponseStatus(userver::server::http::HttpStatus::kBadRequest);
            return formats::json::MakeObject(
                "error", std::string("Invalid request format: ") + e.what()
            );
        }
    }
    
    request.SetResponseStatus(userver::server::http::HttpStatus::kMethodNotAllowed);
    return formats::json::MakeObject("error", "Method not allowed. Use POST or GET.");
}

}
