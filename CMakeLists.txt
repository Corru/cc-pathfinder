cmake_minimum_required(VERSION 2.8.3)
project(cc-pathfinder)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

include(libanvil)

find_package(Boost REQUIRED COMPONENTS locale)

include_directories(${Boost_INCLUDE_DIRS} ${libanvil_INCLUDE_DIRS})
message(STATUS ${libanvil_INCLUDE_DIRS})

add_subdirectory(src)