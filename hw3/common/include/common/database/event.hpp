#pragma once

#include <string>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/serialize/common_containers.hpp>

namespace common::database {

struct GeoPosition {
    std::string country;
    std::string city;
    std::string street;
};

struct Event {
    std::string id;
    std::string name;
    GeoPosition geo_position;
    int places_count;
    std::string organizer_id;
    std::string event_time;
    std::string created_at;
};

// JSON serialization for GeoPosition
inline userver::formats::json::Value Serialize(
    const GeoPosition& geo,
    userver::formats::serialize::To<userver::formats::json::Value>) {
    userver::formats::json::ValueBuilder builder;
    builder["country"] = geo.country;
    builder["city"] = geo.city;
    builder["street"] = geo.street;
    return builder.ExtractValue();
}

// JSON serialization for Event
inline userver::formats::json::Value Serialize(
    const Event& event,
    userver::formats::serialize::To<userver::formats::json::Value>) {
    userver::formats::json::ValueBuilder builder;
    builder["id"] = event.id;
    builder["name"] = event.name;
    builder["geo_position"] = Serialize(event.geo_position, 
        userver::formats::serialize::To<userver::formats::json::Value>{});
    builder["places_count"] = event.places_count;
    builder["organizer_id"] = event.organizer_id;
    builder["event_time"] = event.event_time;
    builder["created_at"] = event.created_at;
    return builder.ExtractValue();
}

}  // namespace common::database
