project(analytics_service CXX)

add_library(${PROJECT_NAME}_objs OBJECT
    src/api/log_activity/handler.cpp
    src/api/record_search/handler.cpp
    src/api/event_stats/handler.cpp
)
target_include_directories(${PROJECT_NAME}_objs PUBLIC src)
target_link_libraries(${PROJECT_NAME}_objs PUBLIC userver::core userver::mongo event_common)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_objs)
