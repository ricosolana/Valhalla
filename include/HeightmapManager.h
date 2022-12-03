#pragma once

#include "HashUtils.h"
#include "VUtils.h"
#include "HeightMap.h"

//#include <ro>

//class Heightmap;

struct HMBuildData {
	std::array<BitMask<Heightmap::Biome>, 4> m_cornerBiomes;
	//Heightmap::Biome m_cornerBiomes[4];

	//std::vector<float> m_baseHeights;

	//std::vector<float> m_baseHeights;
	// is this cpu cache friendly?
	// track any common usages and determine how is used
	std::array<float, (Heightmap::WIDTH + 1) * (Heightmap::WIDTH + 1)> m_baseHeights;
	//float m_baseHeights[(Heightmap::WIDTH + 1) * (Heightmap::WIDTH + 1)];

	//std::vector<Color> m_baseMask;

	std::array<Color, Heightmap::WIDTH * Heightmap::WIDTH> m_baseMask;
	//Color m_baseMask[Heightmap::WIDTH * Heightmap::WIDTH];
};

class HeightmapManager {
	static robin_hood::unordered_map<Vector2i, std::unique_ptr<Heightmap>, HashUtils::Hasher> m_heightmaps;

public:
	//void ForceGenerateAll();
	static void ForceQueuedRegeneration();

	static float GetOceanDepthAll(const Vector3& worldPos);

	static bool AtMaxLevelDepth(const Vector3& worldPos);
	static bool GetHeight(const Vector3& worldPos, float& height);
	static bool GetAverageHeight(const Vector3& worldPos, float& radius, float height);

	//static std::vector<Heightmap> GetAllHeightmaps();
	static robin_hood::unordered_map<Vector2i, std::unique_ptr<Heightmap>, HashUtils::Hasher>& GetAllHeightmaps();
	static Heightmap* FindHeightmap(const Vector3& point); // terribly slow
	static void FindHeightmap(const Vector3& point, float radius, std::vector<Heightmap*>& heightmaps);
	static Heightmap::Biome FindBiome(const Vector3& point);
	static bool IsRegenerateQueued(const Vector3& point, float radius);
};

//VHeightmapManager* HeightmapManager();
