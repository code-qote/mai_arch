#pragma once

#include "common/jwt/jwt.hpp"
#include <common/database/user.hpp>
#include <common/database/event.hpp>
#include <common/database/booking.hpp>

#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>

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

/// Pseudo in-memory database for storing users, events, and bookings.
/// Thread-safe via internal mutex.
/// Uses singleton pattern to share data across handlers.
class InMemoryDb {
public:
    /// Get the singleton instance
    static InMemoryDb& Instance() {
        static InMemoryDb instance;
        return instance;
    }

    // User operations
    UserResult RegisterUser(const std::string& username,
                            const std::string& email,
                            const std::string& first_name,
                            const std::string& last_name,
                            const std::string& password,
                            const std::string& role = common::kParticipantRole);
    UserResult LoginUser(const std::string& username, const std::string& password);

    /// Find users matching the filter criteria.
    /// Only non-empty filter fields are used for matching.
    /// Returns all matching users.
    std::vector<User> FindUsers(const UserFilter& filter) const;
    
    std::size_t UserCount() const;

    // Event operations
    EventResult CreateEvent(const std::string& name,
                            const GeoPosition& geo_position,
                            int places_count,
                            const std::string& event_time,
                            const std::string& organizer_id);

    /// Find events matching the filter criteria.
    /// Only non-empty filter fields are used for matching.
    /// Returns all matching events.
    std::vector<Event> FindEvents(const EventFilter& filter) const;

    std::size_t EventCount() const;

    BookingResult BookEvent(const std::string& event_id, const std::string& participant_id);
    BookingResult CancelBooking(const std::string& event_id, const std::string& participant_id);

    // Booking operations
    BookingDbResult CreateBooking(const std::string& event_id,
                                  const std::string& user_id,
                                  int places_count);
    BookingDbResult UpdateBookingStatus(const std::string& booking_id, BookingStatus status);
    BookingDbResult GetBooking(const std::string& booking_id) const;
    std::vector<Booking> FindBookings(const BookingFilter& filter) const;
    std::size_t BookingCount() const;

private:
    std::string GenerateId() const;
    std::string HashPassword(const std::string& password) const;
    std::string GetCurrentTimestamp() const;
    bool MatchesFilter(const User& user, const UserFilter& filter) const;
    bool MatchesFilter(const Event& event, const EventFilter& filter) const;
    bool MatchesFilter(const Booking& booking, const BookingFilter& filter) const;

    mutable std::mutex user_mutex_;
    mutable std::mutex event_mutex_;
    mutable std::mutex booking_mutex_;
    std::unordered_map<std::string, User> users_;
    std::unordered_map<std::string, Event> events_;
    std::unordered_map<std::string, Booking> bookings_;
};

}  // namespace common::database
