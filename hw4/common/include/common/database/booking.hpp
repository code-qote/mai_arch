#pragma once

#include <string>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/serialize/common_containers.hpp>

namespace common::database {

enum class BookingStatus {
    kPending,
    kConfirmed,
    kCancelled,
    kFailed
};

inline std::string BookingStatusToString(BookingStatus status) {
    switch (status) {
        case BookingStatus::kPending: return "pending";
        case BookingStatus::kConfirmed: return "confirmed";
        case BookingStatus::kCancelled: return "cancelled";
        case BookingStatus::kFailed: return "failed";
    }
    return "unknown";
}

inline BookingStatus BookingStatusFromString(const std::string& status) {
    if (status == "pending") return BookingStatus::kPending;
    if (status == "confirmed") return BookingStatus::kConfirmed;
    if (status == "cancelled") return BookingStatus::kCancelled;
    if (status == "failed") return BookingStatus::kFailed;
    return BookingStatus::kPending;
}

struct Booking {
    std::string id;
    std::string event_id;
    std::string user_id;
    int places_count;
    BookingStatus status;
    std::string created_at;
    std::string updated_at;
};

inline userver::formats::json::Value Serialize(
    const Booking& booking,
    userver::formats::serialize::To<userver::formats::json::Value>) {
    userver::formats::json::ValueBuilder builder;
    builder["id"] = booking.id;
    builder["event_id"] = booking.event_id;
    builder["user_id"] = booking.user_id;
    builder["places_count"] = booking.places_count;
    builder["status"] = BookingStatusToString(booking.status);
    builder["created_at"] = booking.created_at;
    builder["updated_at"] = booking.updated_at;
    return builder.ExtractValue();
}

}
