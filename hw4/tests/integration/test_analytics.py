"""Integration tests for Analytics Service with MongoDB."""

import pytest
import requests
from datetime import datetime, timezone


class TestAnalyticsService:
    """Test suite for Analytics Service MongoDB operations."""

    BASE_URL = "http://localhost:8083"

    def test_log_activity(self):
        """Test logging user activity."""
        activity_data = {
            "user_id": "test-user-1",
            "activity_type": "booking_created",
            "event_id": "event-123",
            "event_name": "Test Event",
            "booking_id": "booking-456",
            "places_count": 2,
            "amount": 100.0,
            "session_id": "session-789",
            "ip_address": "192.168.1.1",
            "user_agent": "pytest/1.0"
        }

        response = requests.post(
            f"{self.BASE_URL}/analytics/log",
            json=activity_data
        )

        assert response.status_code == 201
        data = response.json()
        assert data["success"] is True
        assert "message" in data

    def test_log_activity_minimal(self):
        """Test logging activity with minimal required fields."""
        activity_data = {
            "user_id": "test-user-2",
            "activity_type": "user_login"
        }

        response = requests.post(
            f"{self.BASE_URL}/analytics/log",
            json=activity_data
        )

        assert response.status_code == 201
        data = response.json()
        assert data["success"] is True

    def test_log_activity_invalid_method(self):
        """Test that GET method is not allowed for logging."""
        response = requests.get(f"{self.BASE_URL}/analytics/log")
        assert response.status_code == 405

    def test_record_search(self):
        """Test recording search history."""
        search_data = {
            "user_id": "test-user-1",
            "city": "New York",
            "country": "USA",
            "name_keyword": "music",
            "results_count": 15,
            "clicked_event_ids": ["event-1", "event-2", "event-3"],
            "booked_event_id": "event-1",
            "conversion": True,
            "session_id": "session-123"
        }

        response = requests.post(
            f"{self.BASE_URL}/analytics/search",
            json=search_data
        )

        assert response.status_code == 201
        data = response.json()
        assert data["success"] is True

    def test_record_search_no_conversion(self):
        """Test recording search without conversion."""
        search_data = {
            "user_id": "test-user-2",
            "city": "Los Angeles",
            "results_count": 8,
            "clicked_event_ids": ["event-4"],
            "conversion": False,
            "session_id": "session-456"
        }

        response = requests.post(
            f"{self.BASE_URL}/analytics/search",
            json=search_data
        )

        assert response.status_code == 201
        data = response.json()
        assert data["success"] is True

    def test_update_event_statistics(self):
        """Test updating event statistics."""
        stats_data = {
            "event_id": "event-test-1",
            "event_name": "Test Event",
            "organizer_id": "organizer-1",
            "total_bookings": 75,
            "total_participants": 150,
            "total_places_booked": 320,
            "total_places_available": 500,
            "occupancy_rate": 0.64,
            "cancelled_bookings": 5,
            "cancellation_rate": 0.067,
            "average_places_per_booking": 4.27,
            "total_revenue": 15000.0
        }

        response = requests.post(
            f"{self.BASE_URL}/analytics/stats",
            json=stats_data
        )

        assert response.status_code == 200
        data = response.json()
        assert data["success"] is True

    def test_get_event_statistics(self):
        """Test retrieving event statistics."""
        # First, create statistics
        stats_data = {
            "event_id": "event-test-2",
            "event_name": "Test Event 2",
            "organizer_id": "organizer-2",
            "total_bookings": 100,
            "total_participants": 200,
            "total_places_booked": 450,
            "total_places_available": 600,
            "occupancy_rate": 0.75,
            "cancelled_bookings": 10,
            "cancellation_rate": 0.1,
            "average_places_per_booking": 4.5,
            "total_revenue": 25000.0
        }

        create_response = requests.post(
            f"{self.BASE_URL}/analytics/stats",
            json=stats_data
        )
        assert create_response.status_code == 200

        # Now retrieve them
        get_response = requests.get(
            f"{self.BASE_URL}/analytics/stats",
            json={"event_id": "event-test-2"}
        )

        assert get_response.status_code == 200
        data = get_response.json()
        assert data["event_id"] == "event-test-2"
        assert data["total_bookings"] == 100
        assert data["occupancy_rate"] == 0.75

    def test_get_top_events_by_occupancy(self):
        """Test retrieving top events by occupancy rate."""
        # Create multiple events with different occupancy rates
        events = [
            {
                "event_id": f"event-top-{i}",
                "event_name": f"Event {i}",
                "organizer_id": "organizer-1",
                "total_bookings": 50 + i * 10,
                "total_participants": 100 + i * 20,
                "total_places_booked": 200 + i * 50,
                "total_places_available": 500,
                "occupancy_rate": 0.4 + i * 0.1,
                "cancelled_bookings": 5,
                "cancellation_rate": 0.05,
                "average_places_per_booking": 4.0,
                "total_revenue": 10000.0 + i * 5000
            }
            for i in range(5)
        ]

        for event in events:
            response = requests.post(
                f"{self.BASE_URL}/analytics/stats",
                json=event
            )
            assert response.status_code == 200

        # Get top events
        response = requests.get(
            f"{self.BASE_URL}/analytics/stats",
            json={"top": True, "limit": 3}
        )

        assert response.status_code == 200
        data = response.json()
        assert "events" in data
        assert len(data["events"]) <= 3
        # Verify they're sorted by occupancy rate (descending)
        if len(data["events"]) > 1:
            for i in range(len(data["events"]) - 1):
                assert data["events"][i]["occupancy_rate"] >= data["events"][i + 1]["occupancy_rate"]

    def test_get_statistics_not_found(self):
        """Test retrieving statistics for non-existent event."""
        response = requests.get(
            f"{self.BASE_URL}/analytics/stats",
            json={"event_id": "non-existent-event"}
        )

        assert response.status_code == 404
        data = response.json()
        assert "error" in data

    def test_invalid_request_format(self):
        """Test handling of invalid request format."""
        response = requests.post(
            f"{self.BASE_URL}/analytics/log",
            json={"invalid": "data"}
        )

        assert response.status_code == 400
        data = response.json()
        assert "error" in data

    def test_activity_types(self):
        """Test logging different activity types."""
        activity_types = [
            "user_registered",
            "user_login",
            "event_created",
            "event_viewed",
            "event_searched",
            "booking_created",
            "booking_cancelled"
        ]

        for activity_type in activity_types:
            activity_data = {
                "user_id": f"test-user-{activity_type}",
                "activity_type": activity_type
            }

            response = requests.post(
                f"{self.BASE_URL}/analytics/log",
                json=activity_data
            )

            assert response.status_code == 201, f"Failed for activity type: {activity_type}"


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
