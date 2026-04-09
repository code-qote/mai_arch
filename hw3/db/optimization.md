# Query Optimization Analysis

This document describes the optimization strategies applied to the Event Booking System database and provides EXPLAIN ANALYZE results for key queries.

## Index Strategy Overview

### Users Table Indexes

| Index Name | Columns | Type | Purpose |
|------------|---------|------|---------|
| `idx_users_username` | username | B-tree | Fast login lookup by username |
| `idx_users_email` | email | B-tree | Email search and uniqueness check |
| `idx_users_first_name` | first_name | B-tree (pattern_ops) | LIKE 'prefix%' searches on first name |
| `idx_users_last_name` | last_name | B-tree (pattern_ops) | LIKE 'prefix%' searches on last name |
| `idx_users_full_name` | (first_name, last_name) | B-tree | Combined name search |
| `idx_users_role` | role | B-tree | Filter users by role |

### Events Table Indexes

| Index Name | Columns | Type | Purpose |
|------------|---------|------|---------|
| `idx_events_organizer_id` | organizer_id | B-tree | FK lookup, filter by organizer |
| `idx_events_city` | city | B-tree | Filter events by city |
| `idx_events_country` | country | B-tree | Filter events by country |
| `idx_events_location` | (country, city) | B-tree | Combined location filter |
| `idx_events_name` | name | B-tree (pattern_ops) | LIKE 'prefix%' searches on name |
| `idx_events_event_time` | event_time | B-tree | Date range queries |
| `idx_events_available_places` | available_places | Partial B-tree | Find events with available places |

### Bookings Table Indexes

| Index Name | Columns | Type | Purpose |
|------------|---------|------|---------|
| `idx_bookings_event_id` | event_id | B-tree | FK lookup, filter by event |
| `idx_bookings_user_id` | user_id | B-tree | FK lookup, filter by user |
| `idx_bookings_status` | status | B-tree | Filter by booking status |
| `idx_bookings_user_event_active` | (user_id, event_id) | Unique partial | Prevent duplicate active bookings |
| `idx_bookings_created_at` | created_at DESC | B-tree | Order by creation date |

---

## Query Analysis with EXPLAIN ANALYZE

### 1. User Login Query

**Query:**
```sql
SELECT id, username, email, first_name, last_name, password_hash, role, created_at
FROM users
WHERE username = 'alice_wonder';
```

**EXPLAIN ANALYZE:**
```
Index Scan using idx_users_username on users  (cost=0.14..8.16 rows=1 width=1668) (actual time=0.065..0.066 rows=1 loops=1)
  Index Cond: ((username)::text = 'alice_wonder'::text)
Planning Time: 1.018 ms
Execution Time: 0.145 ms
```

**Analysis:**
- Index `idx_users_username` is used for fast lookup
- Index Scan provides O(log n) lookup complexity
- Only 1 row returned as expected (unique username)
- Execution time is very fast (0.145 ms)

---

### 2. User Search by Name (Partial Match with ILIKE)

**Query:**
```sql
SELECT id, username, email, first_name, last_name, role, created_at
FROM users
WHERE first_name ILIKE 'John%' AND last_name ILIKE 'S%';
```

**EXPLAIN ANALYZE:**
```
Seq Scan on users  (cost=0.00..10.60 rows=1 width=1152) (actual time=0.095..0.099 rows=1 loops=1)
  Filter: (((first_name)::text ~~* 'John%'::text) AND ((last_name)::text ~~* 'S%'::text))
  Rows Removed by Filter: 14
Planning Time: 0.714 ms
Execution Time: 0.178 ms
```

**Analysis:**
- Sequential Scan is used instead of index
- ILIKE (case-insensitive) cannot use `varchar_pattern_ops` indexes
- For small datasets, Seq Scan is acceptable
- For large datasets, consider functional indexes with LOWER()

**Optimization Recommendation:**
```sql
-- Create functional indexes for case-insensitive search
CREATE INDEX idx_users_first_name_lower ON users(LOWER(first_name) varchar_pattern_ops);
CREATE INDEX idx_users_last_name_lower ON users(LOWER(last_name) varchar_pattern_ops);

-- Then use LOWER() in queries:
SELECT * FROM users 
WHERE LOWER(first_name) LIKE LOWER('John%') 
  AND LOWER(last_name) LIKE LOWER('S%');
```

---

### 3. List Events by City

**Query:**
```sql
SELECT id, name, country, city, street, places_count, available_places, organizer_id, event_time, created_at
FROM events
WHERE city = 'New York'
ORDER BY event_time ASC;
```

**EXPLAIN ANALYZE:**
```
Sort  (cost=8.17..8.17 rows=1 width=1426) (actual time=0.087..0.087 rows=2 loops=1)
  Sort Key: event_time
  Sort Method: quicksort  Memory: 25kB
  ->  Index Scan using idx_events_city on events  (cost=0.14..8.16 rows=1 width=1426) (actual time=0.025..0.042 rows=2 loops=1)
        Index Cond: ((city)::text = 'New York'::text)
Planning Time: 0.674 ms
Execution Time: 0.139 ms
```

**Analysis:**
- Index `idx_events_city` is used for filtering
- Only 2 rows returned (events in New York)
- Additional Sort operation required for ORDER BY
- Sort is done in memory (quicksort, 25kB)

**Optimization Recommendation:**
For frequently used city + time queries, create a composite index:
```sql
CREATE INDEX idx_events_city_time ON events(city, event_time);
```

---

### 4. Reserve Event Places (Booking Transaction)

**Query:**
```sql
UPDATE events
SET available_places = available_places - 2
WHERE id = 'e0000000-0000-0000-0000-000000000001' AND available_places >= 2
RETURNING *;
```

**EXPLAIN ANALYZE:**
```
Update on events  (cost=0.14..8.16 rows=1 width=10) (actual time=1.298..1.299 rows=1 loops=1)
  ->  Index Scan using events_pkey on events  (cost=0.14..8.16 rows=1 width=10) (actual time=0.016..0.016 rows=1 loops=1)
        Index Cond: (id = 'e0000000-0000-0000-0000-000000000001'::uuid)
        Filter: (available_places >= 2)
Planning Time: 0.329 ms
Trigger update_events_updated_at: time=0.199 calls=1
Execution Time: 1.388 ms
```

**Analysis:**
- Primary key index is used for fast lookup
- Filter `available_places >= 2` is applied after index lookup
- Trigger `update_events_updated_at` is executed (0.199 ms)
- Total execution time is acceptable (1.388 ms)

---

### 5. Get User's Bookings with Event Details (JOIN)

**Query:**
```sql
SELECT 
    b.id AS booking_id,
    b.places_count,
    b.status,
    b.created_at AS booking_created_at,
    e.id AS event_id,
    e.name AS event_name,
    e.city,
    e.event_time
FROM bookings b
JOIN events e ON b.event_id = e.id
WHERE b.user_id = 'c0000000-0000-0000-0000-000000000001'
ORDER BY b.created_at DESC;
```

**EXPLAIN ANALYZE:**
```
Sort  (cost=22.04..22.05 rows=3 width=746) (actual time=0.093..0.094 rows=2 loops=1)
  Sort Key: b.created_at DESC
  Sort Method: quicksort  Memory: 25kB
  ->  Hash Join  (cost=11.32..22.02 rows=3 width=746) (actual time=0.078..0.081 rows=2 loops=1)
        Hash Cond: (e.id = b.event_id)
        ->  Seq Scan on events e  (cost=0.00..10.50 rows=50 width=660) (actual time=0.012..0.013 rows=12 loops=1)
        ->  Hash  (cost=11.28..11.28 rows=3 width=102) (actual time=0.058..0.058 rows=2 loops=1)
              Buckets: 1024  Batches: 1  Memory Usage: 9kB
              ->  Bitmap Heap Scan on bookings b  (cost=4.17..11.28 rows=3 width=102) (actual time=0.054..0.054 rows=2 loops=1)
                    Recheck Cond: (user_id = 'c0000000-0000-0000-0000-000000000001'::uuid)
                    Heap Blocks: exact=1
                    ->  Bitmap Index Scan on idx_bookings_user_id  (cost=0.00..4.17 rows=3 width=0) (actual time=0.032..0.032 rows=2 loops=1)
                          Index Cond: (user_id = 'c0000000-0000-0000-0000-000000000001'::uuid)
Planning Time: 1.468 ms
Execution Time: 0.127 ms
```

**Analysis:**
- Index `idx_bookings_user_id` is used via Bitmap Index Scan
- Hash Join is efficient for this query
- Sort is done in memory (quicksort, 25kB)
- Very fast execution (0.127 ms)

---

### 6. Check for Duplicate Active Booking (Partial Unique Index)

**Query:**
```sql
SELECT id, event_id, user_id, places_count, status, created_at
FROM bookings
WHERE user_id = 'c0000000-0000-0000-0000-000000000001' 
  AND event_id = 'e0000000-0000-0000-0000-000000000001' 
  AND status IN ('pending', 'confirmed');
```

**EXPLAIN ANALYZE:**
```
Index Scan using idx_bookings_user_event_active on bookings  (cost=0.14..8.16 rows=1 width=118) (actual time=0.041..0.042 rows=1 loops=1)
  Index Cond: ((user_id = 'c0000000-0000-0000-0000-000000000001'::uuid) AND (event_id = 'e0000000-0000-0000-0000-000000000001'::uuid))
Planning Time: 0.626 ms
Execution Time: 0.079 ms
```

**Analysis:**
- Partial unique index `idx_bookings_user_event_active` is used
- Index only includes rows where status IN ('pending', 'confirmed')
- Provides both fast lookup AND uniqueness constraint
- Very fast execution (0.079 ms)

---

### 7. Get Event Statistics (Aggregation with JOIN)

**Query:**
```sql
SELECT 
    e.id,
    e.name,
    e.places_count AS total_places,
    e.available_places,
    COUNT(CASE WHEN b.status = 'confirmed' THEN 1 END) AS confirmed_bookings,
    COUNT(CASE WHEN b.status = 'pending' THEN 1 END) AS pending_bookings
FROM events e
LEFT JOIN bookings b ON e.id = b.event_id
WHERE e.id = 'e0000000-0000-0000-0000-000000000001'
GROUP BY e.id, e.name, e.places_count, e.available_places;
```

**EXPLAIN ANALYZE:**
```
GroupAggregate  (cost=4.31..19.50 rows=1 width=458) (actual time=0.063..0.063 rows=1 loops=1)
  Group Key: e.id
  ->  Nested Loop Left Join  (cost=4.31..19.48 rows=1 width=500) (actual time=0.054..0.058 rows=2 loops=1)
        Join Filter: (e.id = b.event_id)
        ->  Index Scan using events_pkey on events e  (cost=0.14..8.16 rows=1 width=442) (actual time=0.013..0.016 rows=1 loops=1)
              Index Cond: (id = 'e0000000-0000-0000-0000-000000000001'::uuid)
        ->  Bitmap Heap Scan on bookings b  (cost=4.17..11.28 rows=3 width=74) (actual time=0.035..0.036 rows=2 loops=1)
              Recheck Cond: (event_id = 'e0000000-0000-0000-0000-000000000001'::uuid)
              Heap Blocks: exact=1
              ->  Bitmap Index Scan on idx_bookings_event_id  (cost=0.00..4.17 rows=3 width=0) (actual time=0.020..0.021 rows=2 loops=1)
                    Index Cond: (event_id = 'e0000000-0000-0000-0000-000000000001'::uuid)
Planning Time: 0.800 ms
Execution Time: 0.112 ms
```

**Analysis:**
- Primary key index on events is used for WHERE clause
- Index `idx_bookings_event_id` is used for LEFT JOIN
- Nested Loop Left Join is efficient for single-row lookup
- GroupAggregate is efficient for single-group aggregation
- Very fast execution (0.112 ms)

---

## Summary of Optimizations

### Applied Optimizations

1. **Primary Key Indexes**: Auto-created for all tables, used for fast lookups
2. **Foreign Key Indexes**: Added for JOIN performance (`idx_events_organizer_id`, `idx_bookings_event_id`, `idx_bookings_user_id`)
3. **Search Indexes**: Pattern_ops indexes for LIKE queries on names
4. **Partial Unique Index**: `idx_bookings_user_event_active` prevents duplicate active bookings
5. **Location Indexes**: Composite index for country + city filtering

### Performance Results

| Query | Index Used | Execution Time |
|-------|------------|----------------|
| User Login | idx_users_username | 0.145 ms |
| User Search (ILIKE) | Seq Scan | 0.178 ms |
| Events by City | idx_events_city | 0.139 ms |
| Reserve Places | events_pkey | 1.388 ms |
| User Bookings (JOIN) | idx_bookings_user_id | 0.127 ms |
| Check Duplicate Booking | idx_bookings_user_event_active | 0.079 ms |
| Event Statistics | events_pkey + idx_bookings_event_id | 0.112 ms |

### Recommendations for Production

1. **For case-insensitive search**: Create functional indexes with LOWER()
2. **For city + time queries**: Create composite index `(city, event_time)`
3. **Monitor slow queries**: Use `pg_stat_statements` extension
4. **Regular maintenance**: Run `ANALYZE` after bulk data changes
5. **Connection pooling**: Use PgBouncer for high-traffic scenarios
