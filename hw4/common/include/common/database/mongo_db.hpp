#pragma once

#include "common/jwt/jwt.hpp"
#include <common/database/user.hpp>
#include <common/database/event.hpp>
#include <common/database/booking.hpp>

#include <userver/storages/mongo.hpp>
#include <userver/storages/mongo/component.hpp>

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

struct UserFilter {
    std::string id;
    std::string username;
    std::string email;
    std::string first_name;
    std::string last_name;
};

struct EventFilter {
    std::string id;
    std::string name;
    std::string organizer_id;
    std::string country;
    std::string city;
};

struct BookingFilter {
    std::string id;
    std::string event_id;
    std::string user_id;
    BookingStatus status = BookingStatus::kPending;
    bool filter_by_status = false;
};

class MongoDb {
public:
    explicit MongoDb(userver::storages::mongo::PoolPtr pool);

    UserResult RegisterUser(const std::string& username,
                            const std::string& email,
                            const std::string& first_name,
                            const std::string& last_name,
                            const std::string& password_hash,
                            const std::string& role) const;
    
    UserResult LoginUser(const std::string& username) const;
    
    std::vector<User> FindUsers(const UserFilter& filter) const;

    EventResult CreateEvent(const std::string& name,
                            const GeoPosition& geo_position,
                            int places_count,
                            const std::string& event_time,
                            const std::string& organizer_id) const;

    std::vector<Event> FindEvents(const EventFilter& filter) const;

    BookingResult ReservePlaces(const std::string& event_id, int places_count) const;
    BookingResult ReleasePlaces(const std::string& event_id, int places_count) const;

    BookingDbResult CreateBooking(const std::string& event_id,
                                  const std::string& user_id,
                                  int places_count) const;
    
    BookingDbResult UpdateBookingStatus(const std::string& booking_id, BookingStatus status) const;
    
    BookingDbResult GetBooking(const std::string& booking_id) const;
    
    std::vector<Booking> FindBookings(const BookingFilter& filter) const;

    std::optional<Booking> FindActiveBooking(const std::string& user_id,
                                              const std::string& event_id) const;

private:
    userver::storages::mongo::PoolPtr pool_;
};

}
