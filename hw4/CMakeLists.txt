cmake_minimum_required(VERSION 3.20)
project(monorepo CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(userver COMPONENTS core postgresql mongo REQUIRED)
find_package(jwt-cpp REQUIRED)

add_subdirectory(common)

add_subdirectory(services/user)
add_subdirectory(services/event)
add_subdirectory(services/book)
add_subdirectory(services/analytics)

