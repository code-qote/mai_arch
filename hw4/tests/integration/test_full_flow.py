#!/usr/bin/env python3
"""
Integration test for the event booking system.

This test covers the full flow:
1. Register an organizer
2. Create an event (as organizer)
3. Register a user (participant)
4. Login as user
5. List events
6. Book the event
7. Cancel the booking
"""

import requests
import time
import os
import pytest
from typing import Dict, Any


# Service URLs
USER_SERVICE_URL = os.environ.get("USER_SERVICE_URL", "http://localhost:8080")
EVENT_SERVICE_URL = os.environ.get("EVENT_SERVICE_URL", "http://localhost:8081")
BOOK_SERVICE_URL = os.environ.get("BOOK_SERVICE_URL", "http://localhost:8082")

# Verbose mode for debugging
VERBOSE = os.environ.get("VERBOSE", "0") == "1"

# Test data
ORGANIZER_DATA = {
    "username": "test_organizer",
    "email": "organizer@test.com",
    "first_name": "Test",
    "last_name": "Organizer",
    "password": "secure_password_123",
    "role": "organizer"
}

USER_DATA = {
    "username": "test_user",
    "email": "user@test.com",
    "first_name": "Test",
    "last_name": "User",
    "password": "user_password_456",
    "role": "participant"
}

EVENT_DATA = {
    "name": "Test Concert",
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

# Global variable to store booking information for cancellation test
booking_info = {}

@pytest.fixture(scope="module")
def services_ready():
    """Wait for all services to be ready before running tests."""
    def wait_for_service(url: str, timeout: int = 30) -> bool:
        """Wait for a service to become available."""
        # Determine which endpoint to check based on the service URL
        if url == USER_SERVICE_URL:
            check_path = "/register"
            check_method = "POST"
            check_data = {}
        elif url == EVENT_SERVICE_URL:
            check_path = "/events/list"
            check_method = "POST"
            check_data = {}
        elif url == BOOK_SERVICE_URL:
            # Book service requires auth, so we'll just try a basic connection
            check_path = "/bookings"
            check_method = "POST"
            check_data = {}
        else:
            # Fallback to a simple GET request
            check_path = "/"
            check_method = "GET"
            check_data = None
        
        start_time = time.time()
        while time.time() - start_time < timeout:
            try:
                if check_method == "GET":
                    response = requests.get(f"{url}{check_path}", timeout=1)
                else:
                    response = requests.post(f"{url}{check_path}", json=check_data, timeout=1)
                
                # For services that require auth, we expect 401 or 403, not connection errors
                # For public endpoints, we expect 200 or 400 (bad request due to empty body)
                if response.status_code in [200, 400, 401, 403]:
                    return True
            except requests.RequestException:
                pass
            time.sleep(0.5)
        return False
    
    # Wait for services to be ready
    assert wait_for_service(USER_SERVICE_URL, timeout=30), "User service did not start in time"
    assert wait_for_service(EVENT_SERVICE_URL, timeout=30), "Event service did not start in time"
    assert wait_for_service(BOOK_SERVICE_URL, timeout=30), "Book service did not start in time"


@pytest.fixture(scope="module")
def organizer_token(services_ready) -> str:
    """Register an organizer and return the JWT token."""
    response = requests.post(
        f"{USER_SERVICE_URL}/register",
        json=ORGANIZER_DATA
    )
    
    if VERBOSE:
        print(f"  Response: {response.status_code} - {response.text[:100]}")
    
    assert response.status_code == 201, f"Expected 201, got {response.status_code}: {response.text}"
    
    data = response.json()
    assert "token" in data, "Response should contain token"
    
    return data["token"]


@pytest.fixture(scope="module")
def event_id(organizer_token) -> str:
    """Create an event and return the event ID."""
    response = requests.post(
        f"{EVENT_SERVICE_URL}/events",
        headers={"Authorization": f"Bearer {organizer_token}"},
        json=EVENT_DATA
    )
    
    if VERBOSE:
        print(f"  Response: {response.status_code} - {response.text[:100]}")
    
    assert response.status_code == 201, f"Expected 201, got {response.status_code}: {response.text}"
    
    data = response.json()
    assert "id" in data, "Response should contain event id"
    
    return data["id"]


@pytest.fixture(scope="module")
def user_token(services_ready) -> str:
    """Register a user and return the JWT token."""
    # Register user
    response = requests.post(
        f"{USER_SERVICE_URL}/register",
        json=USER_DATA
    )
    
    if VERBOSE:
        print(f"  Response: {response.status_code} - {response.text[:100]}")
    
    assert response.status_code == 201, f"Expected 201, got {response.status_code}: {response.text}"
    
    # Login user
    response = requests.post(
        f"{USER_SERVICE_URL}/login",
        json={
            "username": USER_DATA["username"],
            "password": USER_DATA["password"]
        }
    )
    
    if VERBOSE:
        print(f"  Response: {response.status_code} - {response.text[:100]}")
    
    assert response.status_code == 200, f"Expected 200, got {response.status_code}: {response.text}"
    
    data = response.json()
    assert "token" in data, "Response should contain token"
    
    return data["token"]


def test_organizer_registration(organizer_token: str):
    """Test that an organizer can be registered successfully."""
    # This test is implicitly covered by the organizer_token fixture
    assert organizer_token is not None and isinstance(organizer_token, str)


def test_event_creation(event_id: str):
    """Test that an event can be created successfully."""
    # This test is implicitly covered by the event_id fixture
    assert event_id is not None and isinstance(event_id, str)


def test_user_registration_and_login(user_token: str):
    """Test that a user can be registered and logged in successfully."""
    # This test is implicitly covered by the user_token fixture
    assert user_token is not None and isinstance(user_token, str)


def test_event_listing(event_id: str):
    """Test listing events and verify our event is present."""
    response = requests.post(
        f"{EVENT_SERVICE_URL}/events/list",
        json={}
    )
    
    if VERBOSE:
        print(f"  Response: {response.status_code} - {response.text[:100]}")
    
    assert response.status_code == 200, f"Expected 200, got {response.status_code}: {response.text}"
    
    data = response.json()
    assert "events" in data, "Response should contain events array"
    
    events = data["events"]
    assert len(events) > 0, "Should have at least one event"
    
    found_event = None
    for event in events:
        if event.get("id") == event_id:
            found_event = event
            break
    
    assert found_event is not None, f"Event {event_id} should be in the list"
    assert found_event["name"] == EVENT_DATA["name"], "Event name should match"
    # Store initial places count
    booking_info["initial_places"] = found_event["places_count"]


def test_event_booking(user_token: str, event_id: str):
    """Test booking an event and store booking info for cancellation."""
    response = requests.post(
        f"{BOOK_SERVICE_URL}/bookings",
        headers={"Authorization": f"Bearer {user_token}"},
        json={
            "event_id": event_id,
            **BOOKING_DATA
        }
    )
    
    if VERBOSE:
        print(f"  Response: {response.status_code} - {response.text[:100]}")
    
    assert response.status_code == 200, f"Expected 200, got {response.status_code}: {response.text}"
    
    data = response.json()
    assert data.get("success") == True, "Booking should be successful"
    
    # Store booking info for cancellation test
    assert "booking" in data, "Response should contain booking information"
    assert "id" in data["booking"], "Booking should have an ID"
    booking_info["id"] = data["booking"]["id"]
    booking_info["event_id"] = event_id


def test_booking_cancellation(user_token: str):
    """Test canceling a booking."""
    # Ensure we have a booking to cancel
    assert "id" in booking_info, "Booking ID should be available"
    assert "event_id" in booking_info, "Event ID should be available"
    
    response = requests.post(
        f"{BOOK_SERVICE_URL}/bookings/cancel",
        headers={"Authorization": f"Bearer {user_token}"},
        json={
            "booking_id": booking_info["id"],
            "event_id": booking_info["event_id"]
        }
    )
    
    if VERBOSE:
        print(f"  Response: {response.status_code} - {response.text[:100]}")
    
    assert response.status_code == 200, f"Expected 200, got {response.status_code}: {response.text}"
    
    data = response.json()
    assert data.get("success") == True, "Cancellation should be successful"
    assert "booking" in data, "Response should contain booking information"
    assert data["booking"].get("status") == "cancelled", "Booking status should be cancelled"


def test_event_availability_after_cancellation():
    """Test that event places are available after cancellation."""
    # Ensure we have an event to check
    assert "event_id" in booking_info, "Event ID should be available"
    assert "initial_places" in booking_info, "Initial places count should be available"
    
    response = requests.post(
        f"{EVENT_SERVICE_URL}/events/list",
        json={}
    )
    
    if VERBOSE:
        print(f"  Response: {response.status_code} - {response.text[:100]}")
    
    assert response.status_code == 200, f"Expected 200, got {response.status_code}: {response.text}"
    
    data = response.json()
    assert "events" in data, "Response should contain events array"
    
    events = data["events"]
    found_event = None
    for event in events:
        if event.get("id") == booking_info["event_id"]:
            found_event = event
            break
    
    assert found_event is not None, f"Event {booking_info['event_id']} should be in the list"
    # After cancellation, the places should be available again
    # Initially we had 100 places, booked 1, cancelled 1, so should have 100 again
    assert found_event.get("places_count") == booking_info["initial_places"], "All places should be available after cancellation"

if __name__ == "__main__":
    pytest.main([__file__, "-v"])
