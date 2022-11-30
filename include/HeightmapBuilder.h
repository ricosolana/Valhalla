#pragma once

#include "HeightMap.h"
#include "Vector.h"
#include "VUtils.h"

namespace HeightmapBuilder {

    struct HMBuildData {
        Heightmap::Biome m_cornerBiomes[4];

        std::vector<float> m_baseHeights;

        Color m_baseMask[Heightmap::WIDTH * Heightmap::WIDTH];
    };

    void Init();
    void Uninit();

    std::unique_ptr<HMBuildData> RequestTerrainBlocking(const Vector2i& center);
    std::unique_ptr<HMBuildData> RequestTerrain(const Vector2i& center);
	bool IsTerrainReady(const Vector2i& center);
}
