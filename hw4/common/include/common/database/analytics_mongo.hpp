#pragma once

#include <userver/storages/mongo.hpp>
#include <userver/storages/mongo/component.hpp>
#include <userver/formats/bson.hpp>

#include <string>
#include <vector>
#include <optional>
#include <chrono>

namespace common::database {

struct ActivityLog {
    std::string user_id;
    std::string activity_type;
    std::chrono::system_clock::time_point timestamp;
    std::string event_id;
    std::string event_name;
    std::string booking_id;
    int places_count;
    double amount;
    std::string session_id;
    std::string ip_address;
    std::string user_agent;
};

struct SearchHistory {
    std::string user_id;
    std::string city;
    std::string country;
    std::string name_keyword;
    std::optional<std::chrono::system_clock::time_point> date_from;
    std::optional<std::chrono::system_clock::time_point> date_to;
    int results_count;
    std::vector<std::string> clicked_event_ids;
    std::optional<std::string> booked_event_id;
    bool conversion;
    std::chrono::system_clock::time_point searched_at;
    std::string session_id;
};

struct EventStatistics {
    std::string event_id;
    std::string event_name;
    std::string organizer_id;
    int total_bookings;
    int total_participants;
    int total_places_booked;
    int total_places_available;
    double occupancy_rate;
    int cancelled_bookings;
    double cancellation_rate;
    double average_places_per_booking;
    double total_revenue;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point updated_at;
};

template<typename T>
struct AnalyticsResult {
    bool success;
    std::string error;
    T data;
};

class AnalyticsMongo {
public:
    explicit AnalyticsMongo(userver::storages::mongo::PoolPtr pool);

    AnalyticsResult<ActivityLog> LogActivity(const ActivityLog& log) const;
    
    std::vector<ActivityLog> GetUserActivities(
        const std::string& user_id,
        const std::optional<std::chrono::system_clock::time_point>& from,
        const std::optional<std::chrono::system_clock::time_point>& to,
        const std::optional<std::string>& activity_type = std::nullopt) const;

    AnalyticsResult<SearchHistory> RecordSearch(const SearchHistory& search) const;
    
    std::vector<SearchHistory> GetUserSearchHistory(
        const std::string& user_id,
        int limit = 10) const;
    
    std::vector<std::pair<std::string, int>> GetPopularSearchCities(int limit = 20) const;

    AnalyticsResult<EventStatistics> UpdateEventStatistics(const EventStatistics& stats) const;
    
    std::optional<EventStatistics> GetEventStatistics(const std::string& event_id) const;
    
    std::vector<EventStatistics> GetTopEventsByOccupancy(int limit = 10) const;

private:
    userver::storages::mongo::PoolPtr pool_;
    
    static constexpr const char* kActivityLogsCollection = "user_activity_logs";
    static constexpr const char* kSearchHistoryCollection = "event_search_history";
    static constexpr const char* kEventStatsCollection = "event_statistics";
};

}
