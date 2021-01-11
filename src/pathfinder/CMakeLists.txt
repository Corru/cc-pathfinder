add_executable(pathfinder)

file(GLOB_RECURSE libanvil_SRC_FILES
    "*.cpp" "*.c" "*.hpp" "*.h")

target_include_directories(pathfinder PUBLIC ${libanvil_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
target_sources(pathfinder PUBLIC ${libanvil_SRC_FILES})
target_link_libraries(pathfinder Boost::locale libanvil)