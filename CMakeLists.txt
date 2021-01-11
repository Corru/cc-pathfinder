# ------------------------------------------------------- #


cmake_minimum_required(VERSION 3.8)
project(cc-pathfinder VERSION 0.0.1 LANGUAGES CXX)


# ------------------------------------------------------- #


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

set(Boost_USE_STATIC_LIBS ON)


# ------------------------------------------------------- #


include(libanvil)
find_package(Boost REQUIRED COMPONENTS locale)


# ------------------------------------------------------- #


include_directories(${Boost_INCLUDE_DIRS} ${libanvil_INCLUDE_DIRS})
add_subdirectory(src/server)
add_subdirectory(src/pathfinder)