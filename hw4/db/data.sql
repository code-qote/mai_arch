-- ============================================================================
-- Event Booking System - Test Data
-- ============================================================================
-- This script inserts test data for all tables (minimum 10 records each)
-- Password hash is bcrypt hash of 'password123' for all users
-- ============================================================================

-- ============================================================================
-- Users (15 records: 2 admins, 5 organizers, 8 participants)
-- ============================================================================

INSERT INTO users (id, username, email, first_name, last_name, password_hash, role, created_at) VALUES
-- Admins
('a0000000-0000-0000-0000-000000000001', 'admin', 'admin@eventbooking.com', 'System', 'Administrator', '$2a$10$N9qo8uLOickgx2ZMRZoMyeIjZRGdjGj/n3.Q7Z5xQhqQxou5Uh1S.', 'admin', '2024-01-01 00:00:00+00'),
('a0000000-0000-0000-0000-000000000002', 'superadmin', 'superadmin@eventbooking.com', 'Super', 'Admin', '$2a$10$N9qo8uLOickgx2ZMRZoMyeIjZRGdjGj/n3.Q7Z5xQhqQxou5Uh1S.', 'admin', '2024-01-01 00:00:00+00'),

-- Organizers
('b0000000-0000-0000-0000-000000000001', 'music_events', 'music@events.com', 'John', 'Smith', '$2a$10$N9qo8uLOickgx2ZMRZoMyeIjZRGdjGj/n3.Q7Z5xQhqQxou5Uh1S.', 'organizer', '2024-01-05 10:00:00+00'),
('b0000000-0000-0000-0000-000000000002', 'tech_conf', 'tech@conferences.com', 'Emily', 'Johnson', '$2a$10$N9qo8uLOickgx2ZMRZoMyeIjZRGdjGj/n3.Q7Z5xQhqQxou5Uh1S.', 'organizer', '2024-01-06 11:00:00+00'),
('b0000000-0000-0000-0000-000000000003', 'sports_org', 'sports@events.com', 'Michael', 'Williams', '$2a$10$N9qo8uLOickgx2ZMRZoMyeIjZRGdjGj/n3.Q7Z5xQhqQxou5Uh1S.', 'organizer', '2024-01-07 12:00:00+00'),
('b0000000-0000-0000-0000-000000000004', 'art_gallery', 'art@gallery.com', 'Sarah', 'Brown', '$2a$10$N9qo8uLOickgx2ZMRZoMyeIjZRGdjGj/n3.Q7Z5xQhqQxou5Uh1S.', 'organizer', '2024-01-08 13:00:00+00'),
('b0000000-0000-0000-0000-000000000005', 'food_fest', 'food@festivals.com', 'David', 'Davis', '$2a$10$N9qo8uLOickgx2ZMRZoMyeIjZRGdjGj/n3.Q7Z5xQhqQxou5Uh1S.', 'organizer', '2024-01-09 14:00:00+00'),

-- Participants
('c0000000-0000-0000-0000-000000000001', 'alice_wonder', 'alice@example.com', 'Alice', 'Wonder', '$2a$10$N9qo8uLOickgx2ZMRZoMyeIjZRGdjGj/n3.Q7Z5xQhqQxou5Uh1S.', 'participant', '2024-01-10 09:00:00+00'),
('c0000000-0000-0000-0000-000000000002', 'bob_builder', 'bob@example.com', 'Bob', 'Builder', '$2a$10$N9qo8uLOickgx2ZMRZoMyeIjZRGdjGj/n3.Q7Z5xQhqQxou5Uh1S.', 'participant', '2024-01-11 10:00:00+00'),
('c0000000-0000-0000-0000-000000000003', 'charlie_chap', 'charlie@example.com', 'Charlie', 'Chaplin', '$2a$10$N9qo8uLOickgx2ZMRZoMyeIjZRGdjGj/n3.Q7Z5xQhqQxou5Uh1S.', 'participant', '2024-01-12 11:00:00+00'),
('c0000000-0000-0000-0000-000000000004', 'diana_prince', 'diana@example.com', 'Diana', 'Prince', '$2a$10$N9qo8uLOickgx2ZMRZoMyeIjZRGdjGj/n3.Q7Z5xQhqQxou5Uh1S.', 'participant', '2024-01-13 12:00:00+00'),
('c0000000-0000-0000-0000-000000000005', 'edward_snow', 'edward@example.com', 'Edward', 'Snow', '$2a$10$N9qo8uLOickgx2ZMRZoMyeIjZRGdjGj/n3.Q7Z5xQhqQxou5Uh1S.', 'participant', '2024-01-14 13:00:00+00'),
('c0000000-0000-0000-0000-000000000006', 'fiona_green', 'fiona@example.com', 'Fiona', 'Green', '$2a$10$N9qo8uLOickgx2ZMRZoMyeIjZRGdjGj/n3.Q7Z5xQhqQxou5Uh1S.', 'participant', '2024-01-15 14:00:00+00'),
('c0000000-0000-0000-0000-000000000007', 'george_lucas', 'george@example.com', 'George', 'Lucas', '$2a$10$N9qo8uLOickgx2ZMRZoMyeIjZRGdjGj/n3.Q7Z5xQhqQxou5Uh1S.', 'participant', '2024-01-16 15:00:00+00'),
('c0000000-0000-0000-0000-000000000008', 'hannah_montana', 'hannah@example.com', 'Hannah', 'Montana', '$2a$10$N9qo8uLOickgx2ZMRZoMyeIjZRGdjGj/n3.Q7Z5xQhqQxou5Uh1S.', 'participant', '2024-01-17 16:00:00+00');

-- ============================================================================
-- Events (12 records)
-- ============================================================================

INSERT INTO events (id, name, country, city, street, places_count, available_places, organizer_id, event_time, created_at) VALUES
-- Music events (organizer: music_events)
('e0000000-0000-0000-0000-000000000001', 'Summer Music Festival', 'USA', 'New York', 'Central Park West', 500, 495, 'b0000000-0000-0000-0000-000000000001', '2024-07-15 18:00:00+00', '2024-02-01 10:00:00+00'),
('e0000000-0000-0000-0000-000000000002', 'Jazz Night', 'USA', 'New Orleans', 'Bourbon Street 123', 100, 98, 'b0000000-0000-0000-0000-000000000001', '2024-06-20 20:00:00+00', '2024-02-02 11:00:00+00'),
('e0000000-0000-0000-0000-000000000003', 'Rock Concert', 'UK', 'London', 'Wembley Stadium', 1000, 990, 'b0000000-0000-0000-0000-000000000001', '2024-08-10 19:00:00+00', '2024-02-03 12:00:00+00'),

-- Tech conferences (organizer: tech_conf)
('e0000000-0000-0000-0000-000000000004', 'Tech Summit 2024', 'USA', 'San Francisco', 'Moscone Center', 2000, 1985, 'b0000000-0000-0000-0000-000000000002', '2024-09-15 09:00:00+00', '2024-02-04 13:00:00+00'),
('e0000000-0000-0000-0000-000000000005', 'AI Conference', 'Germany', 'Berlin', 'Messe Berlin', 500, 480, 'b0000000-0000-0000-0000-000000000002', '2024-10-20 10:00:00+00', '2024-02-05 14:00:00+00'),

-- Sports events (organizer: sports_org)
('e0000000-0000-0000-0000-000000000006', 'Marathon 2024', 'USA', 'Boston', 'Copley Square', 5000, 4950, 'b0000000-0000-0000-0000-000000000003', '2024-04-15 06:00:00+00', '2024-02-06 15:00:00+00'),
('e0000000-0000-0000-0000-000000000007', 'Tennis Championship', 'UK', 'London', 'Wimbledon', 300, 295, 'b0000000-0000-0000-0000-000000000003', '2024-07-01 11:00:00+00', '2024-02-07 16:00:00+00'),

-- Art exhibitions (organizer: art_gallery)
('e0000000-0000-0000-0000-000000000008', 'Modern Art Exhibition', 'France', 'Paris', 'Louvre Museum', 200, 195, 'b0000000-0000-0000-0000-000000000004', '2024-05-10 10:00:00+00', '2024-02-08 17:00:00+00'),
('e0000000-0000-0000-0000-000000000009', 'Photography Show', 'USA', 'New York', 'MoMA', 150, 148, 'b0000000-0000-0000-0000-000000000004', '2024-06-15 11:00:00+00', '2024-02-09 18:00:00+00'),

-- Food festivals (organizer: food_fest)
('e0000000-0000-0000-0000-000000000010', 'Street Food Festival', 'USA', 'Los Angeles', 'Grand Park', 1000, 980, 'b0000000-0000-0000-0000-000000000005', '2024-05-25 12:00:00+00', '2024-02-10 19:00:00+00'),
('e0000000-0000-0000-0000-000000000011', 'Wine Tasting Event', 'France', 'Bordeaux', 'Chateau Margaux', 50, 45, 'b0000000-0000-0000-0000-000000000005', '2024-09-05 14:00:00+00', '2024-02-11 20:00:00+00'),
('e0000000-0000-0000-0000-000000000012', 'Cooking Masterclass', 'Italy', 'Rome', 'Culinary Institute', 30, 28, 'b0000000-0000-0000-0000-000000000005', '2024-08-20 10:00:00+00', '2024-02-12 21:00:00+00');

-- ============================================================================
-- Bookings (15 records)
-- ============================================================================

INSERT INTO bookings (id, event_id, user_id, places_count, status, created_at) VALUES
-- Bookings for Summer Music Festival
('d0000000-0000-0000-0000-000000000001', 'e0000000-0000-0000-0000-000000000001', 'c0000000-0000-0000-0000-000000000001', 2, 'confirmed', '2024-03-01 10:00:00+00'),
('d0000000-0000-0000-0000-000000000002', 'e0000000-0000-0000-0000-000000000001', 'c0000000-0000-0000-0000-000000000002', 3, 'confirmed', '2024-03-02 11:00:00+00'),

-- Bookings for Jazz Night
('d0000000-0000-0000-0000-000000000003', 'e0000000-0000-0000-0000-000000000002', 'c0000000-0000-0000-0000-000000000003', 2, 'confirmed', '2024-03-03 12:00:00+00'),

-- Bookings for Rock Concert
('d0000000-0000-0000-0000-000000000004', 'e0000000-0000-0000-0000-000000000003', 'c0000000-0000-0000-0000-000000000004', 5, 'confirmed', '2024-03-04 13:00:00+00'),
('d0000000-0000-0000-0000-000000000005', 'e0000000-0000-0000-0000-000000000003', 'c0000000-0000-0000-0000-000000000005', 5, 'confirmed', '2024-03-05 14:00:00+00'),

-- Bookings for Tech Summit
('d0000000-0000-0000-0000-000000000006', 'e0000000-0000-0000-0000-000000000004', 'c0000000-0000-0000-0000-000000000001', 1, 'confirmed', '2024-03-06 15:00:00+00'),
('d0000000-0000-0000-0000-000000000007', 'e0000000-0000-0000-0000-000000000004', 'c0000000-0000-0000-0000-000000000006', 2, 'confirmed', '2024-03-07 16:00:00+00'),
('d0000000-0000-0000-0000-000000000008', 'e0000000-0000-0000-0000-000000000004', 'c0000000-0000-0000-0000-000000000007', 5, 'pending', '2024-03-08 17:00:00+00'),
('d0000000-0000-0000-0000-000000000009', 'e0000000-0000-0000-0000-000000000004', 'c0000000-0000-0000-0000-000000000008', 7, 'confirmed', '2024-03-09 18:00:00+00'),

-- Bookings for AI Conference
('d0000000-0000-0000-0000-000000000010', 'e0000000-0000-0000-0000-000000000005', 'c0000000-0000-0000-0000-000000000002', 10, 'confirmed', '2024-03-10 19:00:00+00'),
('d0000000-0000-0000-0000-000000000011', 'e0000000-0000-0000-0000-000000000005', 'c0000000-0000-0000-0000-000000000003', 10, 'confirmed', '2024-03-11 20:00:00+00'),

-- Bookings for Marathon
('d0000000-0000-0000-0000-000000000012', 'e0000000-0000-0000-0000-000000000006', 'c0000000-0000-0000-0000-000000000004', 1, 'confirmed', '2024-03-12 21:00:00+00'),
('d0000000-0000-0000-0000-000000000013', 'e0000000-0000-0000-0000-000000000006', 'c0000000-0000-0000-0000-000000000005', 1, 'cancelled', '2024-03-13 22:00:00+00'),

-- Bookings for other events
('d0000000-0000-0000-0000-000000000014', 'e0000000-0000-0000-0000-000000000008', 'c0000000-0000-0000-0000-000000000006', 2, 'confirmed', '2024-03-14 23:00:00+00'),
('d0000000-0000-0000-0000-000000000015', 'e0000000-0000-0000-0000-000000000010', 'c0000000-0000-0000-0000-000000000007', 4, 'confirmed', '2024-03-15 10:00:00+00');
