find_package(jwt-cpp REQUIRED)

add_library(event_common STATIC
    src/database/db.cpp
)

target_include_directories(event_common PUBLIC include)

target_link_libraries(event_common PUBLIC userver::core userver::postgresql jwt-cpp::jwt-cpp Boost::json)

# Unit Tests
add_executable(event_common_unittest
    src/jwt/jwt_test.cpp
)
target_link_libraries(event_common_unittest PRIVATE event_common userver::utest)
add_google_tests(event_common_unittest)
