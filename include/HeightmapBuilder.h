#pragma once

#include "HeightmapManager.h"
#include "Vector.h"
#include "VUtils.h"

namespace HeightmapBuilder {

    void Init();
    void Uninit();

    std::unique_ptr<HMBuildData> RequestTerrainBlocking(const Vector2i& zoneCoord);
    std::unique_ptr<HMBuildData> RequestTerrain(const Vector2i& zoneCoord);
	bool IsTerrainReady(const Vector2i& zoneCoord);
}
