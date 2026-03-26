#include <common/database/db.hpp>

#include <userver/utest/utest.hpp>

UTEST(InMemoryDb, SuccessfulRegistration) {
    common::database::InMemoryDb db;

    auto result = db.RegisterUser("testuser", "test@example.com", "name", "surname", "password123");

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.error.empty());
    EXPECT_EQ(result.user.username, "testuser");
    EXPECT_EQ(result.user.email, "test@example.com");
    EXPECT_FALSE(result.user.id.empty());
    EXPECT_FALSE(result.user.created_at.empty());
    // Password should be hashed, not stored as plain text
    EXPECT_NE(result.user.password_hash, "password123");
    EXPECT_FALSE(result.user.password_hash.empty());
}

UTEST(InMemoryDb, EmptyUsername) {
    common::database::InMemoryDb db;

    auto result = db.RegisterUser("", "test@example.com", "name", "surname", "password123");

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "Username, email, and password are required");
}

UTEST(InMemoryDb, EmptyEmail) {
    common::database::InMemoryDb db;

    auto result = db.RegisterUser("testuser", "", "name", "surname", "password123");

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "Username, email, and password are required");
}

UTEST(InMemoryDb, EmptyPassword) {
    common::database::InMemoryDb db;

    auto result = db.RegisterUser("testuser", "test@example.com", "name", "surname", "");

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "Username, email, and password are required");
}

UTEST(InMemoryDb, DuplicateUsername) {
    common::database::InMemoryDb db;

    auto result1 = db.RegisterUser("duplicateuser", "test1@example.com", "name", "surname", "password123");
    EXPECT_TRUE(result1.success);

    auto result2 = db.RegisterUser("duplicateuser", "test2@example.com", "name", "surname", "password456");
    EXPECT_FALSE(result2.success);
    EXPECT_EQ(result2.error, "User with this username or email already exists");
}

UTEST(InMemoryDb, DuplicateEmail) {
    common::database::InMemoryDb db;

    auto result1 = db.RegisterUser("user1", "duplicate@example.com", "name", "surname", "password123");
    EXPECT_TRUE(result1.success);

    auto result2 = db.RegisterUser("user2", "duplicate@example.com", "name", "surname", "password456");
    EXPECT_FALSE(result2.success);
    EXPECT_EQ(result2.error, "User with this username or email already exists");
}

UTEST(InMemoryDb, MultipleUsers) {
    common::database::InMemoryDb db;

    auto r1 = db.RegisterUser("user1", "user1@example.com", "name", "surname", "pass1");
    auto r2 = db.RegisterUser("user2", "user2@example.com", "name", "surname", "pass2");
    auto r3 = db.RegisterUser("user3", "user3@example.com", "name", "surname", "pass3");

    EXPECT_TRUE(r1.success);
    EXPECT_TRUE(r2.success);
    EXPECT_TRUE(r3.success);
    EXPECT_EQ(db.UserCount(), 3);
}

UTEST(InMemoryDb, FindUsersByUsername) {
    common::database::InMemoryDb db;

    db.RegisterUser("findme", "findme@example.com", "name", "surname", "password123");

    common::database::UserFilter filter;
    filter.username = "findme";
    auto found = db.FindUsers(filter);
    ASSERT_EQ(found.size(), 1);
    EXPECT_EQ(found[0].username, "findme");
    EXPECT_EQ(found[0].email, "findme@example.com");

    filter.username = "nonexistent";
    auto not_found = db.FindUsers(filter);
    EXPECT_TRUE(not_found.empty());
}

UTEST(InMemoryDb, FindUsersByEmail) {
    common::database::InMemoryDb db;

    db.RegisterUser("emailuser", "find@example.com", "name", "surname", "password123");

    common::database::UserFilter filter;
    filter.email = "find@example.com";
    auto found = db.FindUsers(filter);
    ASSERT_EQ(found.size(), 1);
    EXPECT_EQ(found[0].username, "emailuser");

    filter.email = "notfound@example.com";
    auto not_found = db.FindUsers(filter);
    EXPECT_TRUE(not_found.empty());
}

UTEST(InMemoryDb, FindUsersById) {
    common::database::InMemoryDb db;

    auto result = db.RegisterUser("iduser", "id@example.com", "name", "surname", "password123");
    ASSERT_TRUE(result.success);

    common::database::UserFilter filter;
    filter.id = result.user.id;
    auto found = db.FindUsers(filter);
    ASSERT_EQ(found.size(), 1);
    EXPECT_EQ(found[0].username, "iduser");
    EXPECT_EQ(found[0].email, "id@example.com");

    filter.id = "nonexistent-id";
    auto not_found = db.FindUsers(filter);
    EXPECT_TRUE(not_found.empty());
}

UTEST(InMemoryDb, FindUsersByFirstName) {
    common::database::InMemoryDb db;

    db.RegisterUser("user1", "user1@example.com", "John", "Doe", "password123");
    db.RegisterUser("user2", "user2@example.com", "John", "Smith", "password123");
    db.RegisterUser("user3", "user3@example.com", "Jane", "Doe", "password123");

    common::database::UserFilter filter;
    filter.first_name = "John";
    auto found = db.FindUsers(filter);
    ASSERT_EQ(found.size(), 2);

    filter.first_name = "Jane";
    found = db.FindUsers(filter);
    ASSERT_EQ(found.size(), 1);
    EXPECT_EQ(found[0].username, "user3");
}

UTEST(InMemoryDb, FindUsersByLastName) {
    common::database::InMemoryDb db;

    db.RegisterUser("user1", "user1@example.com", "John", "Doe", "password123");
    db.RegisterUser("user2", "user2@example.com", "John", "Smith", "password123");
    db.RegisterUser("user3", "user3@example.com", "Jane", "Doe", "password123");

    common::database::UserFilter filter;
    filter.last_name = "Doe";
    auto found = db.FindUsers(filter);
    ASSERT_EQ(found.size(), 2);

    filter.last_name = "Smith";
    found = db.FindUsers(filter);
    ASSERT_EQ(found.size(), 1);
    EXPECT_EQ(found[0].username, "user2");
}

UTEST(InMemoryDb, FindUsersByMultipleFilters) {
    common::database::InMemoryDb db;

    db.RegisterUser("user1", "user1@example.com", "John", "Doe", "password123");
    db.RegisterUser("user2", "user2@example.com", "John", "Smith", "password123");
    db.RegisterUser("user3", "user3@example.com", "Jane", "Doe", "password123");

    common::database::UserFilter filter;
    filter.first_name = "John";
    filter.last_name = "Doe";
    auto found = db.FindUsers(filter);
    ASSERT_EQ(found.size(), 1);
    EXPECT_EQ(found[0].username, "user1");
}

UTEST(InMemoryDb, FindUsersEmptyFilter) {
    common::database::InMemoryDb db;

    db.RegisterUser("user1", "user1@example.com", "John", "Doe", "password123");
    db.RegisterUser("user2", "user2@example.com", "Jane", "Smith", "password123");

    common::database::UserFilter filter;  // Empty filter - should return all users
    auto found = db.FindUsers(filter);
    EXPECT_EQ(found.size(), 2);
}

UTEST(InMemoryDb, UniqueUserIds) {
    common::database::InMemoryDb db;

    auto r1 = db.RegisterUser("user1", "u1@example.com", "name", "surname", "pass1");
    auto r2 = db.RegisterUser("user2", "u2@example.com", "name", "surname", "pass2");

    EXPECT_TRUE(r1.success);
    EXPECT_TRUE(r2.success);
    EXPECT_NE(r1.user.id, r2.user.id);
}

UTEST(InMemoryDb, PasswordsHashedDeterministically) {
    common::database::InMemoryDb db;

    auto r1 = db.RegisterUser("user1", "u1@example.com", "name", "surname", "samepassword");
    auto r2 = db.RegisterUser("user2", "u2@example.com", "name", "surname", "samepassword");

    EXPECT_TRUE(r1.success);
    EXPECT_TRUE(r2.success);
    // Same password should produce the same hash (SHA-256 is deterministic)
    EXPECT_EQ(r1.user.password_hash, r2.user.password_hash);
    // But neither should be the plain text
    EXPECT_NE(r1.user.password_hash, "samepassword");
}

// Event tests

UTEST(InMemoryDb, SuccessfulEventCreation) {
    common::database::InMemoryDb db;

    common::database::GeoPosition geo{"USA", "New York", "123 Main St"};
    auto result = db.CreateEvent("Test Event", geo, 100, "2024-12-31T18:00:00Z", "organizer123");

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.error.empty());
    EXPECT_EQ(result.event.name, "Test Event");
    EXPECT_EQ(result.event.places_count, 100);
    EXPECT_EQ(result.event.organizer_id, "organizer123");
    EXPECT_EQ(result.event.geo_position.country, "USA");
    EXPECT_EQ(result.event.geo_position.city, "New York");
    EXPECT_FALSE(result.event.id.empty());
    EXPECT_FALSE(result.event.created_at.empty());
}

UTEST(InMemoryDb, EventCreationEmptyName) {
    common::database::InMemoryDb db;

    common::database::GeoPosition geo{"USA", "New York", "123 Main St"};
    auto result = db.CreateEvent("", geo, 100, "2024-12-31T18:00:00Z", "organizer123");

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "Event name and positive places count are required");
}

UTEST(InMemoryDb, EventCreationInvalidPlacesCount) {
    common::database::InMemoryDb db;

    common::database::GeoPosition geo{"USA", "New York", "123 Main St"};
    auto result = db.CreateEvent("Test Event", geo, 0, "2024-12-31T18:00:00Z", "organizer123");

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "Event name and positive places count are required");
}

UTEST(InMemoryDb, EventCreationNegativePlacesCount) {
    common::database::InMemoryDb db;

    common::database::GeoPosition geo{"USA", "New York", "123 Main St"};
    auto result = db.CreateEvent("Test Event", geo, -10, "2024-12-31T18:00:00Z", "organizer123");

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "Event name and positive places count are required");
}

UTEST(InMemoryDb, MultipleEvents) {
    common::database::InMemoryDb db;

    common::database::GeoPosition geo1{"USA", "New York", "123 Main St"};
    common::database::GeoPosition geo2{"UK", "London", "456 Oxford St"};
    common::database::GeoPosition geo3{"France", "Paris", "789 Champs-Élysées"};

    auto r1 = db.CreateEvent("Event 1", geo1, 100, "2024-12-31T18:00:00Z", "org1");
    auto r2 = db.CreateEvent("Event 2", geo2, 200, "2024-12-31T19:00:00Z", "org2");
    auto r3 = db.CreateEvent("Event 3", geo3, 150, "2024-12-31T20:00:00Z", "org3");

    EXPECT_TRUE(r1.success);
    EXPECT_TRUE(r2.success);
    EXPECT_TRUE(r3.success);
    EXPECT_EQ(db.EventCount(), 3);
}

UTEST(InMemoryDb, FindEventsById) {
    common::database::InMemoryDb db;

    common::database::GeoPosition geo{"USA", "New York", "123 Main St"};
    auto result = db.CreateEvent("Find Me Event", geo, 100, "2024-12-31T18:00:00Z", "organizer123");
    ASSERT_TRUE(result.success);

    common::database::EventFilter filter;
    filter.id = result.event.id;
    auto found = db.FindEvents(filter);
    ASSERT_EQ(found.size(), 1);
    EXPECT_EQ(found[0].name, "Find Me Event");
    EXPECT_EQ(found[0].organizer_id, "organizer123");

    filter.id = "nonexistent-id";
    auto not_found = db.FindEvents(filter);
    EXPECT_TRUE(not_found.empty());
}

UTEST(InMemoryDb, FindEventsByName) {
    common::database::InMemoryDb db;

    common::database::GeoPosition geo{"USA", "New York", "123 Main St"};
    db.CreateEvent("Music Festival", geo, 100, "2024-12-31T18:00:00Z", "org1");
    db.CreateEvent("Tech Conference", geo, 200, "2024-12-31T19:00:00Z", "org2");
    db.CreateEvent("Music Festival", geo, 150, "2024-12-31T20:00:00Z", "org3");

    common::database::EventFilter filter;
    filter.name = "Music Festival";
    auto found = db.FindEvents(filter);
    ASSERT_EQ(found.size(), 2);

    filter.name = "Tech Conference";
    found = db.FindEvents(filter);
    ASSERT_EQ(found.size(), 1);
}

UTEST(InMemoryDb, FindEventsByOrganizerId) {
    common::database::InMemoryDb db;

    common::database::GeoPosition geo{"USA", "New York", "123 Main St"};
    db.CreateEvent("Event 1", geo, 100, "2024-12-31T18:00:00Z", "organizer1");
    db.CreateEvent("Event 2", geo, 200, "2024-12-31T19:00:00Z", "organizer1");
    db.CreateEvent("Event 3", geo, 150, "2024-12-31T20:00:00Z", "organizer2");

    common::database::EventFilter filter;
    filter.organizer_id = "organizer1";
    auto found = db.FindEvents(filter);
    ASSERT_EQ(found.size(), 2);

    filter.organizer_id = "organizer2";
    found = db.FindEvents(filter);
    ASSERT_EQ(found.size(), 1);
}

UTEST(InMemoryDb, FindEventsByCountry) {
    common::database::InMemoryDb db;

    common::database::GeoPosition geo1{"USA", "New York", "123 Main St"};
    common::database::GeoPosition geo2{"USA", "Los Angeles", "456 Hollywood Blvd"};
    common::database::GeoPosition geo3{"UK", "London", "789 Oxford St"};

    db.CreateEvent("Event 1", geo1, 100, "2024-12-31T18:00:00Z", "org1");
    db.CreateEvent("Event 2", geo2, 200, "2024-12-31T19:00:00Z", "org2");
    db.CreateEvent("Event 3", geo3, 150, "2024-12-31T20:00:00Z", "org3");

    common::database::EventFilter filter;
    filter.country = "USA";
    auto found = db.FindEvents(filter);
    ASSERT_EQ(found.size(), 2);

    filter.country = "UK";
    found = db.FindEvents(filter);
    ASSERT_EQ(found.size(), 1);
}

UTEST(InMemoryDb, FindEventsByCity) {
    common::database::InMemoryDb db;

    common::database::GeoPosition geo1{"USA", "New York", "123 Main St"};
    common::database::GeoPosition geo2{"USA", "New York", "456 Broadway"};
    common::database::GeoPosition geo3{"USA", "Los Angeles", "789 Hollywood Blvd"};

    db.CreateEvent("Event 1", geo1, 100, "2024-12-31T18:00:00Z", "org1");
    db.CreateEvent("Event 2", geo2, 200, "2024-12-31T19:00:00Z", "org2");
    db.CreateEvent("Event 3", geo3, 150, "2024-12-31T20:00:00Z", "org3");

    common::database::EventFilter filter;
    filter.city = "New York";
    auto found = db.FindEvents(filter);
    ASSERT_EQ(found.size(), 2);

    filter.city = "Los Angeles";
    found = db.FindEvents(filter);
    ASSERT_EQ(found.size(), 1);
}

UTEST(InMemoryDb, FindEventsByMultipleFilters) {
    common::database::InMemoryDb db;

    common::database::GeoPosition geo1{"USA", "New York", "123 Main St"};
    common::database::GeoPosition geo2{"USA", "New York", "456 Broadway"};
    common::database::GeoPosition geo3{"USA", "Los Angeles", "789 Hollywood Blvd"};

    db.CreateEvent("Music Festival", geo1, 100, "2024-12-31T18:00:00Z", "org1");
    db.CreateEvent("Tech Conference", geo2, 200, "2024-12-31T19:00:00Z", "org1");
    db.CreateEvent("Music Festival", geo3, 150, "2024-12-31T20:00:00Z", "org2");

    common::database::EventFilter filter;
    filter.name = "Music Festival";
    filter.country = "USA";
    filter.city = "New York";
    auto found = db.FindEvents(filter);
    ASSERT_EQ(found.size(), 1);
    EXPECT_EQ(found[0].organizer_id, "org1");
}

UTEST(InMemoryDb, FindEventsEmptyFilter) {
    common::database::InMemoryDb db;

    common::database::GeoPosition geo1{"USA", "New York", "123 Main St"};
    common::database::GeoPosition geo2{"UK", "London", "456 Oxford St"};

    db.CreateEvent("Event 1", geo1, 100, "2024-12-31T18:00:00Z", "org1");
    db.CreateEvent("Event 2", geo2, 200, "2024-12-31T19:00:00Z", "org2");

    common::database::EventFilter filter;  // Empty filter - should return all events
    auto found = db.FindEvents(filter);
    EXPECT_EQ(found.size(), 2);
}

UTEST(InMemoryDb, UniqueEventIds) {
    common::database::InMemoryDb db;

    common::database::GeoPosition geo1{"USA", "New York", "123 Main St"};
    common::database::GeoPosition geo2{"UK", "London", "456 Oxford St"};

    auto r1 = db.CreateEvent("Event 1", geo1, 100, "2024-12-31T18:00:00Z", "org1");
    auto r2 = db.CreateEvent("Event 2", geo2, 200, "2024-12-31T19:00:00Z", "org2");

    EXPECT_TRUE(r1.success);
    EXPECT_TRUE(r2.success);
    EXPECT_NE(r1.event.id, r2.event.id);
}
