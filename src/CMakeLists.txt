file(GLOB_RECURSE pathfinder_SRC
     "*.h"
     "*.cpp"
     "*.h"
     "*.c"
)

add_library(pathfinder STATIC ${pathfinder_SRC})
target_link_libraries(pathfinder ${Boost_LIBRARIES})