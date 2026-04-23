project(event_service CXX)

add_library(${PROJECT_NAME}_objs OBJECT
    src/api/create/handler.cpp
    src/api/list/handler.cpp
    src/api/book/handler.cpp
    src/api/cancel/handler.cpp
)
target_include_directories(${PROJECT_NAME}_objs PUBLIC src)
target_link_libraries(${PROJECT_NAME}_objs PUBLIC userver::core event_common)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_objs)
