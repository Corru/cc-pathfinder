//

#include <vector>
#include <string>
#include <unordered_map>
#include <sstream>
#include <filesystem>
#include <region_file_reader.h>
#include <optional>

namespace fs = std::filesystem;

enum Side {
    NORTH, EAST, WEST, SOUTH
};
constexpr unsigned int DOWN = 4;
constexpr unsigned int UP = 5;

struct RealBlockCoord {
    int x, z;
    unsigned int y;

    RealBlockCoord dec_x() const {
        return {x - 1, z, y};
    }

    RealBlockCoord dec_z() const {
        return {x, z - 1, y};
    }

    RealBlockCoord dec_y() const {
        return {x, z, y - 1};
    }

    RealBlockCoord inc_x() const {
        return {x + 1, z, y};
    }

    RealBlockCoord inc_z() const {
        return {x, z + 1, y};
    }

    RealBlockCoord inc_y() const {
        return {x, z, y + 1};
    }
};

struct RelativeBlockCoord {
    unsigned int x, z, y;

    RelativeBlockCoord dec_x() const {
        return {x - 1u, z, y};
    }

    RelativeBlockCoord dec_z() const {
        return {x, z - 1u, y};
    }

    RelativeBlockCoord dec_y() const {
        return {x, z, y - 1u};
    }

    RelativeBlockCoord inc_x() const {
        return {x + 1u, z, y};
    }

    RelativeBlockCoord inc_z() const {
        return {x, z + 1u, y};
    }

    RelativeBlockCoord inc_y() const {
        return {x, z, y + 1u};
    }
};

struct PathNode {
    unsigned int n;

    PathNode(unsigned int n) : n(n) {}

    PathNode(const PathNode& from, int off) 
        : n(static_cast<unsigned int>(static_cast<int>(from.n) + off)) 
        {}

    bool operator==(const PathNode &p) const
    {
        return n == p.n;
    }
};
struct PathNodeHash
{
    std::size_t operator() (const PathNode &node) const
    {
        return std::hash<unsigned int>()(node.n);
    }
};

class RegionView 
{
    static inline unsigned int to_region_coord(unsigned int x) 
    {
        return x >> 9;
    }

    static inline unsigned int to_chunk_coord(unsigned int x) 
    {
        return x >> 4;
    }

    static inline std::string region_path(std::string base_path, int x, int z) 
    {
        std::ostringstream region_file;
        region_file << "r." << x << '.' << z << ".mca";
        return (fs::path(base_path) / fs::path(region_file.str())).string();
    }

    int real_x, real_z;
    unsigned int real_width, real_height;

    int off_x, off_z;
    unsigned int width_, height_;

    std::vector<region_file_reader> regions;

public:

    static constexpr unsigned int CHUNK_HEIGHT = 256u;
    static constexpr unsigned int CHUNK_WIDTH = 1u << 4;

    static constexpr unsigned int REGION_HEIGHT = 256u;
    static constexpr unsigned int REGION_WIDTH = 1u << 9;

    static constexpr unsigned int IN_REGION_MASK = REGION_WIDTH - 1u;
    static constexpr unsigned int REGION_MASK = ~IN_REGION_MASK;
    static constexpr unsigned int IN_CHUNK_MASK = CHUNK_WIDTH - 1u;
    static constexpr unsigned int CHUNK_MASK = ~IN_CHUNK_MASK;

    RegionView(const RegionView&) = default;
    RegionView(RegionView&&) = default;

    RegionView(
            std::string path,
            int x, int z, 
            unsigned int width, unsigned int height
        ) : real_x(x), 
            real_z(z),
            real_width(width), 
            real_height(height)
    {
        if (width < 1 || height < 1) 
        {
            throw std::invalid_argument("width and height should be greater 0"); // TODO:
        }

        int upper_x = real_x + static_cast<int>(real_width);
        int upper_z = real_z + static_cast<int>(real_height);

        this->off_x = real_x >> 9;
        this->off_z = real_z >> 9;

        this->width_ = static_cast<unsigned int>((upper_x >> 9) - off_x);
        this->height_ = static_cast<unsigned int>((upper_z >> 9) - off_z);

        if ((IN_REGION_MASK & upper_x) != 0) 
        {
            this->width_ += 1;
        }

        if ((IN_REGION_MASK & upper_z) != 0) 
        {
            this->height_ += 1;
        }

        regions.reserve(width_ * height_);

        for (unsigned int z = 0u; z < height_; z++) 
        {
            for (unsigned int x = 0u; x < width_; x++) 
            {
                auto& region = regions.emplace_back(region_path(path, x, z));
                region.read();
            }
        }
    }

    const unsigned int& width() {
        return this->width_;
    }

    const unsigned int& height() {
        return this->height_;
    }

    inline RelativeBlockCoord to_relative(const RealBlockCoord& coord) {
        return {
            .x = static_cast<unsigned int>(coord.x - off_x * static_cast<int>(REGION_WIDTH)), 
            .z = static_cast<unsigned int>(coord.z - off_z * static_cast<int>(REGION_WIDTH)),
            .y = coord.y
        };
    }

    inline RealBlockCoord to_real(const RelativeBlockCoord& coord) {
        return {
            .x = static_cast<int>(coord.x + off_x * static_cast<int>(REGION_WIDTH)), 
            .z = static_cast<int>(coord.z + off_z * static_cast<int>(REGION_WIDTH)),
            .y = coord.y
        };
    }

    bool is_air_block(const RelativeBlockCoord& coord)
    {
        // Resolve region
        auto const r_x = to_region_coord(coord.x);
        auto const r_z = to_region_coord(coord.z);
        auto& region = regions[r_z * width_ + (r_x)];
        // Resolve local block coords
        auto const x = coord.x & IN_REGION_MASK;
        auto const z = coord.z & IN_REGION_MASK;

        auto const c_x = to_chunk_coord(x);
        auto const c_z = to_chunk_coord(z);

        auto const b_x = coord.x & IN_CHUNK_MASK;
        auto const b_y = coord.y;
        auto const b_z = coord.z & IN_CHUNK_MASK;

        return region.get_block_at(c_x, c_z, b_x, b_y, b_z) == 0;
    }

    bool is_air_block(const RealBlockCoord& coord)
    {
        return is_air_block(to_relative(coord));
    }
};

class TurtlePathGraph 
{
    RegionView view;
    std::unordered_map<PathNode, std::vector<PathNode>, PathNodeHash> adjacency_matrix;

    int adj_node_off[6];

    void push_adj_node_north(std::vector<PathNode>& row, const PathNode& node)
    {
        auto const coord = to_relative_coord(node);
        if (coord.z > 1u && view.is_air_block(coord.dec_z())) {
            row.emplace_back(node, adj_node_off[Side::NORTH]);
        }
    }

    void push_adj_node_south(std::vector<PathNode>& row, const PathNode& node)
    {
        auto const coord = to_relative_coord(node);
        if (coord.z + 1u < view.height() && view.is_air_block(coord.inc_z())) {
            row.emplace_back(node, adj_node_off[Side::SOUTH]);
        }
    }

    void push_adj_node_west(std::vector<PathNode>& row, const PathNode& node)
    {
        auto const coord = to_relative_coord(node);
        if (coord.x > 1u && view.is_air_block(coord.dec_x())) {
            row.emplace_back(node, adj_node_off[Side::WEST]);
        }
    }

    void push_adj_node_east(std::vector<PathNode>& row, const PathNode& node)
    {
        auto const coord = to_relative_coord(node);
        if (coord.x + 1u < view.width() && view.is_air_block(coord.inc_x())) {
            row.emplace_back(node, adj_node_off[Side::EAST]);
        }
    }

    void push_adj_node_down(std::vector<PathNode>& row, const PathNode& node)
    {
        auto const coord = to_relative_coord(node);
        if (coord.y > 1u && view.is_air_block(coord.dec_y())) {
            row.emplace_back(node, adj_node_off[DOWN]);
        }
    }

    void push_adj_node_up(std::vector<PathNode>& row, const PathNode& node)
    {
        auto const coord = to_relative_coord(node);
        if (coord.y + 1u < RegionView::REGION_HEIGHT && view.is_air_block(coord.inc_y())) {
            row.emplace_back(node, adj_node_off[UP]);
        }
    }

    std::vector<PathNode> find_adjacent_nodes(const PathNode& node) 
    {
        std::vector<PathNode> res;
        auto const side = static_cast<Side>(node.n % 4);
        switch (side)
        {
        case Side::NORTH: push_adj_node_north(res, node); break;
        case Side::SOUTH: push_adj_node_south(res, node); break;
        case Side::WEST: push_adj_node_west(res, node); break;
        case Side::EAST: push_adj_node_east(res, node); break;
        }
        push_adj_node_up(res, node);
        push_adj_node_down(res, node);
        return res;
    }

public:

    TurtlePathGraph(RegionView&& view)
        : view(std::move(view))
    {
        adj_node_off[UP] = 4;
        adj_node_off[DOWN] = -adj_node_off[UP];

        adj_node_off[Side::EAST] = static_cast<int>(RegionView::CHUNK_HEIGHT) * adj_node_off[UP];
        adj_node_off[Side::WEST] = -adj_node_off[Side::EAST];

        adj_node_off[Side::SOUTH] = static_cast<int>(view.width()) * adj_node_off[Side::EAST];
        adj_node_off[Side::NORTH] = -adj_node_off[Side::SOUTH];
    }

    PathNode to_node(const RelativeBlockCoord& coord, Side side)
    {
        auto const xz_i = coord.z * view.width() + coord.x;
        auto const xzy_i = xz_i * RegionView::CHUNK_HEIGHT + coord.y;
        return {xzy_i * 4u + static_cast<unsigned int>(side)};
    }

    PathNode to_node(const RealBlockCoord& coord, Side side)
    {
        return to_node(view.to_relative(coord), side);
    }

    RelativeBlockCoord to_relative_coord(const PathNode& node) {
        auto const xzy_i = node.n / 4u;
        auto const xz_i = xzy_i / RegionView::CHUNK_HEIGHT;
        return {
            .x = xz_i % view.width(), 
            .z = xz_i / view.width(), 
            .y = xzy_i % RegionView::CHUNK_HEIGHT
        };
    }

    RealBlockCoord to_real_coord(const PathNode& node) {
        return view.to_real(to_relative_coord(node));
    }

    const std::vector<PathNode>& adjacent_nodes(const PathNode& node) 
    {
        auto it = adjacency_matrix.find(node);
        if (it != adjacency_matrix.end())
        {
            return it->second;
        }
        const auto [it2, success] = adjacency_matrix.insert({node, find_adjacent_nodes(node)});
        return it2->second;
    }
};
