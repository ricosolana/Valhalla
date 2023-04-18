#pragma once

#include <thread>

#include "HeightmapManager.h"
#include "Vector.h"
#include "VUtils.h"
#include "ZoneManager.h"

class IHeightmapBuilder {
    struct Shared {
        std::vector<ZoneID> m_waiting;
        //std::vector<std::unique_ptr<Heightmap>> m_waiting;
        std::mutex m_mux;
        std::jthread m_thread;
    };

private:
    //UNORDERED_SET_t<ZoneID> m_toBuild;
    UNORDERED_SET_t<ZoneID> m_building;
    UNORDERED_MAP_t<ZoneID, std::unique_ptr<Heightmap>> m_ready;
    
    std::mutex m_mux;
    std::vector<std::unique_ptr<Shared>> m_builders;
    decltype(m_builders)::iterator m_nextBuilder;

private:
    static void Build(BaseHeightmap* data, const ZoneID& zone);

public:
    void PostGeoInit();
    void Uninit();

    void Update();
    
    //void QueueBatch(const std::ZoneID& zone);

    std::unique_ptr<Heightmap> PollHeightmap(const ZoneID& zone);

    //std::unique_ptr<HMBuildData> RequestTerrainBlocking(const ZoneID& zone);
    //std::unique_ptr<HMBuildData> RequestTerrain(const ZoneID& zone);
    //bool IsTerrainReady(const ZoneID& zone);
};

IHeightmapBuilder* HeightmapBuilder();
