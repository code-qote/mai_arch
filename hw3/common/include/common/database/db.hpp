#pragma once

#include "common/jwt/jwt.hpp"
#include <common/database/user.hpp>
#include <common/database/event.hpp>
#include <common/database/booking.hpp>

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

#include <string>
#include <vector>
#include <optional>

namespace common::database {

struct UserResult {
    bool success;
    std::string error;
    User user;
};

struct EventResult {
    bool success;
    std::string error;
    Event event;
};

struct BookingResult {
    bool success;
    std::string error;
    Event event;
};

struct BookingDbResult {
    bool success;
    std::string error;
    Booking booking;
};

/// Filter struct for searching users.
/// Empty fields are ignored in the search.
struct UserFilter {
    std::string id;
    std::string username;
    std::string email;
    std::string first_name;
    std::string last_name;
};

/// Filter struct for searching events.
/// Empty fields are ignored in the search.
struct EventFilter {
    std::string id;
    std::string name;
    std::string organizer_id;
    std::string country;
    std::string city;
};

/// Filter struct for searching bookings.
/// Empty fields are ignored in the search.
struct BookingFilter {
    std::string id;
    std::string event_id;
    std::string user_id;
    BookingStatus status = BookingStatus::kPending;
    bool filter_by_status = false;
};

/// PostgreSQL database wrapper for the Event Booking System.
/// Uses userver's PostgreSQL driver for async database operations.
class PostgresDb {
public:
    explicit PostgresDb(userver::storages::postgres::ClusterPtr cluster);

    // User operations
    UserResult RegisterUser(const std::string& username,
                            const std::string& email,
                            const std::string& first_name,
                            const std::string& last_name,
                            const std::string& password_hash,
                            const std::string& role) const;
    
    UserResult LoginUser(const std::string& username) const;
    
    std::vector<User> FindUsers(const UserFilter& filter) const;

    // Event operations
    EventResult CreateEvent(const std::string& name,
                            const GeoPosition& geo_position,
                            int places_count,
                            const std::string& event_time,
                            const std::string& organizer_id) const;

    std::vector<Event> FindEvents(const EventFilter& filter) const;

    BookingResult ReservePlaces(const std::string& event_id, int places_count) const;
    BookingResult ReleasePlaces(const std::string& event_id, int places_count) const;

    // Booking operations
    BookingDbResult CreateBooking(const std::string& event_id,
                                  const std::string& user_id,
                                  int places_count) const;
    
    BookingDbResult UpdateBookingStatus(const std::string& booking_id, BookingStatus status) const;
    
    BookingDbResult GetBooking(const std::string& booking_id) const;
    
    std::vector<Booking> FindBookings(const BookingFilter& filter) const;

    std::optional<Booking> FindActiveBooking(const std::string& user_id,
                                              const std::string& event_id) const;

private:
    userver::storages::postgres::ClusterPtr cluster_;
};

}  // namespace common::database
