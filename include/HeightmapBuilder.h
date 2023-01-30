#pragma once

#include "HeightmapManager.h"
#include "Vector.h"
#include "VUtils.h"
#include "ZoneManager.h"

namespace HeightmapBuilder {

    void Init();
    void Uninit();

    std::unique_ptr<HMBuildData> RequestTerrainBlocking(const ZoneID& zone);
    std::unique_ptr<HMBuildData> RequestTerrain(const ZoneID& zone);
	bool IsTerrainReady(const ZoneID& zone);
}
