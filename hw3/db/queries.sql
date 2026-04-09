-- ============================================================================
-- Event Booking System - SQL Queries for API Operations
-- ============================================================================
-- This file contains all SQL queries used by the API endpoints
-- ============================================================================

-- ============================================================================
-- USER SERVICE QUERIES
-- ============================================================================

-- ----------------------------------------------------------------------------
-- POST /register - Register a new user
-- ----------------------------------------------------------------------------
-- Insert new user and return the created user data
INSERT INTO users (username, email, first_name, last_name, password_hash, role)
VALUES ($1, $2, $3, $4, $5, $6)
RETURNING id, username, email, first_name, last_name, role, created_at;

-- Example:
-- INSERT INTO users (username, email, first_name, last_name, password_hash, role)
-- VALUES ('john_doe', 'john@example.com', 'John', 'Doe', '$2a$10$...', 'participant')
-- RETURNING id, username, email, first_name, last_name, role, created_at;

-- ----------------------------------------------------------------------------
-- POST /login - Login user
-- ----------------------------------------------------------------------------
-- Find user by username for authentication
SELECT id, username, email, first_name, last_name, password_hash, role, created_at
FROM users
WHERE username = $1;

-- Example:
-- SELECT id, username, email, first_name, last_name, password_hash, role, created_at
-- FROM users
-- WHERE username = 'john_doe';

-- ----------------------------------------------------------------------------
-- POST /search - Search users (admin only)
-- ----------------------------------------------------------------------------
-- Search users by various criteria (all filters are optional)
-- Base query with dynamic WHERE conditions

-- Search by exact ID
SELECT id, username, email, first_name, last_name, role, created_at
FROM users
WHERE id = $1;

-- Search by username (partial match)
SELECT id, username, email, first_name, last_name, role, created_at
FROM users
WHERE username ILIKE $1 || '%';

-- Search by email (partial match)
SELECT id, username, email, first_name, last_name, role, created_at
FROM users
WHERE email ILIKE $1 || '%';

-- Search by first_name and last_name (partial match)
SELECT id, username, email, first_name, last_name, role, created_at
FROM users
WHERE first_name ILIKE $1 || '%'
  AND last_name ILIKE $2 || '%';

-- Get all users (empty filter)
SELECT id, username, email, first_name, last_name, role, created_at
FROM users
ORDER BY created_at DESC;

-- Combined search query (dynamic)
SELECT id, username, email, first_name, last_name, role, created_at
FROM users
WHERE ($1::uuid IS NULL OR id = $1)
  AND ($2::text IS NULL OR username ILIKE $2 || '%')
  AND ($3::text IS NULL OR email ILIKE $3 || '%')
  AND ($4::text IS NULL OR first_name ILIKE $4 || '%')
  AND ($5::text IS NULL OR last_name ILIKE $5 || '%')
ORDER BY created_at DESC;

-- ============================================================================
-- EVENT SERVICE QUERIES
-- ============================================================================

-- ----------------------------------------------------------------------------
-- POST /events - Create event (organizer/admin only)
-- ----------------------------------------------------------------------------
-- Insert new event and return the created event data
INSERT INTO events (name, country, city, street, places_count, available_places, organizer_id, event_time)
VALUES ($1, $2, $3, $4, $5, $5, $6, $7)
RETURNING id, name, country, city, street, places_count, available_places, organizer_id, event_time, created_at;

-- Example:
-- INSERT INTO events (name, country, city, street, places_count, available_places, organizer_id, event_time)
-- VALUES ('Summer Music Festival', 'USA', 'New York', 'Central Park West', 500, 500, 'b0000000-0000-0000-0000-000000000001', '2024-07-15 18:00:00+00')
-- RETURNING id, name, country, city, street, places_count, available_places, organizer_id, event_time, created_at;

-- ----------------------------------------------------------------------------
-- POST /events/list - List events with filters
-- ----------------------------------------------------------------------------
-- Get all events
SELECT id, name, country, city, street, places_count, available_places, organizer_id, event_time, created_at
FROM events
ORDER BY event_time ASC;

-- Get event by ID
SELECT id, name, country, city, street, places_count, available_places, organizer_id, event_time, created_at
FROM events
WHERE id = $1;

-- Get events by organizer
SELECT id, name, country, city, street, places_count, available_places, organizer_id, event_time, created_at
FROM events
WHERE organizer_id = $1
ORDER BY event_time ASC;

-- Get events by city
SELECT id, name, country, city, street, places_count, available_places, organizer_id, event_time, created_at
FROM events
WHERE city = $1
ORDER BY event_time ASC;

-- Get events by country
SELECT id, name, country, city, street, places_count, available_places, organizer_id, event_time, created_at
FROM events
WHERE country = $1
ORDER BY event_time ASC;

-- Get events by name (partial match)
SELECT id, name, country, city, street, places_count, available_places, organizer_id, event_time, created_at
FROM events
WHERE name ILIKE '%' || $1 || '%'
ORDER BY event_time ASC;

-- Combined search query (dynamic)
SELECT id, name, country, city, street, places_count, available_places, organizer_id, event_time, created_at
FROM events
WHERE ($1::uuid IS NULL OR id = $1)
  AND ($2::text IS NULL OR name ILIKE '%' || $2 || '%')
  AND ($3::uuid IS NULL OR organizer_id = $3)
  AND ($4::text IS NULL OR country = $4)
  AND ($5::text IS NULL OR city = $5)
ORDER BY event_time ASC;

-- Get events by date range
SELECT id, name, country, city, street, places_count, available_places, organizer_id, event_time, created_at
FROM events
WHERE event_time >= $1 AND event_time <= $2
ORDER BY event_time ASC;

-- ----------------------------------------------------------------------------
-- POST /events/book - Reserve event places (internal, called by Book Service)
-- ----------------------------------------------------------------------------
-- Check if event exists and has available places
SELECT id, name, country, city, street, places_count, available_places, organizer_id, event_time, created_at
FROM events
WHERE id = $1 AND available_places >= $2
FOR UPDATE;

-- Reserve places (decrement available_places)
UPDATE events
SET available_places = available_places - $2
WHERE id = $1 AND available_places >= $2
RETURNING id, name, country, city, street, places_count, available_places, organizer_id, event_time, created_at;

-- Example:
-- UPDATE events
-- SET available_places = available_places - 2
-- WHERE id = 'e0000000-0000-0000-0000-000000000001' AND available_places >= 2
-- RETURNING id, name, country, city, street, places_count, available_places, organizer_id, event_time, created_at;

-- ----------------------------------------------------------------------------
-- POST /events/cancel - Release event places (internal, called by Book Service)
-- ----------------------------------------------------------------------------
-- Get booking to find places_count
SELECT places_count
FROM bookings
WHERE id = $1 AND user_id = $2 AND event_id = $3 AND status = 'confirmed';

-- Release places (increment available_places)
UPDATE events
SET available_places = available_places + $2
WHERE id = $1 AND available_places + $2 <= places_count
RETURNING id, name, country, city, street, places_count, available_places, organizer_id, event_time, created_at;

-- Example:
-- UPDATE events
-- SET available_places = available_places + 2
-- WHERE id = 'e0000000-0000-0000-0000-000000000001' AND available_places + 2 <= places_count
-- RETURNING id, name, country, city, street, places_count, available_places, organizer_id, event_time, created_at;

-- ----------------------------------------------------------------------------
-- POST /bookings/book - Create a booking
-- ----------------------------------------------------------------------------
-- Check if user already has an active booking for this event
SELECT id, event_id, user_id, places_count, status, created_at
FROM bookings
WHERE user_id = $1 AND event_id = $2 AND status IN ('pending', 'confirmed');

-- Create new booking in pending state
INSERT INTO bookings (event_id, user_id, places_count, status)
VALUES ($1, $2, $3, 'pending')
RETURNING id, event_id, user_id, places_count, status, created_at;

-- Update booking status to confirmed
UPDATE bookings
SET status = 'confirmed'
WHERE id = $1
RETURNING id, event_id, user_id, places_count, status, created_at;

-- Update booking status to failed (if reservation fails)
UPDATE bookings
SET status = 'failed'
WHERE id = $1
RETURNING id, event_id, user_id, places_count, status, created_at;

-- Example:
-- INSERT INTO bookings (event_id, user_id, places_count, status)
-- VALUES ('e0000000-0000-0000-0000-000000000001', 'c0000000-0000-0000-0000-000000000001', 2, 'pending')
-- RETURNING id, event_id, user_id, places_count, status, created_at;

-- ----------------------------------------------------------------------------
-- POST /bookings/cancel - Cancel a booking
-- ----------------------------------------------------------------------------
-- Get booking by ID and verify ownership
SELECT id, event_id, user_id, places_count, status, created_at
FROM bookings
WHERE id = $1;

-- Get booking by ID and user_id (for ownership verification)
SELECT id, event_id, user_id, places_count, status, created_at
FROM bookings
WHERE id = $1 AND user_id = $2;

-- Update booking status to cancelled
UPDATE bookings
SET status = 'cancelled'
WHERE id = $1 AND status IN ('pending', 'confirmed')
RETURNING id, event_id, user_id, places_count, status, created_at;

-- Example:
-- UPDATE bookings
-- SET status = 'cancelled'
-- WHERE id = 'd0000000-0000-0000-0000-000000000001' AND status IN ('pending', 'confirmed')
-- RETURNING id, event_id, user_id, places_count, status, created_at;

-- ============================================================================
-- ADDITIONAL USEFUL QUERIES
-- ============================================================================

-- ----------------------------------------------------------------------------
-- Get user's bookings with event details
-- ----------------------------------------------------------------------------
SELECT 
    b.id AS booking_id,
    b.places_count,
    b.status,
    b.created_at AS booking_created_at,
    e.id AS event_id,
    e.name AS event_name,
    e.country,
    e.city,
    e.street,
    e.event_time
FROM bookings b
JOIN events e ON b.event_id = e.id
WHERE b.user_id = $1
ORDER BY b.created_at DESC;

-- ----------------------------------------------------------------------------
-- Get event participants (for organizers)
-- ----------------------------------------------------------------------------
SELECT 
    u.id AS user_id,
    u.username,
    u.first_name,
    u.last_name,
    u.email,
    b.places_count,
    b.status,
    b.created_at AS booking_created_at
FROM bookings b
JOIN users u ON b.user_id = u.id
WHERE b.event_id = $1 AND b.status = 'confirmed'
ORDER BY b.created_at ASC;

-- ----------------------------------------------------------------------------
-- Get event statistics
-- ----------------------------------------------------------------------------
SELECT 
    e.id,
    e.name,
    e.places_count AS total_places,
    e.available_places,
    e.places_count - e.available_places AS booked_places,
    COUNT(CASE WHEN b.status = 'confirmed' THEN 1 END) AS confirmed_bookings,
    COUNT(CASE WHEN b.status = 'pending' THEN 1 END) AS pending_bookings,
    COUNT(CASE WHEN b.status = 'cancelled' THEN 1 END) AS cancelled_bookings
FROM events e
LEFT JOIN bookings b ON e.id = b.event_id
WHERE e.id = $1
GROUP BY e.id, e.name, e.places_count, e.available_places;

-- ----------------------------------------------------------------------------
-- Get organizer's events with booking statistics
-- ----------------------------------------------------------------------------
SELECT 
    e.id,
    e.name,
    e.city,
    e.event_time,
    e.places_count AS total_places,
    e.available_places,
    COUNT(CASE WHEN b.status = 'confirmed' THEN 1 END) AS confirmed_bookings,
    SUM(CASE WHEN b.status = 'confirmed' THEN b.places_count ELSE 0 END) AS total_booked_places
FROM events e
LEFT JOIN bookings b ON e.id = b.event_id
WHERE e.organizer_id = $1
GROUP BY e.id, e.name, e.city, e.event_time, e.places_count, e.available_places
ORDER BY e.event_time ASC;

-- ----------------------------------------------------------------------------
-- Check username availability
-- ----------------------------------------------------------------------------
SELECT EXISTS(SELECT 1 FROM users WHERE username = $1) AS username_exists;

-- ----------------------------------------------------------------------------
-- Check email availability
-- ----------------------------------------------------------------------------
SELECT EXISTS(SELECT 1 FROM users WHERE email = $1) AS email_exists;
