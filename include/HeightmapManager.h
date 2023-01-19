#pragma once

#include "HashUtils.h"
#include "VUtils.h"
#include "HeightMap.h"
#include "TerrainModifier.h"

//#include <ro>

//class Heightmap;

class HMBuildData {
public:
	std::array<Heightmap::Biome, 4> m_cornerBiomes;

	Heightmap::Heights_t m_baseHeights;
	Heightmap::Mask_t m_baseMask;
};

class HeightmapManager {
	static robin_hood::unordered_map<Vector2i, std::unique_ptr<Heightmap>> m_heightmaps;

public:
	//void ForceGenerateAll();
	static void ForceQueuedRegeneration();

	static float GetOceanDepthAll(const Vector3& worldPos);

	static bool AtMaxLevelDepth(const Vector3& worldPos);
	static bool GetHeight(const Vector3& worldPos, float& height);
	static bool GetAverageHeight(const Vector3& worldPos, float& radius, float height);

	//static std::vector<Heightmap> GetAllHeightmaps();
	static robin_hood::unordered_map<Vector2i, std::unique_ptr<Heightmap>>& GetAllHeightmaps();
	static Heightmap* FindHeightmap(const Vector3& point); // terribly slow
	static std::vector<Heightmap*> FindHeightmaps(const Vector3& point, float radius);
	static Heightmap::Biome FindBiome(const Vector3& point);
	static bool IsRegenerateQueued(const Vector3& point, float radius);

	static Heightmap* CreateHeightmap(const Vector2i& zone);
};

//VHeightmapManager* HeightmapManager();
