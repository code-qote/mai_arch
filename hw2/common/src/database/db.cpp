#include "common/jwt/jwt.hpp"
#include <common/database/db.hpp>

#include <userver/crypto/hash.hpp>
#include <random>
#include <iomanip>
#include <sstream>

namespace common::database {

UserResult InMemoryDb::RegisterUser(const std::string& username,
                                        const std::string& email,
                                        const std::string& first_name,
                                        const std::string& last_name,
                                        const std::string& password,
                                        const std::string& role) {
    // Validate required fields
    if (username.empty() || email.empty() || password.empty()) {
        return {false, "Username, email, and password are required", {}};
    }

    std::lock_guard<std::mutex> lock(user_mutex_);

    // Check if user already exists
    for (const auto& [id, user] : users_) {
        if (user.username == username || user.email == email) {
            return {false, "User with this username or email already exists", {}};
        }
    }

    // Create new user
    User new_user;
    new_user.id = GenerateId();
    new_user.username = username;
    new_user.email = email;
    new_user.first_name = first_name;
    new_user.last_name = last_name;
    new_user.password_hash = HashPassword(password);
    new_user.role = role;
    new_user.created_at = GetCurrentTimestamp();

    users_[new_user.id] = new_user;

    return {true, "", new_user};
}

UserResult InMemoryDb::LoginUser(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(user_mutex_);

    // First try to find by username
    for (const auto& [id, user] : users_) {
        if (user.username == username) {
            if (HashPassword(password) == user.password_hash) {
                return {true, "", user};
            } else {
                return {false, "Incorrect password", {}};
            }
        }
    }

    return {false, "User not found", {}};
}

std::vector<User> InMemoryDb::FindUsers(const UserFilter& filter) const {
    std::lock_guard<std::mutex> lock(user_mutex_);
    std::vector<User> result;
    
    for (const auto& [id, user] : users_) {
        if (MatchesFilter(user, filter)) {
            result.push_back(user);
        }
    }
    
    return result;
}

bool InMemoryDb::MatchesFilter(const User& user, const UserFilter& filter) const {
    // Check each non-empty filter field
    if (!filter.id.empty() && user.id != filter.id) {
        return false;
    }
    if (!filter.username.empty() && user.username != filter.username) {
        return false;
    }
    if (!filter.email.empty() && user.email != filter.email) {
        return false;
    }
    if (!filter.first_name.empty() && user.first_name != filter.first_name) {
        return false;
    }
    if (!filter.last_name.empty() && user.last_name != filter.last_name) {
        return false;
    }
    return true;
}

std::size_t InMemoryDb::UserCount() const {
    std::lock_guard<std::mutex> lock(user_mutex_);
    return users_.size();
}

std::string InMemoryDb::GenerateId() const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static const char* hex_digits = "0123456789abcdef";

    std::string uuid(36, ' ');
    for (size_t i = 0; i < 36; ++i) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            uuid[i] = '-';
        } else {
            uuid[i] = hex_digits[dis(gen)];
        }
    }
    return uuid;
}

std::string InMemoryDb::HashPassword(const std::string& password) const {
    return userver::crypto::hash::Sha256(password);
}

std::string InMemoryDb::GetCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

EventResult InMemoryDb::CreateEvent(const std::string& name,
                                    const GeoPosition& geo_position,
                                    int places_count,
                                    const std::string& event_time,
                                    const std::string& organizer_id) {
    // Validate required fields
    if (name.empty() || places_count <= 0) {
        return {false, "Event name and positive places count are required", {}};
    }

    std::lock_guard<std::mutex> lock(event_mutex_);

    // Create new event
    Event new_event;
    new_event.id = GenerateId();
    new_event.name = name;
    new_event.geo_position = geo_position;
    new_event.places_count = places_count;
    new_event.organizer_id = organizer_id;
    new_event.event_time = event_time;
    new_event.created_at = GetCurrentTimestamp();

    events_[new_event.id] = new_event;

    return {true, "", new_event};
}

std::size_t InMemoryDb::EventCount() const {
    std::lock_guard<std::mutex> lock(event_mutex_);
    return events_.size();
}

std::vector<Event> InMemoryDb::FindEvents(const EventFilter& filter) const {
    std::lock_guard<std::mutex> lock(event_mutex_);
    std::vector<Event> result;
    
    for (const auto& [id, event] : events_) {
        if (MatchesFilter(event, filter)) {
            result.push_back(event);
        }
    }
    
    return result;
}

bool InMemoryDb::MatchesFilter(const Event& event, const EventFilter& filter) const {
    // Check each non-empty filter field
    if (!filter.id.empty() && event.id != filter.id) {
        return false;
    }
    if (!filter.name.empty() && event.name != filter.name) {
        return false;
    }
    if (!filter.organizer_id.empty() && event.organizer_id != filter.organizer_id) {
        return false;
    }
    if (!filter.country.empty() && event.geo_position.country != filter.country) {
        return false;
    }
    if (!filter.city.empty() && event.geo_position.city != filter.city) {
        return false;
    }
    return true;
}

BookingResult InMemoryDb::BookEvent(const std::string& event_id, const std::string& participant_id) {
    std::lock_guard<std::mutex> lock(event_mutex_);

    // Find the event
    auto it = events_.find(event_id);
    if (it == events_.end()) {
        return {false, "Event not found", {}};
    }

    Event& event = it->second;

    // Check if there are available places
    if (event.places_count <= 0) {
        return {false, "No available places for this event", {}};
    }

    // Decrement the places count
    event.places_count--;

    return {true, "", event};
}

BookingResult InMemoryDb::CancelBooking(const std::string& event_id, const std::string& participant_id) {
    std::lock_guard<std::mutex> lock(event_mutex_);

    // Find the event
    auto it = events_.find(event_id);
    if (it == events_.end()) {
        return {false, "Event not found", {}};
    }

    Event& event = it->second;

    // Increment the places count (release the booking)
    event.places_count++;

    return {true, "", event};
}

BookingDbResult InMemoryDb::CreateBooking(const std::string& event_id,
                                          const std::string& user_id,
                                          int places_count) {
    if (event_id.empty() || user_id.empty() || places_count <= 0) {
        return {false, "Event ID, user ID, and positive places count are required", {}};
    }

    std::lock_guard<std::mutex> lock(booking_mutex_);

    // Create new booking
    Booking new_booking;
    new_booking.id = GenerateId();
    new_booking.event_id = event_id;
    new_booking.user_id = user_id;
    new_booking.places_count = places_count;
    new_booking.status = BookingStatus::kPending;
    new_booking.created_at = GetCurrentTimestamp();
    new_booking.updated_at = new_booking.created_at;

    bookings_[new_booking.id] = new_booking;

    return {true, "", new_booking};
}

BookingDbResult InMemoryDb::UpdateBookingStatus(const std::string& booking_id, BookingStatus status) {
    std::lock_guard<std::mutex> lock(booking_mutex_);

    auto it = bookings_.find(booking_id);
    if (it == bookings_.end()) {
        return {false, "Booking not found", {}};
    }

    Booking& booking = it->second;
    booking.status = status;
    booking.updated_at = GetCurrentTimestamp();

    return {true, "", booking};
}

BookingDbResult InMemoryDb::GetBooking(const std::string& booking_id) const {
    std::lock_guard<std::mutex> lock(booking_mutex_);

    auto it = bookings_.find(booking_id);
    if (it == bookings_.end()) {
        return {false, "Booking not found", {}};
    }

    return {true, "", it->second};
}

std::vector<Booking> InMemoryDb::FindBookings(const BookingFilter& filter) const {
    std::lock_guard<std::mutex> lock(booking_mutex_);
    std::vector<Booking> result;
    
    for (const auto& [id, booking] : bookings_) {
        if (MatchesFilter(booking, filter)) {
            result.push_back(booking);
        }
    }
    
    return result;
}

bool InMemoryDb::MatchesFilter(const Booking& booking, const BookingFilter& filter) const {
    if (!filter.id.empty() && booking.id != filter.id) {
        return false;
    }
    if (!filter.event_id.empty() && booking.event_id != filter.event_id) {
        return false;
    }
    if (!filter.user_id.empty() && booking.user_id != filter.user_id) {
        return false;
    }
    if (filter.filter_by_status && booking.status != filter.status) {
        return false;
    }
    return true;
}

std::size_t InMemoryDb::BookingCount() const {
    std::lock_guard<std::mutex> lock(booking_mutex_);
    return bookings_.size();
}

}  // namespace common::database
