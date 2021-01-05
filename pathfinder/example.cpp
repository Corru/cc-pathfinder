#include <boost/python.hpp>

#include "example.hpp"

using namespace boost::python;

BOOST_PYTHON_MODULE(pathfinder)
{
    class_<World>("World")
        .def("greet", &World::greet)
        .def("set", &World::set)
    ;
}