file(GLOB_RECURSE pathfinder_SRC
     "*.h"
     "*.cpp"
     "*.h"
     "*.c"
)

PYTHON_ADD_MODULE(pathfinder ${pathfinder_SRC})
target_link_libraries(pathfinder ${Boost_LIBRARIES} ${PYTHON_LIBRARIES})