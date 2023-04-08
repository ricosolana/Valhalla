#pragma once

#include "HeightmapManager.h"
#include "Vector.h"
#include "VUtils.h"
#include "ZoneManager.h"

class IHeightmapBuilder {
private:
    UNORDERED_SET_t<ZoneID> m_toBuild;
    UNORDERED_MAP_t<ZoneID, std::unique_ptr<Heightmap>> m_ready;

    std::thread m_builder;
    std::mutex m_lock;
    std::atomic_bool m_stop;

private:
    static void Build(BaseHeightmap* data, const ZoneID& zone);

public:
    void Init();
    void Uninit();
    
    std::unique_ptr<Heightmap> PollHeightmap(const ZoneID& zone);

    //std::unique_ptr<HMBuildData> RequestTerrainBlocking(const ZoneID& zone);
    //std::unique_ptr<HMBuildData> RequestTerrain(const ZoneID& zone);
    //bool IsTerrainReady(const ZoneID& zone);
};

IHeightmapBuilder* HeightmapBuilder();
