#pragma once

class Heightmap {
public:
	struct Biome {
		static constexpr int None =			0;
		static constexpr int Meadows =		1 << 0;
		static constexpr int Swamp =		1 << 1;
		static constexpr int Mountain =		1 << 2;
		static constexpr int BlackForest =	1 << 3;
		static constexpr int Plains =		1 << 4;
		static constexpr int AshLands =		1 << 5;
		static constexpr int DeepNorth =	1 << 6;
		static constexpr int Ocean =		1 << 7;
		static constexpr int Mistlands =	1 << 8;
	};

	//enum class Biome
	//{
	//	None = 0,
	//	Meadows = 1 << 0,
	//	Swamp = 1 << 1,
	//	Mountain = 1 << 2,
	//	BlackForest = 1 << 3,
	//	Plains = 1 << 4,
	//	AshLands = 1 << 5,
	//	DeepNorth = 1 << 6,
	//	Ocean = 1 << 7,
	//	Mistlands = 1 << 8,
	//	//BiomesMax // seems unused
	//};

	// Token: 0x020001DE RID: 478
	enum class BiomeArea
	{
		Edge = 1 << 0,
		Median = 1 << 1,
		Everything = Edge | Median,
	};
};
