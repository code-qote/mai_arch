-- ============================================================================
-- Event Booking System - Database Schema
-- ============================================================================
-- This schema supports an event booking system with three main entities:
-- 1. Users (participants, organizers, admins)
-- 2. Events (created by organizers)
-- 3. Bookings (made by participants for events)
-- ============================================================================

-- Drop existing tables if they exist (for clean setup)
DROP TABLE IF EXISTS bookings CASCADE;
DROP TABLE IF EXISTS events CASCADE;
DROP TABLE IF EXISTS users CASCADE;

-- ============================================================================
-- Tables
-- ============================================================================

-- Users table
-- Stores all users of the system with their roles
CREATE TABLE users (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    username VARCHAR(50) NOT NULL,
    email VARCHAR(255) NOT NULL,
    first_name VARCHAR(100) NOT NULL,
    last_name VARCHAR(100) NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    -- Using VARCHAR instead of ENUM for role to allow easier addition of new roles
    -- without requiring ALTER TYPE migration
    role VARCHAR(20) NOT NULL DEFAULT 'participant',
    created_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP,
    
    -- Constraints
    CONSTRAINT users_username_unique UNIQUE (username),
    CONSTRAINT users_email_unique UNIQUE (email),
    CONSTRAINT users_username_length CHECK (LENGTH(username) >= 3 AND LENGTH(username) <= 50),
    CONSTRAINT users_email_format CHECK (email ~* '^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$'),
    CONSTRAINT users_first_name_not_empty CHECK (LENGTH(TRIM(first_name)) > 0),
    CONSTRAINT users_last_name_not_empty CHECK (LENGTH(TRIM(last_name)) > 0),
    CONSTRAINT users_role_valid CHECK (role IN ('participant', 'organizer', 'admin'))
);

-- Events table
-- Stores events created by organizers
CREATE TABLE events (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    name VARCHAR(200) NOT NULL,
    country VARCHAR(100) NOT NULL,
    city VARCHAR(100) NOT NULL,
    street VARCHAR(255) NOT NULL,
    places_count INTEGER NOT NULL,
    available_places INTEGER NOT NULL,
    organizer_id UUID NOT NULL,
    event_time TIMESTAMP WITH TIME ZONE NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP,
    
    -- Constraints
    CONSTRAINT events_name_not_empty CHECK (LENGTH(TRIM(name)) > 0),
    CONSTRAINT events_places_positive CHECK (places_count > 0),
    CONSTRAINT events_available_places_valid CHECK (available_places >= 0 AND available_places <= places_count),
    CONSTRAINT events_country_not_empty CHECK (LENGTH(TRIM(country)) > 0),
    CONSTRAINT events_city_not_empty CHECK (LENGTH(TRIM(city)) > 0),
    CONSTRAINT events_street_not_empty CHECK (LENGTH(TRIM(street)) > 0),
    
    -- Foreign keys
    CONSTRAINT events_organizer_fk FOREIGN KEY (organizer_id) 
        REFERENCES users(id) ON DELETE RESTRICT ON UPDATE CASCADE
);

-- Bookings table
-- Stores bookings made by participants for events
CREATE TABLE bookings (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    event_id UUID NOT NULL,
    user_id UUID NOT NULL,
    places_count INTEGER NOT NULL DEFAULT 1,
    -- Using VARCHAR instead of ENUM for status to allow easier addition of new statuses
    -- without requiring ALTER TYPE migration
    status VARCHAR(20) NOT NULL DEFAULT 'pending',
    created_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT CURRENT_TIMESTAMP,
    
    -- Constraints
    CONSTRAINT bookings_places_positive CHECK (places_count > 0),
    CONSTRAINT bookings_status_valid CHECK (status IN ('pending', 'confirmed', 'cancelled', 'failed')),
    
    -- Foreign keys
    CONSTRAINT bookings_event_fk FOREIGN KEY (event_id) 
        REFERENCES events(id) ON DELETE RESTRICT ON UPDATE CASCADE,
    CONSTRAINT bookings_user_fk FOREIGN KEY (user_id) 
        REFERENCES users(id) ON DELETE RESTRICT ON UPDATE CASCADE
);

-- ============================================================================
-- Indexes
-- ============================================================================

-- Users indexes
-- Index for login by username (exact match, frequently used)
CREATE INDEX idx_users_username ON users(username);

-- Index for searching users by email
CREATE INDEX idx_users_email ON users(email);

-- Index for searching users by first_name and last_name (partial match search)
-- Using btree for LIKE 'prefix%' queries
CREATE INDEX idx_users_first_name ON users(first_name varchar_pattern_ops);
CREATE INDEX idx_users_last_name ON users(last_name varchar_pattern_ops);

-- Composite index for name search (first_name + last_name)
CREATE INDEX idx_users_full_name ON users(first_name, last_name);

-- Index for filtering by role
CREATE INDEX idx_users_role ON users(role);

-- Events indexes
-- Index for foreign key (organizer_id) - used in JOINs and filtering by organizer
CREATE INDEX idx_events_organizer_id ON events(organizer_id);

-- Index for searching events by city (frequently used filter)
CREATE INDEX idx_events_city ON events(city);

-- Index for searching events by country
CREATE INDEX idx_events_country ON events(country);

-- Composite index for location-based search (country + city)
CREATE INDEX idx_events_location ON events(country, city);

-- Index for searching events by name (partial match)
CREATE INDEX idx_events_name ON events(name varchar_pattern_ops);

-- Index for filtering events by date (for date range queries)
CREATE INDEX idx_events_event_time ON events(event_time);

-- Index for finding events with available places
CREATE INDEX idx_events_available_places ON events(available_places) WHERE available_places > 0;

-- Bookings indexes
-- Index for foreign key (event_id) - used in JOINs and filtering by event
CREATE INDEX idx_bookings_event_id ON bookings(event_id);

-- Index for foreign key (user_id) - used in JOINs and filtering by user
CREATE INDEX idx_bookings_user_id ON bookings(user_id);

-- Index for filtering bookings by status
CREATE INDEX idx_bookings_status ON bookings(status);

-- Composite index for finding user's bookings for a specific event
CREATE UNIQUE INDEX idx_bookings_user_event_active ON bookings(user_id, event_id) 
    WHERE status IN ('pending', 'confirmed');

-- Index for finding bookings by creation date (for history queries)
CREATE INDEX idx_bookings_created_at ON bookings(created_at DESC);

-- ============================================================================
-- Triggers for updated_at
-- ============================================================================

-- Function to update updated_at timestamp
CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = CURRENT_TIMESTAMP;
    RETURN NEW;
END;
$$ language 'plpgsql';

-- Trigger for users table
CREATE TRIGGER update_users_updated_at
    BEFORE UPDATE ON users
    FOR EACH ROW
    EXECUTE FUNCTION update_updated_at_column();

-- Trigger for events table
CREATE TRIGGER update_events_updated_at
    BEFORE UPDATE ON events
    FOR EACH ROW
    EXECUTE FUNCTION update_updated_at_column();

-- Trigger for bookings table
CREATE TRIGGER update_bookings_updated_at
    BEFORE UPDATE ON bookings
    FOR EACH ROW
    EXECUTE FUNCTION update_updated_at_column();
