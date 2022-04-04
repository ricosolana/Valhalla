#pragma once

#include <array>

namespace Alchyme {
	class World;

	struct ChunkPack {
		int x, y;
		//std::array<Chunk::TileType, Chunk::WIDTH* Chunk::WIDTH> m_tiles;
	};

	class Chunk {

	public:
		using TileType = uint16_t;

		static constexpr auto WIDTH = 32;

		// cool games 
		//	- devil daggers
		//	- ultrakill
		//	- 

		World* world;

		std::array<TileType, WIDTH* WIDTH> m_tiles;

	public:
		Chunk();

	};
}
