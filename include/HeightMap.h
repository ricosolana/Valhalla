#pragma once

enum class Biome
{
	None =			0,
	Meadows =		1 << 0,
	Swamp =			1 << 1,
	Mountain =		1 << 2,
	BlackForest =	1 << 3,
	Plains =		1 << 4,
	AshLands =		1 << 5,
	DeepNorth =		1 << 6,
	Ocean =			1 << 7,
	Mistlands =		1 << 8,
	//BiomesMax // seems unused, and its useless
}

// Token: 0x020001DE RID: 478
enum class BiomeArea
{
	Edge =			1 << 0,
	Median =		1 << 1,
	Everything =	Edge | Median;
}