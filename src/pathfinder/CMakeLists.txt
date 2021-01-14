add_executable(pathfinder)

file(GLOB_RECURSE pathfinder_SRC_FILES
    "*.cpp" "*.c" "*.hpp" "*.h")

find_package(Boost REQUIRED COMPONENTS system regex thread)
find_package(tl-expected CONFIG REQUIRED)
find_package(range-v3 CONFIG REQUIRED)
find_package(skyr-url CONFIG REQUIRED)

target_include_directories(pathfinder PUBLIC ${libanvil_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
target_sources(pathfinder PRIVATE ${pathfinder_SRC_FILES})
target_link_libraries(pathfinder PRIVATE libanvil::libanvil ${Boost_LIBRARIES} skyr::skyr-url)