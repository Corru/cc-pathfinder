#include <iostream>
#include <set>
#include <array>
#include <string>
#include <string_view>
#include <sstream>

#include <boost/format.hpp>
#include <skyr/url.hpp>

#include "network/endpoint.hpp"
#include "network/service.hpp"

namespace network {
    auto register_service(tcp::acceptor& acceptor, tcp::socket& socket, network::endpoints_collection const& endpoints) -> void {
        acceptor.async_accept(
            socket, 
            [&](beast::error_code ec) 
            {
                if (!ec)
                {
                    std::make_shared<service>(std::move(socket))->start(endpoints);
                }
                register_service(acceptor, socket, endpoints);
            }
        );
    }
}

template<typename T, std::size_t N>
auto extract_required_from_query(
    skyr::url_search_parameters const&     query,
    std::array<std::string_view, N> const& required_params,
    std::map<std::string, T>&              result
) noexcept(false) -> void
{
    for (auto const param_name : required_params)
    {
        auto const it = std::find_if(
            std::begin(query),
            std::end(query),
            [&](auto const& pair) -> bool 
            {
                return pair.first == param_name;
            }
        );
        if (it != std::end(query))
        {
            auto const& [key, value] = *it;
            auto parsed_value = T{};

            // Try extract value from string value
            std::stringstream string_value(value);
            string_value >> parsed_value;
            if (string_value.fail())
            {
                auto const error = boost::format("query parameter '%s' is not %s value") % param_name % typeid(parsed_value).name();
                throw std::invalid_argument{ error.str() };
            }

            result[key] = parsed_value;
        }
        else
        {
            auto const error = boost::format("query parameter '%s' is required") % param_name;
            throw std::invalid_argument{ error.str() };
        }
    }
}

auto main(int const argc, char const* argv[]) -> int {
    using namespace network;
    using namespace std::string_view_literals;

    /*
    TurtlePathGraph graph(RegionView(
        "/c/Users/wmath/Twitch/Minecraft/Instances/FTB Revelation/saves/Test/region",
        -5, -5,
        10, 10)
    );
    auto& view = graph.view();
    RealBlockCoord b1{ .x = 2,.z = 2,.y = 65 };
    RealBlockCoord b2{ .x = 2,.z = 2,.y = 64 };
    RealBlockCoord b3{ .x = 3,.z = 3,.y = 64 };
    RealBlockCoord b4{ .x = 3,.z = 9,.y = 63 };
    RealBlockCoord b5{ .x = 3,.z = 10,.y = 63 };

    std::cout << view.is_air_block(b1) << std::endl;
    std::cout << view.is_air_block(b2) << std::endl;
    std::cout << view.is_air_block(b3) << std::endl;
    std::cout << view.is_air_block(b4) << std::endl;
    std::cout << view.is_air_block(b5) << std::endl;
    */

    // Check command line arguments.
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <address> <port>\n";
        std::cerr << "  For IPv4, try:\n";
        std::cerr << "    receiver 0.0.0.0 80\n";
        std::cerr << "  For IPv6, try:\n";
        std::cerr << "    receiver 0::0 80\n";
        return EXIT_FAILURE;
    }

    endpoints_collection endpoints;
    endpoints.update({
        .path     = "/navigate",
        .method   = network::http::verb::get,
        .callback = [](network::request const& request, network::response& response) -> void
        {
            auto constexpr required_params = std::array
            {
                "x_start"sv, "z_start"sv, "y_start"sv,
                "x_finish"sv, "z_finish"sv, "y_finish"sv,
            };
            auto parsed_params = std::map<std::string, int>{};

            //
            // Extract required params.
            // If there is no some of them extract_required_from_query() will throw an error.
            //
            auto const url = skyr::url{ std::string{request.target()} };
            extract_required_from_query(url.search_parameters(), required_params, parsed_params);

            //
            // Nah, we reached unreachable, bruh on you
            //
            throw std::runtime_error{ "not implemented" };
        }
    });

    try {
        auto const address = net::ip::make_address(argv[1]);
        unsigned short port = static_cast<unsigned short>(std::atoi(argv[2]));

        net::io_context ioc{ 1 };

        tcp::acceptor acceptor{ ioc, {address, port} };
        tcp::socket socket{ ioc };
        register_service(acceptor, socket, endpoints);

        ioc.run();
    }
    catch (std::exception const& e) {
        std::cerr << "[FATAL] " << e.what() << std::endl;
    }
}
