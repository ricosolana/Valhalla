#pragma once

#include "HashUtils.h"
#include "VUtils.h"
#include "HeightMap.h"
#include "TerrainModifier.h"
#include "Biome.h"


class IHeightmapManager {
	robin_hood::unordered_map<Vector2i, std::unique_ptr<Heightmap>> m_heightmaps;

public:
	//void ForceGenerateAll();
	void ForceQueuedRegeneration();

	float GetOceanDepthAll(const Vector3& worldPos);

	bool AtMaxLevelDepth(const Vector3& worldPos);

	// Get the heightmap height at position
	//	Will only succeed given the heightmap exists
	bool GetHeight(const Vector3& worldPos, float& height);
	bool GetAverageHeight(const Vector3& worldPos, float& radius, float height);

	// Get the heightmap height at position,
	//	The heightmap will be created if it does not exist
	float GetHeight(const Vector3& worldPos);

	//static std::vector<Heightmap> GetAllHeightmaps();
	robin_hood::unordered_map<Vector2i, std::unique_ptr<Heightmap>>& GetAllHeightmaps();

	Heightmap* GetOrCreateHeightmap(const Vector2i& zoneID);

	Heightmap* GetHeightmap(const Vector3& point); // terribly slow
	Heightmap* GetHeightmap(const ZoneID& zone);
	std::vector<Heightmap*> GetHeightmaps(const Vector3& point, float radius);
	Biome FindBiome(const Vector3& point);
	bool IsRegenerateQueued(const Vector3& point, float radius);

	//Heightmap* CreateHeightmap(const Vector2i& zone);
};

IHeightmapManager* HeightmapManager();
