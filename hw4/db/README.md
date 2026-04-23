# Event Booking System - Database

This directory contains the PostgreSQL database schema, test data, and SQL queries for the Event Booking System.

## Overview

The Event Booking System is a microservices-based application that allows users to:
- Register and authenticate
- Create and manage events (organizers)
- Book places at events (participants)
- Cancel bookings

## Database Schema

### Tables

#### users
Stores all users of the system with their roles.

| Column | Type | Constraints | Description |
|--------|------|-------------|-------------|
| id | UUID | PK, DEFAULT gen_random_uuid() | Unique identifier |
| username | VARCHAR(50) | NOT NULL, UNIQUE | Login username (3-50 chars) |
| email | VARCHAR(255) | NOT NULL, UNIQUE | Email address |
| first_name | VARCHAR(100) | NOT NULL | First name |
| last_name | VARCHAR(100) | NOT NULL | Last name |
| password_hash | VARCHAR(255) | NOT NULL | Bcrypt password hash |
| role | VARCHAR(20) | NOT NULL, DEFAULT 'participant' | User role: participant, organizer, admin |
| created_at | TIMESTAMPTZ | NOT NULL, DEFAULT CURRENT_TIMESTAMP | Creation timestamp |
| updated_at | TIMESTAMPTZ | NOT NULL, DEFAULT CURRENT_TIMESTAMP | Last update timestamp |

#### events
Stores events created by organizers.

| Column | Type | Constraints | Description |
|--------|------|-------------|-------------|
| id | UUID | PK, DEFAULT gen_random_uuid() | Unique identifier |
| name | VARCHAR(200) | NOT NULL | Event name |
| country | VARCHAR(100) | NOT NULL | Country |
| city | VARCHAR(100) | NOT NULL | City |
| street | VARCHAR(255) | NOT NULL | Street address |
| places_count | INTEGER | NOT NULL, CHECK > 0 | Total places |
| available_places | INTEGER | NOT NULL, CHECK >= 0 | Available places |
| organizer_id | UUID | NOT NULL, FK → users(id) | Event organizer |
| event_time | TIMESTAMPTZ | NOT NULL | Event date/time |
| created_at | TIMESTAMPTZ | NOT NULL, DEFAULT CURRENT_TIMESTAMP | Creation timestamp |
| updated_at | TIMESTAMPTZ | NOT NULL, DEFAULT CURRENT_TIMESTAMP | Last update timestamp |

#### bookings
Stores bookings made by participants for events.

| Column | Type | Constraints | Description |
|--------|------|-------------|-------------|
| id | UUID | PK, DEFAULT gen_random_uuid() | Unique identifier |
| event_id | UUID | NOT NULL, FK → events(id) | Booked event |
| user_id | UUID | NOT NULL, FK → users(id) | Booking user |
| places_count | INTEGER | NOT NULL, DEFAULT 1, CHECK > 0 | Number of places |
| status | VARCHAR(20) | NOT NULL, DEFAULT 'pending' | Status: pending, confirmed, cancelled, failed |
| created_at | TIMESTAMPTZ | NOT NULL, DEFAULT CURRENT_TIMESTAMP | Creation timestamp |
| updated_at | TIMESTAMPTZ | NOT NULL, DEFAULT CURRENT_TIMESTAMP | Last update timestamp |

### Relationships

- `events.organizer_id` → `users.id` (many-to-one)
- `bookings.event_id` → `events.id` (many-to-one)
- `bookings.user_id` → `users.id` (many-to-one)

### Indexes

See [optimization.md](optimization.md) for detailed index analysis.

## Files

| File | Description |
|------|-------------|
| `schema.sql` | Database schema (tables, indexes, triggers) |
| `data.sql` | Test data (15 users, 12 events, 15 bookings) |
| `queries.sql` | SQL queries for all API operations |
| `optimization.md` | Query optimization analysis with EXPLAIN |

## Quick Start

### Using Docker Compose

1. Start PostgreSQL:
```bash
cd hw3
docker compose up -d postgres
```

2. Verify the database is ready:
```bash
docker exec event-booking-postgres psql -U eventbooking -d eventbooking -c "SELECT COUNT(*) FROM users;"
```

3. Connect to the database:
```bash
docker exec -it event-booking-postgres psql -U eventbooking -d eventbooking
```

### Connection Details

| Parameter | Value |
|-----------|-------|
| Host | localhost |
| Port | 5432 |
| Database | eventbooking |
| Username | eventbooking |
| Password | eventbooking_password |

### Connection String
```
postgresql://eventbooking:eventbooking_password@localhost:5432/eventbooking
```

## Manual Setup

If you want to set up the database manually:

1. Create the database:
```sql
CREATE DATABASE eventbooking;
CREATE USER eventbooking WITH PASSWORD 'eventbooking_password';
GRANT ALL PRIVILEGES ON DATABASE eventbooking TO eventbooking;
```

2. Run the schema:
```bash
psql -U eventbooking -d eventbooking -f schema.sql
```

3. Load test data:
```bash
psql -U eventbooking -d eventbooking -f data.sql
```

## Test Data

The `data.sql` file includes:

### Users (15 records)
- 2 admins: `admin`, `superadmin`
- 5 organizers: `music_events`, `tech_conf`, `sports_org`, `art_gallery`, `food_fest`
- 8 participants: `alice_wonder`, `bob_builder`, `charlie_chap`, etc.

**Default password for all users:** `password123`

### Events (12 records)
- Music events: Summer Music Festival, Jazz Night, Rock Concert
- Tech conferences: Tech Summit 2024, AI Conference
- Sports events: Marathon 2024, Tennis Championship
- Art exhibitions: Modern Art Exhibition, Photography Show
- Food festivals: Street Food Festival, Wine Tasting Event, Cooking Masterclass

### Bookings (15 records)
- Various bookings with different statuses (confirmed, pending, cancelled)

## API Operations

The database supports the following API operations:

### User Service (port 8080)
- `POST /register` - Register a new user
- `POST /login` - Authenticate user
- `POST /search` - Search users (admin only)

### Event Service (port 8081)
- `POST /events` - Create event (organizer/admin)
- `POST /events/list` - List events with filters
- `POST /events/book` - Reserve places (internal)
- `POST /events/cancel` - Release places (internal)

### Book Service (port 8082)
- `POST /bookings/book` - Create a booking
- `POST /bookings/cancel` - Cancel a booking

See [queries.sql](queries.sql) for the SQL queries used by each operation.
