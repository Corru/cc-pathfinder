#include <map>
#include <string>

#include <boost/python.hpp>
#include <boost/python/args.hpp>
#include <boost/python/numpy.hpp>

namespace python = boost::python;
namespace numpy = boost::python::numpy;

namespace
{
    template<std::size_t N>
    auto check_dimensions(numpy::ndarray const& array)
    {
        auto const array_nd = static_cast<std::size_t>(array.get_nd());
        if (array_nd != N)
        {
            throw std::invalid_argument
            {
                "expected " + std::to_string(N) + "d array but " + std::to_string(array_nd) + "d is given"
            };
        }
    }

    struct position2
    {
        int x;
        int y;

        static auto from(python::object const& pair) -> position2
        {
            return
            {
                .x = python::extract<decltype(position2::x)>(pair[0]),
                .y = python::extract<decltype(position2::y)>(pair[1]),
            };
        }

        friend auto operator<(position2 const& left, position2 const& right) noexcept -> bool
        {
            if (left.x == right.x)
            {
                return left.y < right.y;
            }
            return left.x < right.x;
        }
    };
} // anonymous namespace

auto navigate_using_heightmap(numpy::ndarray const& hmap, python::object const& start_pos_obj, python::object const& finish_pos_obj)
    -> python::list
{
    //
    // Since map is heightmap it should be 2d array
    //
    check_dimensions<2>(hmap);

    //
    // Unpack start and finish positions
    //
    auto const start  = position2::from(start_pos_obj);
    auto const finish = position2::from(finish_pos_obj);

    throw std::runtime_error{ "not implemented" };
}

/// DLL entry
BOOST_PYTHON_MODULE(pathfinder)
{
    numpy::initialize();
    python::def("navigate_using_heightmap", navigate_using_heightmap, (python::arg("map"), "start", "finish"),
                "returns optimal path from start to finish based on heightmap");
}