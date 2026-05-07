# Event Management System - Variant 22

## Overview

This is an Event Management System implemented as a microservices architecture using C++20 and the userver framework. The system provides comprehensive event management capabilities including event creation, booking, cancellation, participant management, and user event tracking.

## Architecture

The system consists of the following microservices:

- **Event Service** (`event`): Manages event creation, listing, and event-related operations
- **Book Service** (`book`): Handles booking and cancellation operations
- **User Service** (`user`): Manages user registration, authentication, and search
- **Analytics Service** (`analytics`): Tracks event statistics and user activities

## Features

### Core Functionality

1. **Event Management**
   - Create events with detailed information (name, location, capacity, time)
   - List events with filtering capabilities (by ID, name, organizer, location, date range)
   - Event participants tracking

2. **Booking System**
   - Book places for events
   - Cancel bookings
   - Track booking status (pending, confirmed, cancelled, failed)

3. **User Management**
   - User registration with role-based access control
   - JWT-based authentication
   - User search functionality

4. **Analytics**
   - Event statistics tracking
   - User activity logging
   - Search query recording

### Performance Optimizations

#### Caching Strategy

The system implements a **Cache-Aside (Lazy Loading)** caching strategy using Redis 7:

- **Cache Keys**: Structured as `events:list:{id}:{name}:{organizer_id}:{country}:{city}:{date_from}:{date_to}`
- **TTL**: 5 minutes (300 seconds) for cached responses
- **Invalidation**: Pattern-based invalidation using `DeletePattern("events:list:*")` for bulk updates

**Cached Endpoints:**
- `GET /events/list` - Event listings with filters
- `GET /events/participants` - Event participants list
- `GET /events/user-events` - User's booked events

**Cache Invalidation Triggers:**
- Event creation → Invalidates all event list caches
- Event booking → Invalidates event list, participants, and user events caches
- Event cancellation → Invalidates event list, participants, and user events caches

#### Rate Limiting

The system implements **sliding window rate limiting** using Redis:

**Rate Limits by Endpoint:**
- `POST /events/list` - 1000 requests per 60 seconds
- `POST /events/create` - 1000 requests per 60 seconds
- `POST /events/book` - 1000 requests per 60 seconds
- `POST /events/cancel` - 1000 requests per 60 seconds
- `POST /events/participants` - 1000 requests per 60 seconds
- `POST /events/user-events` - 1000 requests per 60 seconds

**Implementation Details:**
- Redis-based distributed rate limiting using INCR operations
- Sliding window approach with time-based keys
- Automatic expiration of rate limit counters
- Per-user rate limiting for write operations
- Per-host rate limiting for read operations
- Graceful degradation: allows requests on Redis errors

## Technology Stack

- **Language**: C++20
- **Framework**: userver (Yandex's C++ framework)
- **Database**: PostgreSQL 15
- **Cache**: Redis 7 (Alpine)
- **Authentication**: JWT (JSON Web Tokens)
- **Build System**: CMake
- **Package Manager**: Conan
- **Containerization**: Docker & Docker Compose

## Project Structure

```
hw5/
├── common/                      # Shared components and utilities
│   ├── include/common/
│   │   ├── cache/              # Caching components
│   │   │   ├── redis_cache_component.hpp
│   │   │   └── rate_limiter.hpp
│   │   ├── database/           # Database models and interfaces
│   │   │   ├── db.hpp
│   │   │   ├── event.hpp
│   │   │   ├── booking.hpp
│   │   │   └── user.hpp
│   │   ├── jwt/                # JWT authentication
│   │   │   └── jwt_component.hpp
│   │   └── middleware/         # HTTP middleware
│   │       └── auth_middleware.hpp
│   └── src/
│       ├── cache/              # Cache implementations
│       │   ├── redis_cache_component.cpp
│       │   └── rate_limiter.cpp
│       └── database/           # Database implementations
│           └── db.cpp
├── services/
│   ├── event/                  # Event management service
│   │   ├── src/api/
│   │   │   ├── list/          # List events with filtering
│   │   │   ├── create/        # Create new events
│   │   │   ├── book/          # Book event places
│   │   │   ├── cancel/        # Cancel bookings
│   │   │   ├── participants/  # Get event participants
│   │   │   └── user_events/   # Get user's events
│   │   ├── main.cpp
│   │   ├── static_config.yaml
│   │   └── Dockerfile
│   ├── book/                   # Booking service
│   ├── user/                   # User management service
│   └── analytics/              # Analytics service
├── tests/
│   └── integration/            # Integration tests
│       ├── test_caching.py
│       ├── test_rate_limiting.py
│       ├── test_new_handlers.py
│       └── test_full_flow.py
├── db/                        # Database schemas and data
│   ├── schema.sql
│   └── data.sql
├── configs/                   # Configuration files
│   ├── secdist.json
│   └── redis.conf
├── docker-compose.yaml         # Service orchestration
├── Dockerfile                 # Main Docker image
├── CMakeLists.txt            # CMake configuration
├── conanfile.txt             # Conan dependencies
└── Makefile                  # Build automation
```

## API Endpoints

### Event Service (Port 8081)

#### List Events
```http
POST /events/list
Content-Type: application/json
Authorization: Bearer <token>

{
  "id": "",
  "name": "",
  "organizer_id": "",
  "country": "",
  "city": "",
  "date_from": "",
  "date_to": ""
}
```

#### Create Event
```http
POST /events/create
Content-Type: application/json
Authorization: Bearer <token>

{
  "name": "Event Name",
  "geo_position": {
    "country": "Russia",
    "city": "Moscow",
    "street": "Red Square 1"
  },
  "places_count": 100,
  "event_time": "2026-06-15T19:00:00Z"
}
```

#### Book Places
```http
POST /events/book
Content-Type: application/json
Authorization: Bearer <token>

{
  "event_id": "event-123",
  "places_count": 5
}
```

#### Cancel Booking
```http
POST /events/cancel
Content-Type: application/json
Authorization: Bearer <token>

{
  "event_id": "event-123",
  "places_count": 2
}
```

#### Get Event Participants
```http
POST /events/participants
Content-Type: application/json
Authorization: Bearer <token>

{
  "event_id": "event-123"
}
```

#### Get User Events
```http
POST /events/user-events
Content-Type: application/json
Authorization: Bearer <token>

{
  "user_id": "user-123"
}
```

### User Service (Port 8080)

#### Register User
```http
POST /register
Content-Type: application/json

{
  "username": "john_doe",
  "email": "john@example.com",
  "first_name": "John",
  "last_name": "Doe",
  "password": "secure_password",
  "role": "participant"
}
```

#### Login
```http
POST /login
Content-Type: application/json

{
  "username": "john_doe",
  "password": "secure_password"
}
```

#### Search Users
```http
POST /search
Content-Type: application/json
Authorization: Bearer <token>

{
  "username": "john",
  "email": "",
  "first_name": "",
  "last_name": ""
}
```

## Building and Running

### Prerequisites

- Docker and Docker Compose
- CMake 3.20+
- Conan 2.0+
- C++20 compatible compiler

### Build Instructions

```bash
# Build all services
make build

# Run integration tests
make integration-test

# Start all services
make up

# Stop all services
make down

# Clean build artifacts
make clean
```

### Running with Docker Compose

```bash
# Start all services (PostgreSQL, MongoDB, Redis, and microservices)
docker-compose up -d

# View logs
docker-compose logs -f

# Stop all services
docker-compose down
```

## Testing

### Integration Tests

The project includes comprehensive integration tests:

```bash
# Run all integration tests
make integration-test

# Run specific test file
pytest tests/integration/test_caching.py -v
pytest tests/integration/test_rate_limiting.py -v
pytest tests/integration/test_new_handlers.py -v
pytest tests/integration/test_full_flow.py -v
```

### Test Coverage

- **Caching Tests**: Verify cache hit/miss behavior and invalidation
- **Rate Limiting Tests**: Verify rate limit enforcement and reset
- **New Handler Tests**: Test participants and user events endpoints
- **Full Flow Tests**: End-to-end system testing

## Performance Considerations

### Caching Benefits

- **Reduced Database Load**: Cached responses reduce PostgreSQL queries
- **Faster Response Times**: Redis provides sub-millisecond response times
- **Scalability**: Distributed caching allows horizontal scaling

### Rate Limiting Benefits

- **Protection Against Abuse**: Prevents API abuse and DDoS attacks
- **Fair Resource Allocation**: Ensures fair access to all users
- **Cost Control**: Limits resource consumption and associated costs

### Optimization Strategies

1. **Connection Pooling**: Reuse database and Redis connections
2. **Async Operations**: Non-blocking I/O for better throughput
3. **Efficient Serialization**: Optimized JSON serialization
4. **Index Optimization**: Proper database indexes for fast queries

## Configuration

### Environment Variables

- `POSTGRES_HOST`: PostgreSQL server host
- `POSTGRES_PORT`: PostgreSQL server port
- `POSTGRES_DB`: Database name
- `POSTGRES_USER`: Database user
- `POSTGRES_PASSWORD`: Database password
- `REDIS_HOST`: Redis server host
- `REDIS_PORT`: Redis server port
- `JWT_SECRET`: Secret key for JWT token signing

### Service Ports

- User Service: 8080
- Event Service: 8081
- Book Service: 8082
- Analytics Service: 8083

## Security

- **Authentication**: JWT-based authentication with role-based access control
- **Authorization**: Role-based permissions (admin, organizer, participant)
- **Rate Limiting**: Protection against API abuse
- **Input Validation**: Request validation and sanitization
- **Secure Communication**: HTTPS support (configurable)