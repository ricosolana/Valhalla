#pragma once

#include "HashUtils.h"
#include "VUtils.h"
#include "HeightMap.h"
#include "TerrainModifier.h"
#include "Types.h"

#if VH_IS_ON(VH_ZONE_GENERATION)

class IHeightmapManager {
	UNORDERED_MAP_t<ZoneID, std::unique_ptr<Heightmap>> m_heightmaps;

public:
	Heightmap* poll(ZoneID zone);

	Heightmap &get_heightmap(Vector3f point);
	Heightmap &get_heightmap(ZoneID zone);
};

// Manager class for everything related to finely partitioned world heights and biomes during generation
IHeightmapManager* HeightmapManager();
#endif