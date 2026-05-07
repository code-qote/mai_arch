#include <common/database/analytics_mongo.hpp>

#include <userver/formats/bson/inline.hpp>
#include <userver/formats/bson/value_builder.hpp>
#include <userver/formats/bson/types.hpp>
#include <userver/utils/datetime.hpp>

namespace common::database {

namespace {

namespace formats = userver::formats;
namespace bson = formats::bson;

bson::Value TimeToBson(const std::chrono::system_clock::time_point& tp) {
    return bson::ValueBuilder(tp).ExtractValue();
}

std::chrono::system_clock::time_point BsonToTime(const bson::Value& value) {
    return value.As<std::chrono::system_clock::time_point>();
}

}

AnalyticsMongo::AnalyticsMongo(userver::storages::mongo::PoolPtr pool)
    : pool_(std::move(pool)) {}

AnalyticsResult<ActivityLog> AnalyticsMongo::LogActivity(const ActivityLog& log) const {
    try {
        bson::ValueBuilder details_builder;
        details_builder["event_id"] = log.event_id;
        details_builder["event_name"] = log.event_name;
        details_builder["booking_id"] = log.booking_id;
        details_builder["places_count"] = log.places_count;
        details_builder["amount"] = log.amount;

        bson::ValueBuilder metadata_builder;
        metadata_builder["session_id"] = log.session_id;
        metadata_builder["ip_address"] = log.ip_address;
        metadata_builder["user_agent"] = log.user_agent;

        bson::ValueBuilder builder;
        builder["user_id"] = log.user_id;
        builder["activity_type"] = log.activity_type;
        builder["timestamp"] = TimeToBson(log.timestamp);
        builder["details"] = details_builder.ExtractValue();
        builder["metadata"] = metadata_builder.ExtractValue();

        auto collection = pool_->GetCollection(kActivityLogsCollection);
        collection.InsertOne(builder.ExtractValue());

        return {true, "", log};
    } catch (const std::exception& e) {
        return {false, std::string("Failed to log activity: ") + e.what(), {}};
    }
}

std::vector<ActivityLog> AnalyticsMongo::GetUserActivities(
    const std::string& user_id,
    const std::optional<std::chrono::system_clock::time_point>& from,
    const std::optional<std::chrono::system_clock::time_point>& to,
    const std::optional<std::string>& activity_type) const {
    
    std::vector<ActivityLog> results;
    
    try {
        bson::ValueBuilder filter_builder;
        filter_builder["user_id"] = user_id;
        
        if (from || to) {
            bson::ValueBuilder timestamp_filter;
            if (from) {
                timestamp_filter["$gte"] = TimeToBson(*from);
            }
            if (to) {
                timestamp_filter["$lte"] = TimeToBson(*to);
            }
            filter_builder["timestamp"] = timestamp_filter.ExtractValue();
        }
        
        if (activity_type) {
            filter_builder["activity_type"] = *activity_type;
        }

        auto collection = pool_->GetCollection(kActivityLogsCollection);
        auto cursor = collection.Find(filter_builder.ExtractValue());

        for (const auto& doc : cursor) {
            ActivityLog log;
            log.user_id = doc["user_id"].As<std::string>();
            log.activity_type = doc["activity_type"].As<std::string>();
            log.timestamp = BsonToTime(doc["timestamp"]);
            
            if (doc.HasMember("details")) {
                auto details = doc["details"];
                log.event_id = details["event_id"].As<std::string>("");
                log.event_name = details["event_name"].As<std::string>("");
                log.booking_id = details["booking_id"].As<std::string>("");
                log.places_count = details["places_count"].As<int>(0);
                log.amount = details["amount"].As<double>(0.0);
            }
            
            if (doc.HasMember("metadata")) {
                auto metadata = doc["metadata"];
                log.session_id = metadata["session_id"].As<std::string>("");
                log.ip_address = metadata["ip_address"].As<std::string>("");
                log.user_agent = metadata["user_agent"].As<std::string>("");
            }
            
            results.push_back(std::move(log));
        }
    } catch (const std::exception&) {
    }
    
    return results;
}

AnalyticsResult<SearchHistory> AnalyticsMongo::RecordSearch(const SearchHistory& search) const {
    try {
        bson::ValueBuilder search_query_builder;
        if (!search.city.empty()) {
            search_query_builder["city"] = search.city;
        }
        if (!search.country.empty()) {
            search_query_builder["country"] = search.country;
        }
        if (!search.name_keyword.empty()) {
            search_query_builder["name_keyword"] = search.name_keyword;
        }
        if (search.date_from) {
            search_query_builder["date_from"] = TimeToBson(*search.date_from);
        }
        if (search.date_to) {
            search_query_builder["date_to"] = TimeToBson(*search.date_to);
        }

        bson::ValueBuilder clicked_events_array(formats::common::Type::kArray);
        for (const auto& event_id : search.clicked_event_ids) {
            clicked_events_array.PushBack(event_id);
        }

        bson::ValueBuilder builder;
        builder["user_id"] = search.user_id;
        builder["search_query"] = search_query_builder.ExtractValue();
        builder["results_count"] = search.results_count;
        builder["clicked_events"] = clicked_events_array.ExtractValue();
        builder["conversion"] = search.conversion;
        builder["searched_at"] = TimeToBson(search.searched_at);
        builder["session_id"] = search.session_id;

        if (search.booked_event_id) {
            builder["booked_event_id"] = *search.booked_event_id;
        }

        auto collection = pool_->GetCollection(kSearchHistoryCollection);
        collection.InsertOne(builder.ExtractValue());

        return {true, "", search};
    } catch (const std::exception& e) {
        return {false, std::string("Failed to record search: ") + e.what(), {}};
    }
}

std::vector<SearchHistory> AnalyticsMongo::GetUserSearchHistory(
    const std::string& user_id,
    int limit) const {
    
    std::vector<SearchHistory> results;
    
    try {
        bson::ValueBuilder filter_builder;
        filter_builder["user_id"] = user_id;

        auto collection = pool_->GetCollection(kSearchHistoryCollection);
        
        auto cursor = collection.Find(
            filter_builder.ExtractValue(),
            userver::storages::mongo::options::Sort{{"searched_at", userver::storages::mongo::options::Sort::kDescending}},
            userver::storages::mongo::options::Limit{static_cast<size_t>(limit)}
        );

        for (const auto& doc : cursor) {
            SearchHistory search;
            search.user_id = doc["user_id"].As<std::string>();
            search.results_count = doc["results_count"].As<int>(0);
            search.conversion = doc["conversion"].As<bool>(false);
            search.searched_at = BsonToTime(doc["searched_at"]);
            search.session_id = doc["session_id"].As<std::string>("");

            if (doc.HasMember("search_query")) {
                auto query = doc["search_query"];
                search.city = query["city"].As<std::string>("");
                search.country = query["country"].As<std::string>("");
                search.name_keyword = query["name_keyword"].As<std::string>("");
            }

            if (doc.HasMember("booked_event_id")) {
                search.booked_event_id = doc["booked_event_id"].As<std::string>();
            }

            results.push_back(std::move(search));
        }
    } catch (const std::exception&) {
    }
    
    return results;
}

std::vector<std::pair<std::string, int>> AnalyticsMongo::GetPopularSearchCities(int limit) const {
    std::vector<std::pair<std::string, int>> results;
    
    try {
        bson::ValueBuilder group_stage;
        {
            bson::ValueBuilder group_doc;
            group_doc["_id"] = "$search_query.city";
            
            bson::ValueBuilder sum_doc;
            sum_doc["$sum"] = 1;
            group_doc["count"] = sum_doc.ExtractValue();
            
            group_stage["$group"] = group_doc.ExtractValue();
        }

        bson::ValueBuilder sort_stage;
        {
            bson::ValueBuilder sort_doc;
            sort_doc["count"] = -1;
            sort_stage["$sort"] = sort_doc.ExtractValue();
        }

        bson::ValueBuilder limit_stage;
        limit_stage["$limit"] = limit;

        bson::ValueBuilder pipeline(formats::common::Type::kArray);
        pipeline.PushBack(group_stage.ExtractValue());
        pipeline.PushBack(sort_stage.ExtractValue());
        pipeline.PushBack(limit_stage.ExtractValue());

        auto collection = pool_->GetCollection(kSearchHistoryCollection);
        auto cursor = collection.Aggregate(pipeline.ExtractValue());

        for (const auto& doc : cursor) {
            std::string city = doc["_id"].As<std::string>("");
            int count = doc["count"].As<int>(0);
            if (!city.empty()) {
                results.emplace_back(city, count);
            }
        }
    } catch (const std::exception&) {
    }
    
    return results;
}

AnalyticsResult<EventStatistics> AnalyticsMongo::UpdateEventStatistics(
    const EventStatistics& stats) const {
    
    try {
        auto now = std::chrono::system_clock::now();
        
        bson::ValueBuilder revenue_builder;
        revenue_builder["total"] = stats.total_revenue;
        revenue_builder["currency"] = "USD";

        bson::ValueBuilder statistics_builder;
        statistics_builder["total_bookings"] = stats.total_bookings;
        statistics_builder["total_participants"] = stats.total_participants;
        statistics_builder["total_places_booked"] = stats.total_places_booked;
        statistics_builder["total_places_available"] = stats.total_places_available;
        statistics_builder["occupancy_rate"] = stats.occupancy_rate;
        statistics_builder["cancelled_bookings"] = stats.cancelled_bookings;
        statistics_builder["cancellation_rate"] = stats.cancellation_rate;
        statistics_builder["average_places_per_booking"] = stats.average_places_per_booking;
        statistics_builder["revenue"] = revenue_builder.ExtractValue();

        bson::ValueBuilder doc_builder;
        doc_builder["event_id"] = stats.event_id;
        doc_builder["event_name"] = stats.event_name;
        doc_builder["organizer_id"] = stats.organizer_id;
        doc_builder["statistics"] = statistics_builder.ExtractValue();
        doc_builder["updated_at"] = TimeToBson(now);

        bson::ValueBuilder filter_builder;
        filter_builder["event_id"] = stats.event_id;

        bson::ValueBuilder set_on_insert_builder;
        set_on_insert_builder["created_at"] = TimeToBson(now);

        bson::ValueBuilder update_builder;
        update_builder["$set"] = doc_builder.ExtractValue();
        update_builder["$setOnInsert"] = set_on_insert_builder.ExtractValue();

        auto collection = pool_->GetCollection(kEventStatsCollection);
        collection.UpdateOne(
            filter_builder.ExtractValue(),
            update_builder.ExtractValue(),
            userver::storages::mongo::options::Upsert{}
        );

        return {true, "", stats};
    } catch (const std::exception& e) {
        return {false, std::string("Failed to update statistics: ") + e.what(), {}};
    }
}

std::optional<EventStatistics> AnalyticsMongo::GetEventStatistics(
    const std::string& event_id) const {
    
    try {
        bson::ValueBuilder filter_builder;
        filter_builder["event_id"] = event_id;
        
        auto collection = pool_->GetCollection(kEventStatsCollection);
        auto doc_opt = collection.FindOne(filter_builder.ExtractValue());

        if (!doc_opt) {
            return std::nullopt;
        }

        auto doc = *doc_opt;
        EventStatistics stats;
        stats.event_id = doc["event_id"].As<std::string>();
        stats.event_name = doc["event_name"].As<std::string>("");
        stats.organizer_id = doc["organizer_id"].As<std::string>("");
        stats.created_at = BsonToTime(doc["created_at"]);
        stats.updated_at = BsonToTime(doc["updated_at"]);

        if (doc.HasMember("statistics")) {
            auto statistics = doc["statistics"];
            stats.total_bookings = statistics["total_bookings"].As<int>(0);
            stats.total_participants = statistics["total_participants"].As<int>(0);
            stats.total_places_booked = statistics["total_places_booked"].As<int>(0);
            stats.total_places_available = statistics["total_places_available"].As<int>(0);
            stats.occupancy_rate = statistics["occupancy_rate"].As<double>(0.0);
            stats.cancelled_bookings = statistics["cancelled_bookings"].As<int>(0);
            stats.cancellation_rate = statistics["cancellation_rate"].As<double>(0.0);
            stats.average_places_per_booking = statistics["average_places_per_booking"].As<double>(0.0);
            
            if (statistics.HasMember("revenue")) {
                stats.total_revenue = statistics["revenue"]["total"].As<double>(0.0);
            }
        }

        return stats;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

std::vector<EventStatistics> AnalyticsMongo::GetTopEventsByOccupancy(int limit) const {
    std::vector<EventStatistics> results;
    
    try {
        bson::ValueBuilder gte_builder;
        gte_builder["$gte"] = 0.0;

        bson::ValueBuilder filter_builder;
        filter_builder["statistics.occupancy_rate"] = gte_builder.ExtractValue();

        auto collection = pool_->GetCollection(kEventStatsCollection);
        auto cursor = collection.Find(
            filter_builder.ExtractValue(),
            userver::storages::mongo::options::Sort{{"statistics.occupancy_rate", userver::storages::mongo::options::Sort::kDescending}},
            userver::storages::mongo::options::Limit{static_cast<size_t>(limit)}
        );

        for (const auto& doc : cursor) {
            EventStatistics stats;
            stats.event_id = doc["event_id"].As<std::string>();
            stats.event_name = doc["event_name"].As<std::string>("");
            stats.organizer_id = doc["organizer_id"].As<std::string>("");
            stats.created_at = BsonToTime(doc["created_at"]);
            stats.updated_at = BsonToTime(doc["updated_at"]);

            if (doc.HasMember("statistics")) {
                auto statistics = doc["statistics"];
                stats.total_bookings = statistics["total_bookings"].As<int>(0);
                stats.total_participants = statistics["total_participants"].As<int>(0);
                stats.total_places_booked = statistics["total_places_booked"].As<int>(0);
                stats.total_places_available = statistics["total_places_available"].As<int>(0);
                stats.occupancy_rate = statistics["occupancy_rate"].As<double>(0.0);
                stats.cancelled_bookings = statistics["cancelled_bookings"].As<int>(0);
                stats.cancellation_rate = statistics["cancellation_rate"].As<double>(0.0);
                stats.average_places_per_booking = statistics["average_places_per_booking"].As<double>(0.0);
                
                if (statistics.HasMember("revenue")) {
                    stats.total_revenue = statistics["revenue"]["total"].As<double>(0.0);
                }
            }

            results.push_back(std::move(stats));
        }
    } catch (const std::exception&) {
    }
    
    return results;
}

}
