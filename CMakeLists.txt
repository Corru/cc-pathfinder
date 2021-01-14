# ------------------------------------------------------- #


cmake_minimum_required(VERSION 3.8)
project(cc-pathfinder VERSION 0.0.1 LANGUAGES CXX)


# ------------------------------------------------------- #


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

set(Boost_USE_STATIC_LIBS ON)


# ------------------------------------------------------- #


include(libanvil)


# ------------------------------------------------------- #


add_subdirectory(src/pathfinder)