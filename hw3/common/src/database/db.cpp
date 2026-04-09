#include "common/database/db.hpp"

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/query.hpp>

namespace common::database {

namespace {

const userver::storages::postgres::Query kInsertUser{
    "INSERT INTO users (username, email, first_name, last_name, password_hash, role) "
    "VALUES ($1, $2, $3, $4, $5, $6) "
    "RETURNING id::text, username, email, first_name, last_name, role, created_at::text",
    userver::storages::postgres::Query::Name{"insert_user"}
};

const userver::storages::postgres::Query kSelectUserByUsername{
    "SELECT id::text, username, email, first_name, last_name, password_hash, role, created_at::text "
    "FROM users WHERE username = $1",
    userver::storages::postgres::Query::Name{"select_user_by_username"}
};

const userver::storages::postgres::Query kSelectAllUsers{
    "SELECT id::text, username, email, first_name, last_name, role, created_at::text "
    "FROM users ORDER BY created_at DESC",
    userver::storages::postgres::Query::Name{"select_all_users"}
};

const userver::storages::postgres::Query kSelectUserById{
    "SELECT id::text, username, email, first_name, last_name, role, created_at::text "
    "FROM users WHERE id = $1::uuid",
    userver::storages::postgres::Query::Name{"select_user_by_id"}
};

const userver::storages::postgres::Query kSearchUsers{
    "SELECT id::text, username, email, first_name, last_name, role, created_at::text "
    "FROM users "
    "WHERE ($1::uuid IS NULL OR id = $1::uuid) "
    "AND ($2::text IS NULL OR username ILIKE $2 || '%') "
    "AND ($3::text IS NULL OR email ILIKE $3 || '%') "
    "AND ($4::text IS NULL OR first_name ILIKE $4 || '%') "
    "AND ($5::text IS NULL OR last_name ILIKE $5 || '%') "
    "ORDER BY created_at DESC",
    userver::storages::postgres::Query::Name{"search_users"}
};

const userver::storages::postgres::Query kInsertEvent{
    "INSERT INTO events (name, country, city, street, places_count, available_places, organizer_id, event_time) "
    "VALUES ($1, $2, $3, $4, $5, $5, $6::uuid, $7::timestamptz) "
    "RETURNING id::text, name, country, city, street, places_count, available_places, organizer_id::text, event_time::text, created_at::text",
    userver::storages::postgres::Query::Name{"insert_event"}
};

const userver::storages::postgres::Query kSelectAllEvents{
    "SELECT id::text, name, country, city, street, places_count, available_places, organizer_id::text, event_time::text, created_at::text "
    "FROM events ORDER BY event_time ASC",
    userver::storages::postgres::Query::Name{"select_all_events"}
};

const userver::storages::postgres::Query kSearchEvents{
    "SELECT id::text, name, country, city, street, places_count, available_places, organizer_id::text, event_time::text, created_at::text "
    "FROM events "
    "WHERE ($1::uuid IS NULL OR id = $1::uuid) "
    "AND ($2::text IS NULL OR name ILIKE '%' || $2 || '%') "
    "AND ($3::uuid IS NULL OR organizer_id = $3::uuid) "
    "AND ($4::text IS NULL OR country = $4) "
    "AND ($5::text IS NULL OR city = $5) "
    "ORDER BY event_time ASC",
    userver::storages::postgres::Query::Name{"search_events"}
};

const userver::storages::postgres::Query kReservePlaces{
    "UPDATE events SET available_places = available_places - $2 "
    "WHERE id = $1::uuid AND available_places >= $2 "
    "RETURNING id::text, name, country, city, street, places_count, available_places, organizer_id::text, event_time::text, created_at::text",
    userver::storages::postgres::Query::Name{"reserve_places"}
};

const userver::storages::postgres::Query kReleasePlaces{
    "UPDATE events SET available_places = available_places + $2 "
    "WHERE id = $1::uuid AND available_places + $2 <= places_count "
    "RETURNING id::text, name, country, city, street, places_count, available_places, organizer_id::text, event_time::text, created_at::text",
    userver::storages::postgres::Query::Name{"release_places"}
};

const userver::storages::postgres::Query kInsertBooking{
    "INSERT INTO bookings (event_id, user_id, places_count, status) "
    "VALUES ($1::uuid, $2::uuid, $3, 'pending') "
    "RETURNING id::text, event_id::text, user_id::text, places_count, status, created_at::text",
    userver::storages::postgres::Query::Name{"insert_booking"}
};

const userver::storages::postgres::Query kUpdateBookingStatus{
    "UPDATE bookings SET status = $2 "
    "WHERE id = $1::uuid "
    "RETURNING id::text, event_id::text, user_id::text, places_count, status, created_at::text",
    userver::storages::postgres::Query::Name{"update_booking_status"}
};

const userver::storages::postgres::Query kSelectBookingById{
    "SELECT id::text, event_id::text, user_id::text, places_count, status, created_at::text "
    "FROM bookings WHERE id = $1::uuid",
    userver::storages::postgres::Query::Name{"select_booking_by_id"}
};

const userver::storages::postgres::Query kSelectActiveBooking{
    "SELECT id::text, event_id::text, user_id::text, places_count, status, created_at::text "
    "FROM bookings "
    "WHERE user_id = $1::uuid AND event_id = $2::uuid AND status IN ('pending', 'confirmed')",
    userver::storages::postgres::Query::Name{"select_active_booking"}
};

const userver::storages::postgres::Query kSearchBookings{
    "SELECT id::text, event_id::text, user_id::text, places_count, status, created_at::text "
    "FROM bookings "
    "WHERE ($1::uuid IS NULL OR id = $1::uuid) "
    "AND ($2::uuid IS NULL OR event_id = $2::uuid) "
    "AND ($3::uuid IS NULL OR user_id = $3::uuid) "
    "ORDER BY created_at DESC",
    userver::storages::postgres::Query::Name{"search_bookings"}
};

std::optional<std::string> NullIfEmpty(const std::string& str) {
    if (str.empty()) {
        return std::nullopt;
    }
    return str;
}

BookingStatus ParseBookingStatus(const std::string& status) {
    if (status == "pending") return BookingStatus::kPending;
    if (status == "confirmed") return BookingStatus::kConfirmed;
    if (status == "cancelled") return BookingStatus::kCancelled;
    if (status == "failed") return BookingStatus::kFailed;
    return BookingStatus::kPending;
}


}  // namespace

PostgresDb::PostgresDb(userver::storages::postgres::ClusterPtr cluster)
    : cluster_(std::move(cluster)) {}

UserResult PostgresDb::RegisterUser(
    const std::string& username,
    const std::string& email,
    const std::string& first_name,
    const std::string& last_name,
    const std::string& password_hash,
    const std::string& role) const {
    
    UserResult result;
    
    try {
        auto res = cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            kInsertUser,
            username, email, first_name, last_name, password_hash, role
        );
        
        if (res.IsEmpty()) {
            result.success = false;
            result.error = "Failed to create user";
            return result;
        }
        
        auto row = res[0];
        result.user.id = row["id"].As<std::string>();
        result.user.username = row["username"].As<std::string>();
        result.user.email = row["email"].As<std::string>();
        result.user.first_name = row["first_name"].As<std::string>();
        result.user.last_name = row["last_name"].As<std::string>();
        result.user.role = row["role"].As<std::string>();
        result.user.created_at = row["created_at"].As<std::string>();
        result.success = true;
        
    } catch (const userver::storages::postgres::UniqueViolation& ex) {
        result.success = false;
        if (std::string(ex.what()).find("username") != std::string::npos) {
            result.error = "User with this username already exists";
        } else if (std::string(ex.what()).find("email") != std::string::npos) {
            result.error = "User with this email already exists";
        } else {
            result.error = "User already exists";
        }
    } catch (const std::exception& ex) {
        result.success = false;
        result.error = std::string("Database error: ") + ex.what();
    }
    
    return result;
}

UserResult PostgresDb::LoginUser(const std::string& username) const {
    UserResult result;
    
    try {
        auto res = cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            kSelectUserByUsername,
            username
        );
        
        if (res.IsEmpty()) {
            result.success = false;
            result.error = "User not found";
            return result;
        }
        
        auto row = res[0];
        result.user.id = row["id"].As<std::string>();
        result.user.username = row["username"].As<std::string>();
        result.user.email = row["email"].As<std::string>();
        result.user.first_name = row["first_name"].As<std::string>();
        result.user.last_name = row["last_name"].As<std::string>();
        result.user.password_hash = row["password_hash"].As<std::string>();
        result.user.role = row["role"].As<std::string>();
        result.user.created_at = row["created_at"].As<std::string>();
        result.success = true;
        
    } catch (const std::exception& ex) {
        result.success = false;
        result.error = std::string("Database error: ") + ex.what();
    }
    
    return result;
}

std::vector<User> PostgresDb::FindUsers(const UserFilter& filter) const {
    std::vector<User> users;
    
    try {
        auto res = cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            kSearchUsers,
            NullIfEmpty(filter.id),
            NullIfEmpty(filter.username),
            NullIfEmpty(filter.email),
            NullIfEmpty(filter.first_name),
            NullIfEmpty(filter.last_name)
        );
        
        for (const auto& row : res) {
            User user;
            user.id = row["id"].As<std::string>();
            user.username = row["username"].As<std::string>();
            user.email = row["email"].As<std::string>();
            user.first_name = row["first_name"].As<std::string>();
            user.last_name = row["last_name"].As<std::string>();
            user.role = row["role"].As<std::string>();
            user.created_at = row["created_at"].As<std::string>();
            users.push_back(std::move(user));
        }
        
    } catch (const std::exception& ex) {
    }
    
    return users;
}

EventResult PostgresDb::CreateEvent(
    const std::string& name,
    const GeoPosition& geo_position,
    int places_count,
    const std::string& event_time,
    const std::string& organizer_id) const {
    
    EventResult result;
    
    try {
        auto res = cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            kInsertEvent,
            name, geo_position.country, geo_position.city, geo_position.street,
            places_count, organizer_id, event_time
        );
        
        if (res.IsEmpty()) {
            result.success = false;
            result.error = "Failed to create event";
            return result;
        }
        
        auto row = res[0];
        result.event.id = row["id"].As<std::string>();
        result.event.name = row["name"].As<std::string>();
        result.event.geo_position.country = row["country"].As<std::string>();
        result.event.geo_position.city = row["city"].As<std::string>();
        result.event.geo_position.street = row["street"].As<std::string>();
        result.event.places_count = row["available_places"].As<int>();
        result.event.organizer_id = row["organizer_id"].As<std::string>();
        result.event.event_time = row["event_time"].As<std::string>();
        result.event.created_at = row["created_at"].As<std::string>();
        result.success = true;
        
    } catch (const std::exception& ex) {
        result.success = false;
        result.error = std::string("Database error: ") + ex.what();
    }
    
    return result;
}

std::vector<Event> PostgresDb::FindEvents(const EventFilter& filter) const {
    std::vector<Event> events;
    
    try {
        auto res = cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            kSearchEvents,
            NullIfEmpty(filter.id),
            NullIfEmpty(filter.name),
            NullIfEmpty(filter.organizer_id),
            NullIfEmpty(filter.country),
            NullIfEmpty(filter.city)
        );
        
        for (const auto& row : res) {
            Event event;
            event.id = row["id"].As<std::string>();
            event.name = row["name"].As<std::string>();
            event.geo_position.country = row["country"].As<std::string>();
            event.geo_position.city = row["city"].As<std::string>();
            event.geo_position.street = row["street"].As<std::string>();
            event.places_count = row["available_places"].As<int>();
            event.organizer_id = row["organizer_id"].As<std::string>();
            event.event_time = row["event_time"].As<std::string>();
            event.created_at = row["created_at"].As<std::string>();
            events.push_back(std::move(event));
        }
        
    } catch (const std::exception& ex) {
    }
    
    return events;
}

BookingResult PostgresDb::ReservePlaces(const std::string& event_id, int places_count) const {
    BookingResult result;
    
    try {
        auto res = cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            kReservePlaces,
            event_id, places_count
        );
        
        if (res.IsEmpty()) {
            result.success = false;
            result.error = "Not enough available places or event not found";
            return result;
        }
        
        auto row = res[0];
        result.event.id = row["id"].As<std::string>();
        result.event.name = row["name"].As<std::string>();
        result.event.geo_position.country = row["country"].As<std::string>();
        result.event.geo_position.city = row["city"].As<std::string>();
        result.event.geo_position.street = row["street"].As<std::string>();
        result.event.places_count = row["available_places"].As<int>();
        result.event.organizer_id = row["organizer_id"].As<std::string>();
        result.event.event_time = row["event_time"].As<std::string>();
        result.event.created_at = row["created_at"].As<std::string>();
        result.success = true;
        
    } catch (const std::exception& ex) {
        result.success = false;
        result.error = std::string("Database error: ") + ex.what();
    }
    
    return result;
}

BookingResult PostgresDb::ReleasePlaces(const std::string& event_id, int places_count) const {
    BookingResult result;
    
    try {
        auto res = cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            kReleasePlaces,
            event_id, places_count
        );
        
        if (res.IsEmpty()) {
            result.success = false;
            result.error = "Event not found or invalid places count";
            return result;
        }
        
        auto row = res[0];
        result.event.id = row["id"].As<std::string>();
        result.event.name = row["name"].As<std::string>();
        result.event.geo_position.country = row["country"].As<std::string>();
        result.event.geo_position.city = row["city"].As<std::string>();
        result.event.geo_position.street = row["street"].As<std::string>();
        result.event.places_count = row["available_places"].As<int>();
        result.event.organizer_id = row["organizer_id"].As<std::string>();
        result.event.event_time = row["event_time"].As<std::string>();
        result.event.created_at = row["created_at"].As<std::string>();
        result.success = true;
        
    } catch (const std::exception& ex) {
        result.success = false;
        result.error = std::string("Database error: ") + ex.what();
    }
    
    return result;
}

BookingDbResult PostgresDb::CreateBooking(
    const std::string& event_id,
    const std::string& user_id,
    int places_count) const {
    
    BookingDbResult result;
    
    try {
        auto res = cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            kInsertBooking,
            event_id, user_id, places_count
        );
        
        if (res.IsEmpty()) {
            result.success = false;
            result.error = "Failed to create booking";
            return result;
        }
        
        auto row = res[0];
        result.booking.id = row["id"].As<std::string>();
        result.booking.event_id = row["event_id"].As<std::string>();
        result.booking.user_id = row["user_id"].As<std::string>();
        result.booking.places_count = row["places_count"].As<int>();
        result.booking.status = ParseBookingStatus(row["status"].As<std::string>());
        result.booking.created_at = row["created_at"].As<std::string>();
        result.success = true;
        
    } catch (const std::exception& ex) {
        result.success = false;
        result.error = std::string("Database error: ") + ex.what();
    }
    
    return result;
}

BookingDbResult PostgresDb::UpdateBookingStatus(const std::string& booking_id, BookingStatus status) const {
    BookingDbResult result;
    
    try {
        auto res = cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            kUpdateBookingStatus,
            booking_id, BookingStatusToString(status)
        );
        
        if (res.IsEmpty()) {
            result.success = false;
            result.error = "Booking not found";
            return result;
        }
        
        auto row = res[0];
        result.booking.id = row["id"].As<std::string>();
        result.booking.event_id = row["event_id"].As<std::string>();
        result.booking.user_id = row["user_id"].As<std::string>();
        result.booking.places_count = row["places_count"].As<int>();
        result.booking.status = ParseBookingStatus(row["status"].As<std::string>());
        result.booking.created_at = row["created_at"].As<std::string>();
        result.success = true;
        
    } catch (const std::exception& ex) {
        result.success = false;
        result.error = std::string("Database error: ") + ex.what();
    }
    
    return result;
}

BookingDbResult PostgresDb::GetBooking(const std::string& booking_id) const {
    BookingDbResult result;
    
    try {
        auto res = cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            kSelectBookingById,
            booking_id
        );
        
        if (res.IsEmpty()) {
            result.success = false;
            result.error = "Booking not found";
            return result;
        }
        
        auto row = res[0];
        result.booking.id = row["id"].As<std::string>();
        result.booking.event_id = row["event_id"].As<std::string>();
        result.booking.user_id = row["user_id"].As<std::string>();
        result.booking.places_count = row["places_count"].As<int>();
        result.booking.status = ParseBookingStatus(row["status"].As<std::string>());
        result.booking.created_at = row["created_at"].As<std::string>();
        result.success = true;
        
    } catch (const std::exception& ex) {
        result.success = false;
        result.error = std::string("Database error: ") + ex.what();
    }
    
    return result;
}

std::vector<Booking> PostgresDb::FindBookings(const BookingFilter& filter) const {
    std::vector<Booking> bookings;
    
    try {
        auto res = cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            kSearchBookings,
            NullIfEmpty(filter.id),
            NullIfEmpty(filter.event_id),
            NullIfEmpty(filter.user_id)
        );
        
        for (const auto& row : res) {
            Booking booking;
            booking.id = row["id"].As<std::string>();
            booking.event_id = row["event_id"].As<std::string>();
            booking.user_id = row["user_id"].As<std::string>();
            booking.places_count = row["places_count"].As<int>();
            booking.status = ParseBookingStatus(row["status"].As<std::string>());
            booking.created_at = row["created_at"].As<std::string>();
            bookings.push_back(std::move(booking));
        }
        
    } catch (const std::exception& ex) {
    }
    
    return bookings;
}

std::optional<Booking> PostgresDb::FindActiveBooking(
    const std::string& user_id, 
    const std::string& event_id) const {
    
    try {
        auto res = cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            kSelectActiveBooking,
            user_id, event_id
        );
        
        if (res.IsEmpty()) {
            return std::nullopt;
        }
        
        auto row = res[0];
        Booking booking;
        booking.id = row["id"].As<std::string>();
        booking.event_id = row["event_id"].As<std::string>();
        booking.user_id = row["user_id"].As<std::string>();
        booking.places_count = row["places_count"].As<int>();
        booking.status = ParseBookingStatus(row["status"].As<std::string>());
        booking.created_at = row["created_at"].As<std::string>();
        return booking;
        
    } catch (const std::exception& ex) {
        return std::nullopt;
    }
}

}  // namespace common::database
