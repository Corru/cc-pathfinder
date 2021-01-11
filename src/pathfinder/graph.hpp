//

#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <filesystem>
#include <region_file_reader.h>
#include <optional>

namespace fs = std::filesystem;

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

	unsigned int real_x, real_z;
	unsigned int real_width, real_height;

	unsigned int off_x, off_z;
	unsigned int width_, height_;

	std::vector<region_file_reader> regions;

	region_file_reader& get_region(int x, int z)
	{
		int i = (z - off_z) * width_ + (x - off_x);
		return regions[i];
	}

public:

	static constexpr unsigned int CHUNK_HEIGHT = 256;
	static constexpr unsigned int CHUNK_WIDTH = 1u << 4;

	static constexpr unsigned int REGION_HEIGHT = 256;
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

		unsigned int upper_x = real_x + real_width;
		unsigned int upper_z = real_z + real_height;

		this->off_x = to_region_coord(real_x);
		this->off_z = to_region_coord(real_z);

		this->width_ = to_region_coord(upper_x) - off_x;
		this->height_ = to_region_coord(upper_z) - off_z;

		if ((IN_REGION_MASK & upper_x) != 0) 
		{
			this->width_ += 1;
		}

		if ((IN_REGION_MASK & upper_z) != 0) 
		{
			this->height_ += 1;
		}

		regions.reserve(width_ * height_);

		for (unsigned int z = 0; z < height_; z++) 
		{
			for (unsigned int x = 0; x < width_; x++) 
			{
				auto& region = regions.emplace_back(region_path(path, x, z));
				region.read();
			}
		}
	}

	bool is_air_block(int x, int z, int y)
	{
		auto& region = get_region(to_region_coord(x), to_region_coord(z));
		x &= IN_REGION_MASK;
		z &= IN_REGION_MASK;
		int block = region.get_block_at(to_chunk_coord(x), to_chunk_coord(z),
						x & IN_CHUNK_MASK, y, z & IN_CHUNK_MASK);
		return block == 0;
	}

	unsigned int width() {
		return this->width_;
	}

	unsigned int height() {
		return this->height_;
	}

	inline unsigned int to_absolute_x(unsigned int x) {
		return x - off_x * REGION_WIDTH;
	}

	inline unsigned int to_absolute_z(unsigned int z) {
		return z - off_z * REGION_WIDTH;
	}

	inline unsigned int to_real_x(unsigned int x) {
		return x + off_x * REGION_WIDTH;
	}

	inline unsigned int to_real_z(unsigned int z) {
		return z + off_z * REGION_WIDTH;
	}

};

enum Side {
	NORTH, EAST, WEST, SOUTH, DOWN, UP
};

class TurtlePathGraph 
{
	RegionView view;
	std::map<unsigned int, std::vector<unsigned int>> adjacency_matrix;

	int adj_node_off[6];

	std::optional<unsigned int> adj_node_north(unsigned int node)
	{
		unsigned int xzy_i = node / 6;
		unsigned int xz_i = xzy_i / RegionView::CHUNK_HEIGHT;
		unsigned int y = xzy_i % RegionView::CHUNK_HEIGHT;
		unsigned int x = xz_i % view.width();
		unsigned int z = xz_i / view.width();
		x = view.to_real_x(x);
		z = view.to_real_z(z);
		return (z > 1 && view.is_air_block(x, y, z - 1)) ? 
			std::optional<unsigned int>{node + adj_node_off[Side::NORTH]} :
			std::nullopt;
	}

	std::optional<unsigned int> adj_node_south(unsigned int node)
	{
		unsigned int xzy_i = node / 6;
		unsigned int xz_i = xzy_i / RegionView::CHUNK_HEIGHT;
		unsigned int y = xzy_i % RegionView::CHUNK_HEIGHT;
		unsigned int x = xz_i % view.width();
		unsigned int z = xz_i / view.width();
		x = view.to_real_x(x);
		z = view.to_real_z(z);
		return (z + 1 < view.height() && view.is_air_block(x, y, z + 1)) ? 
			std::optional<unsigned int>{node + adj_node_off[Side::SOUTH]} :
			std::nullopt;
	}

	std::optional<unsigned int> adj_node_west(unsigned int node)
	{
		unsigned int xzy_i = node / 6;
		unsigned int xz_i = xzy_i / RegionView::CHUNK_HEIGHT;
		unsigned int y = xzy_i % RegionView::CHUNK_HEIGHT;
		unsigned int x = xz_i % view.width();
		unsigned int z = xz_i / view.width();
		x = view.to_real_x(x);
		z = view.to_real_z(z);
		return (x > 1 && view.is_air_block(x - 1, y, z)) ? 
			std::optional<unsigned int>{node + adj_node_off[Side::WEST]} :
			std::nullopt;
	}

	std::optional<unsigned int> adj_node_east(unsigned int node)
	{
		unsigned int xzy_i = node / 6;
		unsigned int xz_i = xzy_i / RegionView::CHUNK_HEIGHT;
		unsigned int y = xzy_i % RegionView::CHUNK_HEIGHT;
		unsigned int x = xz_i % view.width();
		unsigned int z = xz_i / view.width();
		x = view.to_real_x(x);
		z = view.to_real_z(z);
		return (x + 1 < view.width() && view.is_air_block(x + 1, y, z)) ? 
			std::optional<unsigned int>{node + adj_node_off[Side::EAST]} :
			std::nullopt;
	}

	std::optional<unsigned int> adj_node_down(unsigned int node)
	{
		unsigned int xzy_i = node / 6;
		unsigned int xz_i = xzy_i / RegionView::CHUNK_HEIGHT;
		unsigned int y = xzy_i % RegionView::CHUNK_HEIGHT;
		unsigned int x = xz_i % view.width();
		unsigned int z = xz_i / view.width();
		x = view.to_real_x(x);
		z = view.to_real_z(z);
		return (y > 1 && view.is_air_block(x, y - 1, z)) ? 
			std::optional<unsigned int>{node - adj_node_off[Side::DOWN]} :
			std::nullopt;
	}

	std::optional<unsigned int> adj_node_up(unsigned int node)
	{
		unsigned int xzy_i = node / 6;
		unsigned int xz_i = xzy_i / RegionView::CHUNK_HEIGHT;
		unsigned int y = xzy_i % RegionView::CHUNK_HEIGHT;
		unsigned int x = xz_i % view.width();
		unsigned int z = xz_i / view.width();
		x = view.to_real_x(x);
		z = view.to_real_z(z);
		return (y < 255 && view.is_air_block(x, y + 1, z)) ? 
			std::optional<unsigned int>{node + adj_node_off[Side::UP]} :
			std::nullopt;
	}

	std::vector<unsigned int> find_adjacent_nodes(unsigned int node) 
	{
		std::vector<unsigned int> res;
		// add sides
		unsigned int side = node % 6;
		unsigned int base_side_node = node - side;
		for (int i = 0; i < 6; i++) {
			if (base_side_node + i != node) {
				res.push_back(base_side_node + i);
			}
		}
		std::optional<unsigned int> adj_node;
		switch (side)
		{
		case NORTH: adj_node = adj_node_north(node); break;
		case SOUTH: adj_node = adj_node_south(node); break;
		case WEST: adj_node = adj_node_west(node); break;
		case EAST: adj_node = adj_node_east(node); break;
		case UP: adj_node = adj_node_up(node); break;
		case DOWN: adj_node = adj_node_down(node); break;
		}
		if (adj_node) {
			res.push_back(adj_node.value());
		}
		return res;
	}

public:

	TurtlePathGraph(RegionView&& view)
		: view(std::move(view))
	{
		adj_node_off[NORTH] = -view.width() * RegionView::CHUNK_HEIGHT * 6;
		adj_node_off[SOUTH] = view.width() * RegionView::CHUNK_HEIGHT * 6;
		adj_node_off[WEST] = -RegionView::CHUNK_HEIGHT * 6;
		adj_node_off[EAST] = RegionView::CHUNK_HEIGHT * 6;
		adj_node_off[DOWN] = -1 * 6;
		adj_node_off[UP] = 1 * 6;
	}

	unsigned int to_node(int x, int y, int z, Side side) 
	{
		x = view.to_absolute_x(x);
		z = view.to_absolute_z(z);
		unsigned int xz_i = z * view.width() + x;
		unsigned int xzy_i = xz_i * RegionView::CHUNK_HEIGHT + y;
		return xzy_i * 6 + side;
	}

	const std::vector<unsigned int>& adjacent_nodes(unsigned int node) 
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
