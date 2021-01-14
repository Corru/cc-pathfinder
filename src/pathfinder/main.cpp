#include <iostream>
#include <set>
#include <array>
#include <string>
#include <sstream>

#include <boost/json.hpp>

#include "graph.hpp"
#include "network.hpp"

namespace network {
    template<typename ...Tail, template<typename ...> typename Container>
    auto http_server(tcp::acceptor& acceptor, tcp::socket& socket, Container<network::endpoint, Tail...> const& endpoints) -> void {
        acceptor.async_accept(
            socket, 
            [&](beast::error_code ec) {
                if (!ec) {
                    std::make_shared<http_connection>(std::move(socket))->start(endpoints);
                }
                http_server(acceptor, socket, endpoints);
            });
    }
}

template<typename T, std::size_t N>
static auto operator<<(std::ostream& stream, std::array<T, N> const& array) -> std::ostream& {
    std::size_t const size = std::size(array);

    stream << "{";
    if (size > 0) {
        for (std::size_t i = 0; i < size - 1; ++i) {
            stream << array[i] << ", ";
        }
        stream << array[size - 1];
    }
    stream << "}";

    return stream;
}

template<std::size_t N>
static auto match_name(std::string_view const name, std::array<std::string_view, N> const& list) -> void {
    using namespace std::string_literals;

    for (auto const& entry : list) {
        if (name == entry) {
            return;
        }
    }

    std::ostringstream list_ostream;
    list_ostream << list;

    throw std::invalid_argument{
        "parameter name '"s + std::string(name) + "' does not match any of "s + list_ostream.str()
    };
}

auto main(int const argc, char const* argv[]) -> int {
    using namespace network;
    using namespace std::string_view_literals;

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

    try {
        std::set<network::endpoint> endpoints{};
        endpoints.insert({
            .path = "/navigate",
            .method = network::http::verb::get,
            .callback = [](network::request const& request, network::response& response) {
                std::array constexpr       required_params = {"x_start"sv, "z_start"sv, "x_finish"sv, "z_finish"sv};
                std::map<std::string, int> parsed_params;

                // Parse query
                auto const  url = skyr::url(std::string{ request.target() });
                auto const& search = url.search_parameters();
                for (auto const& [key, value] : search) {
                    // Check name of key; if name is unknown exception will be thrown
                    match_name(key, required_params);

                    // Check that there is no multiple defenitions of current parameter
                    if (parsed_params.find(key) != std::end(parsed_params)) {
                        throw std::invalid_argument{ "multiple definitions of " + key };
                    }

                    parsed_params[key] = std::stoi(value);
                }
                if (std::size(parsed_params) != std::size(required_params)) {
                    throw std::invalid_argument{"incorrect number of arguments"};
                }

                // Exctract parameters from parsed query
                int const x_start  = parsed_params["x_start"];
                int const z_start  = parsed_params["z_start"];
                int const x_finish = parsed_params["x_finish"];
                int const z_finish = parsed_params["z_finish"];

                throw std::runtime_error{"not implemented"};
            }
        });

        auto const address = net::ip::make_address(argv[1]);
        unsigned short port = static_cast<unsigned short>(std::atoi(argv[2]));

        net::io_context ioc{ 1 };

        tcp::acceptor acceptor{ ioc, {address, port} };
        tcp::socket socket{ ioc };
        http_server(acceptor, socket, endpoints);

        ioc.run();
    }
    catch (std::exception const& e) {
        std::cerr << "FATAL: " << e.what() << std::endl;
    }
}
