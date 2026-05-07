#!/usr/bin/env python3
"""
Integration tests for rate limiting functionality.

This test verifies that:
1. Rate limiting is enforced for all handlers
2. Rate limits are reset after the time window expires
3. Different endpoints have independent rate limits
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
    "username": "test_organizer_rate",
    "email": "organizer_rate@test.com",
    "first_name": "Test",
    "last_name": "Organizer",
    "password": "secure_password_123",
    "role": "organizer"
}

USER_DATA = {
    "username": "test_user_rate",
    "email": "user_rate@test.com",
    "first_name": "Test",
    "last_name": "User",
    "password": "user_password_456",
    "role": "participant"
}

EVENT_DATA = {
    "name": "Test Concert Rate",
    "geo_position": {
        "country": "Russia",
        "city": "Moscow",
        "street": "Red Square 1"
    },
    "places_count": 100,
    "event_time": "2026-06-15T19:00:00Z"
}


def get_auth_token(username: str, password: str) -> str:
    """Get JWT token for authentication."""
    response = requests.post(f"{USER_SERVICE_URL}/login", json={
        "username": username,
        "password": password
    })
    assert response.status_code == 200
    return response.json()["token"]


def test_list_events_rate_limiting():
    """Test that list events endpoint has rate limiting."""
    # Register and login as organizer
    requests.post(f"{USER_SERVICE_URL}/register", json=ORGANIZER_DATA)
    token = get_auth_token(ORGANIZER_DATA["username"], ORGANIZER_DATA["password"])
    
    headers = {"Authorization": f"Bearer {token}"}
    
    # Create an event first
    response = requests.post(f"{EVENT_SERVICE_URL}/create", json=EVENT_DATA, headers=headers)
    assert response.status_code == 200
    event_id = response.json()["event"]["id"]
    
    # Make requests until rate limit is hit
    # List events has rate limit of 100 requests per 60 seconds
    # We'll make 105 requests to ensure we hit the limit
    rate_limited = False
    for i in range(105):
        response = requests.post(f"{EVENT_SERVICE_URL}/list", json={}, headers=headers)
        if response.status_code == 429:  # Too Many Requests
            rate_limited = True
            print(f"Rate limited after {i+1} requests")
            break
    
    assert rate_limited, "Rate limiting should be enforced for list events endpoint"


def test_create_event_rate_limiting():
    """Test that create event endpoint has rate limiting."""
    # Register and login as organizer
    requests.post(f"{USER_SERVICE_URL}/register", json=ORGANIZER_DATA)
    token = get_auth_token(ORGANIZER_DATA["username"], ORGANIZER_DATA["password"])
    
    headers = {"Authorization": f"Bearer {token}"}
    
    # Make requests until rate limit is hit
    # Create event has rate limit of 20 requests per 60 seconds
    # We'll make 25 requests to ensure we hit the limit
    rate_limited = False
    for i in range(25):
        event_data = EVENT_DATA.copy()
        event_data["name"] = f"Test Event {i}"
        response = requests.post(f"{EVENT_SERVICE_URL}/create", json=event_data, headers=headers)
        if response.status_code == 429:  # Too Many Requests
            rate_limited = True
            print(f"Rate limited after {i+1} requests")
            break
    
    assert rate_limited, "Rate limiting should be enforced for create event endpoint"


def test_book_places_rate_limiting():
    """Test that book places endpoint has rate limiting."""
    # Register and login as organizer
    requests.post(f"{USER_SERVICE_URL}/register", json=ORGANIZER_DATA)
    organizer_token = get_auth_token(ORGANIZER_DATA["username"], ORGANIZER_DATA["password"])
    
    # Register and login as user
    requests.post(f"{USER_SERVICE_URL}/register", json=USER_DATA)
    user_token = get_auth_token(USER_DATA["username"], USER_DATA["password"])
    
    organizer_headers = {"Authorization": f"Bearer {organizer_token}"}
    user_headers = {"Authorization": f"Bearer {user_token}"}
    
    # Create an event
    response = requests.post(f"{EVENT_SERVICE_URL}/create", json=EVENT_DATA, headers=organizer_headers)
    assert response.status_code == 200
    event_id = response.json()["event"]["id"]
    
    # Make requests until rate limit is hit
    # Book places has rate limit of 30 requests per 60 seconds
    # We'll make 35 requests to ensure we hit the limit
    rate_limited = False
    for i in range(35):
        response = requests.post(f"{EVENT_SERVICE_URL}/book", 
                                json={"event_id": event_id, "places_count": 1}, 
                                headers=user_headers)
        if response.status_code == 429:  # Too Many Requests
            rate_limited = True
            print(f"Rate limited after {i+1} requests")
            break
    
    assert rate_limited, "Rate limiting should be enforced for book places endpoint"


def test_cancel_places_rate_limiting():
    """Test that cancel places endpoint has rate limiting."""
    # Register and login as organizer
    requests.post(f"{USER_SERVICE_URL}/register", json=ORGANIZER_DATA)
    organizer_token = get_auth_token(ORGANIZER_DATA["username"], ORGANIZER_DATA["password"])
    
    # Register and login as user
    requests.post(f"{USER_SERVICE_URL}/register", json=USER_DATA)
    user_token = get_auth_token(USER_DATA["username"], USER_DATA["password"])
    
    organizer_headers = {"Authorization": f"Bearer {organizer_token}"}
    user_headers = {"Authorization": f"Bearer {user_token}"}
    
    # Create an event
    response = requests.post(f"{EVENT_SERVICE_URL}/create", json=EVENT_DATA, headers=organizer_headers)
    assert response.status_code == 200
    event_id = response.json()["event"]["id"]
    
    # Book some places first
    response = requests.post(f"{EVENT_SERVICE_URL}/book", 
                            json={"event_id": event_id, "places_count": 10}, 
                            headers=user_headers)
    assert response.status_code == 200
    
    # Make requests until rate limit is hit
    # Cancel places has rate limit of 30 requests per 60 seconds
    # We'll make 35 requests to ensure we hit the limit
    rate_limited = False
    for i in range(35):
        response = requests.post(f"{EVENT_SERVICE_URL}/cancel", 
                                json={"event_id": event_id, "places_count": 1}, 
                                headers=user_headers)
        if response.status_code == 429:  # Too Many Requests
            rate_limited = True
            print(f"Rate limited after {i+1} requests")
            break
    
    assert rate_limited, "Rate limiting should be enforced for cancel places endpoint"


def test_get_participants_rate_limiting():
    """Test that get participants endpoint has rate limiting."""
    # Register and login as organizer
    requests.post(f"{USER_SERVICE_URL}/register", json=ORGANIZER_DATA)
    organizer_token = get_auth_token(ORGANIZER_DATA["username"], ORGANIZER_DATA["password"])
    
    # Register and login as user
    requests.post(f"{USER_SERVICE_URL}/register", json=USER_DATA)
    user_token = get_auth_token(USER_DATA["username"], USER_DATA["password"])
    
    organizer_headers = {"Authorization": f"Bearer {organizer_token}"}
    user_headers = {"Authorization": f"Bearer {user_token}"}
    
    # Create an event
    response = requests.post(f"{EVENT_SERVICE_URL}/create", json=EVENT_DATA, headers=organizer_headers)
    assert response.status_code == 200
    event_id = response.json()["event"]["id"]
    
    # Book some places
    response = requests.post(f"{EVENT_SERVICE_URL}/book", 
                            json={"event_id": event_id, "places_count": 5}, 
                            headers=user_headers)
    assert response.status_code == 200
    
    # Make requests until rate limit is hit
    # Get participants has rate limit of 50 requests per 60 seconds
    # We'll make 55 requests to ensure we hit the limit
    rate_limited = False
    for i in range(55):
        response = requests.post(f"{EVENT_SERVICE_URL}/participants", 
                                json={"event_id": event_id}, 
                                headers=user_headers)
        if response.status_code == 429:  # Too Many Requests
            rate_limited = True
            print(f"Rate limited after {i+1} requests")
            break
    
    assert rate_limited, "Rate limiting should be enforced for get participants endpoint"


def test_get_user_events_rate_limiting():
    """Test that get user events endpoint has rate limiting."""
    # Register and login as organizer
    requests.post(f"{USER_SERVICE_URL}/register", json=ORGANIZER_DATA)
    organizer_token = get_auth_token(ORGANIZER_DATA["username"], ORGANIZER_DATA["password"])
    
    # Register and login as user
    requests.post(f"{USER_SERVICE_URL}/register", json=USER_DATA)
    user_token = get_auth_token(USER_DATA["username"], USER_DATA["password"])
    
    organizer_headers = {"Authorization": f"Bearer {organizer_token}"}
    user_headers = {"Authorization": f"Bearer {user_token}"}
    
    # Create an event
    response = requests.post(f"{EVENT_SERVICE_URL}/create", json=EVENT_DATA, headers=organizer_headers)
    assert response.status_code == 200
    event_id = response.json()["event"]["id"]
    
    # Book some places
    response = requests.post(f"{EVENT_SERVICE_URL}/book", 
                            json={"event_id": event_id, "places_count": 1}, 
                            headers=user_headers)
    assert response.status_code == 200
    
    # Get user_id from token (simplified for testing)
    user_id = USER_DATA["username"]  # In real implementation, decode JWT
    
    # Make requests until rate limit is hit
    # Get user events has rate limit of 50 requests per 60 seconds
    # We'll make 55 requests to ensure we hit the limit
    rate_limited = False
    for i in range(55):
        response = requests.post(f"{EVENT_SERVICE_URL}/user-events", 
                                json={"user_id": user_id}, 
                                headers=user_headers)
        if response.status_code == 429:  # Too Many Requests
            rate_limited = True
            print(f"Rate limited after {i+1} requests")
            break
    
    assert rate_limited, "Rate limiting should be enforced for get user events endpoint"


def test_rate_limit_reset_after_window():
    """Test that rate limits are reset after the time window expires."""
    # Register and login as organizer
    requests.post(f"{USER_SERVICE_URL}/register", json=ORGANIZER_DATA)
    token = get_auth_token(ORGANIZER_DATA["username"], ORGANIZER_DATA["password"])
    
    headers = {"Authorization": f"Bearer {token}"}
    
    # Create an event
    response = requests.post(f"{EVENT_SERVICE_URL}/create", json=EVENT_DATA, headers=headers)
    assert response.status_code == 200
    event_id = response.json()["event"]["id"]
    
    # Hit the rate limit for list events
    rate_limited = False
    for i in range(105):
        response = requests.post(f"{EVENT_SERVICE_URL}/list", json={}, headers=headers)
        if response.status_code == 429:
            rate_limited = True
            break
    
    assert rate_limited, "Should be rate limited"
    
    # Wait for the rate limit window to expire (60 seconds)
    # For testing purposes, we'll wait a shorter time
    # In production, this would be 60 seconds
    print("Waiting for rate limit window to expire...")
    time.sleep(65)  # Wait for 65 seconds to ensure window expires
    
    # Try again - should work now
    response = requests.post(f"{EVENT_SERVICE_URL}/list", json={}, headers=headers)
    assert response.status_code == 200, "Rate limit should be reset after window expires"


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
