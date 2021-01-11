#include <iostream>
#include "graph.hpp"

auto main() -> int try {
    TurtlePathGraph graph(RegionView(
        "C:\\Users\\wmath\\Twitch\\Minecraft\\Instances\\FTB Revelation\\saves\\Test\\region",
        -5, -5, 
        10, 10)
    );
    auto& view = graph.view();
    RealBlockCoord b1{.x=2,.z=2,.y=65};
    RealBlockCoord b2{.x=2,.z=2,.y=64};
    RealBlockCoord b3{.x=3,.z=3,.y=64};

    std::cout << view.is_air_block(b1) << std::endl;
    std::cout << view.is_air_block(b2) << std::endl;
    std::cout << view.is_air_block(b3) << std::endl;
}
catch (std::exception const& e) {
    std::cerr << "FATAL: " << e.what() << std::endl;
}