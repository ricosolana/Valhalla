#pragma once

#include "Chunk.hpp"
#include <robin_hood.h>
#include <algorithm>
#include <chrono>
#include "Utils.hpp"

namespace Alchyme {
	struct WorldPack {
		// contain world data

	};

	class World {
		robin_hood::unordered_map<std::tuple<int, int>, std::unique_ptr<Chunk>, TwoTupleHasher> m_chunks;

		std::string m_name; // world name
		std::size_t m_seed;

		// several things,
		// cannot make a realtime game, focused on quick responses
		// tcp terrible for this

		// slower paced something

	public:
		// Several options for world construction:
		//	- generate from scratch
		//	- load from file
		//	- loaded by remote and chunks subsequently passed in



		World(); // loaded by remote
		World(std::string name); // load from file
		World(std::string name, std::size_t seed); // newly generate

		void Update();

		void DeserializeWorld(WorldPack* pack);
		void DeserializeChunk(ChunkPack* pack);

		// 
		void Generate();

	};
}
