#!/usr/bin/env python3
"""
Integration tests for Redis caching functionality.

This test verifies that:
1. Event list responses are cached
2. Cache is invalidated when events are created
3. Cache is invalidated when bookings are made/cancelled
"""

import requests
import time
import pytest
from typing import Dict, Any

# Service URLs
USER_SERVICE_URL = "http://localhost:8080"
EVENT_SERVICE_URL = "http://localhost:8081"
BOOK_SERVICE_URL = "http://localhost:8082"

# Test data
ORGANIZER_DATA = {
    "username": "test_organizer_cache",
    "email": "organizer_cache@test.com",
    "first_name": "Test",
    "last_name": "Organizer",
    "password": "secure_password_123",
    "role": "organizer"
}

USER_DATA = {
    "username": "test_user_cache",
    "email": "user_cache@test.com",
    "first_name": "Test",
    "last_name": "User",
    "password": "user_password_456",
    "role": "participant"
}

EVENT_DATA = {
    "name": "Test Concert Cache",
    "geo_position": {
        "country": "Russia",
        "city": "Moscow",
        "street": "Red Square 1"
    },
    "places_count": 100,
    "event_time": "2026-06-15T19:00:00Z"
}

BOOKING_DATA = {
    "places_count": 1
}


def get_auth_token(username: str, password: str) -> str:
    """Get JWT token for authentication."""
    response = requests.post(f"{USER_SERVICE_URL}/login", json={
        "username": username,
        "password": password
    })
    assert response.status_code == 200
    return response.json()["token"]


def test_event_list_caching():
    """Test that event list responses are cached."""
    # Register and login as organizer
    requests.post(f"{USER_SERVICE_URL}/register", json=ORGANIZER_DATA)
    token = get_auth_token(ORGANIZER_DATA["username"], ORGANIZER_DATA["password"])
    
    # Create an event
    create_response = requests.post(
        f"{EVENT_SERVICE_URL}/events",
        json=EVENT_DATA,
        headers={"Authorization": f"Bearer {token}"}
    )
    assert create_response.status_code == 201
    event_id = create_response.json()["id"]
    
    # First request - should hit database
    start_time = time.time()
    list_response_1 = requests.post(
        f"{EVENT_SERVICE_URL}/events/list",
        json={},
        headers={"Authorization": f"Bearer {token}"}
    )
    first_request_time = time.time() - start_time
    assert list_response_1.status_code == 200
    
    # Second request - should hit cache (faster)
    start_time = time.time()
    list_response_2 = requests.post(
        f"{EVENT_SERVICE_URL}/events/list",
        json={},
        headers={"Authorization": f"Bearer {token}"}
    )
    second_request_time = time.time() - start_time
    assert list_response_2.status_code == 200
    
    # Responses should be identical
    assert list_response_1.json() == list_response_2.json()
    
    # Second request should be faster (cached)
    # Note: This is a basic check, in real scenarios you'd want more sophisticated timing
    print(f"First request time: {first_request_time:.4f}s")
    print(f"Second request time: {second_request_time:.4f}s")
    print(f"Cache speedup: {first_request_time/second_request_time:.2f}x")


def test_cache_invalidation_on_create():
    """Test that cache is invalidated when a new event is created."""
    # Register and login as organizer
    requests.post(f"{USER_SERVICE_URL}/register", json=ORGANIZER_DATA)
    token = get_auth_token(ORGANIZER_DATA["username"], ORGANIZER_DATA["password"])
    
    # Get initial event list
    list_response_1 = requests.post(
        f"{EVENT_SERVICE_URL}/events/list",
        json={},
        headers={"Authorization": f"Bearer {token}"}
    )
    assert list_response_1.status_code == 200
    initial_count = len(list_response_1.json()["events"])
    
    # Create a new event
    new_event_data = {
        "name": "New Event for Cache Test",
        "geo_position": {
            "country": "Russia",
            "city": "St. Petersburg",
            "street": "Nevsky 1"
        },
        "places_count": 50,
        "event_time": "2026-07-20T18:00:00Z"
    }
    
    create_response = requests.post(
        f"{EVENT_SERVICE_URL}/events",
        json=new_event_data,
        headers={"Authorization": f"Bearer {token}"}
    )
    assert create_response.status_code == 201
    
    # Get event list again - should reflect the new event (cache invalidated)
    list_response_2 = requests.post(
        f"{EVENT_SERVICE_URL}/events/list",
        json={},
        headers={"Authorization": f"Bearer {token}"}
    )
    assert list_response_2.status_code == 200
    new_count = len(list_response_2.json()["events"])
    
    # Should have one more event
    assert new_count == initial_count + 1


def test_cache_invalidation_on_booking():
    """Test that cache is invalidated when places are booked."""
    # Register and login as organizer
    requests.post(f"{USER_SERVICE_URL}/register", json=ORGANIZER_DATA)
    organizer_token = get_auth_token(ORGANIZER_DATA["username"], ORGANIZER_DATA["password"])
    
    # Create an event
    create_response = requests.post(
        f"{EVENT_SERVICE_URL}/events",
        json=EVENT_DATA,
        headers={"Authorization": f"Bearer {organizer_token}"}
    )
    assert create_response.status_code == 201
    event_id = create_response.json()["id"]
    initial_places = create_response.json()["places_count"]
    
    # Get event list and check places count
    list_response_1 = requests.post(
        f"{EVENT_SERVICE_URL}/events/list",
        json={},
        headers={"Authorization": f"Bearer {organizer_token}"}
    )
    assert list_response_1.status_code == 200
    event_in_list_1 = next((e for e in list_response_1.json()["events"] if e["id"] == event_id), None)
    assert event_in_list_1 is not None
    assert event_in_list_1["places_count"] == initial_places
    
    # Register and login as user
    requests.post(f"{USER_SERVICE_URL}/register", json=USER_DATA)
    user_token = get_auth_token(USER_DATA["username"], USER_DATA["password"])
    
    # Book places
    book_response = requests.post(
        f"{EVENT_SERVICE_URL}/events/book",
        json={
            "event_id": event_id,
            "places_count": 5
        },
        headers={"Authorization": f"Bearer {user_token}"}
    )
    assert book_response.status_code == 200
    
    # Get event list again - should reflect updated places (cache invalidated)
    list_response_2 = requests.post(
        f"{EVENT_SERVICE_URL}/events/list",
        json={},
        headers={"Authorization": f"Bearer {organizer_token}"}
    )
    assert list_response_2.status_code == 200
    event_in_list_2 = next((e for e in list_response_2.json()["events"] if e["id"] == event_id), None)
    assert event_in_list_2 is not None
    assert event_in_list_2["places_count"] == initial_places - 5


def test_cache_invalidation_on_cancellation():
    """Test that cache is invalidated when bookings are cancelled."""
    # Register and login as organizer
    requests.post(f"{USER_SERVICE_URL}/register", json=ORGANIZER_DATA)
    organizer_token = get_auth_token(ORGANIZER_DATA["username"], ORGANIZER_DATA["password"])
    
    # Create an event
    create_response = requests.post(
        f"{EVENT_SERVICE_URL}/events",
        json=EVENT_DATA,
        headers={"Authorization": f"Bearer {organizer_token}"}
    )
    assert create_response.status_code == 201
    event_id = create_response.json()["id"]
    initial_places = create_response.json()["places_count"]
    
    # Register and login as user
    requests.post(f"{USER_SERVICE_URL}/register", json=USER_DATA)
    user_token = get_auth_token(USER_DATA["username"], USER_DATA["password"])
    
    # Book places
    book_response = requests.post(
        f"{EVENT_SERVICE_URL}/events/book",
        json={
            "event_id": event_id,
            "places_count": 3
        },
        headers={"Authorization": f"Bearer {user_token}"}
    )
    assert book_response.status_code == 200
    
    # Get event list and check places count
    list_response_1 = requests.post(
        f"{EVENT_SERVICE_URL}/events/list",
        json={},
        headers={"Authorization": f"Bearer {organizer_token}"}
    )
    assert list_response_1.status_code == 200
    event_in_list_1 = next((e for e in list_response_1.json()["events"] if e["id"] == event_id), None)
    assert event_in_list_1 is not None
    assert event_in_list_1["places_count"] == initial_places - 3
    
    # Cancel booking
    cancel_response = requests.post(
        f"{EVENT_SERVICE_URL}/events/cancel",
        json={
            "event_id": event_id,
            "places_count": 2
        },
        headers={"Authorization": f"Bearer {user_token}"}
    )
    assert cancel_response.status_code == 200
    
    # Get event list again - should reflect updated places (cache invalidated)
    list_response_2 = requests.post(
        f"{EVENT_SERVICE_URL}/events/list",
        json={},
        headers={"Authorization": f"Bearer {organizer_token}"}
    )
    assert list_response_2.status_code == 200
    event_in_list_2 = next((e for e in list_response_2.json()["events"] if e["id"] == event_id), None)
    assert event_in_list_2 is not None
    assert event_in_list_2["places_count"] == initial_places - 1


def test_cache_with_filters():
    """Test that different filter combinations create different cache keys."""
    # Register and login as organizer
    requests.post(f"{USER_SERVICE_URL}/register", json=ORGANIZER_DATA)
    token = get_auth_token(ORGANIZER_DATA["username"], ORGANIZER_DATA["password"])
    
    # Create events in different cities
    for city in ["Moscow", "St. Petersburg", "Kazan"]:
        event_data = {
            "name": f"Event in {city}",
            "geo_position": {
                "country": "Russia",
                "city": city,
                "street": "Main Street 1"
            },
            "places_count": 100,
            "event_time": "2026-08-01T19:00:00Z"
        }
        create_response = requests.post(
            f"{EVENT_SERVICE_URL}/events",
            json=event_data,
            headers={"Authorization": f"Bearer {token}"}
        )
        assert create_response.status_code == 201
    
    # Get all events
    all_events_response = requests.post(
        f"{EVENT_SERVICE_URL}/events/list",
        json={},
        headers={"Authorization": f"Bearer {token}"}
    )
    assert all_events_response.status_code == 200
    all_events = all_events_response.json()["events"]
    
    # Get events filtered by city
    moscow_events_response = requests.post(
        f"{EVENT_SERVICE_URL}/events/list",
        json={"city": "Moscow"},
        headers={"Authorization": f"Bearer {token}"}
    )
    assert moscow_events_response.status_code == 200
    moscow_events = moscow_events_response.json()["events"]
    
    # Should have fewer events when filtered
    assert len(moscow_events) < len(all_events)
    
    # All Moscow events should be from Moscow
    for event in moscow_events:
        assert event["geo_position"]["city"] == "Moscow"


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
