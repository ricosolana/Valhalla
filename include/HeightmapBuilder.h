#pragma once

#include <quill/Logger.h>
#include <thread>

#include "VUtils.h"

#if VH_IS_ON(VH_ZONE_GENERATION)
#include "HeightmapManager.h"
#include "Vector.h"

#include "ZoneManager.h"

class IHeightmapBuilder {
    struct Shared {
        std::vector<ZoneID> m_waiting;
        //std::vector<std::unique_ptr<Heightmap>> m_waiting;
        std::mutex m_mux;
        std::jthread m_thread;
    };

private:
    quill::Logger* m_logger;

    //avledet::util::Set<ZoneID> m_toBuild;
    avledet::util::Set<ZoneID> m_building;
    avledet::util::Map<ZoneID, std::unique_ptr<Heightmap>> m_ready;
    
    std::mutex m_mux;
    std::vector<std::unique_ptr<Shared>> m_builders;
    decltype(m_builders)::iterator m_nextBuilder;

private:
    static void Build(BaseHeightmap* data, ZoneID zone);

public:
    void PostGeoInit();
    void Uninit();

    void Update();
    
    //void QueueBatch(const std::ZoneID& zone);

    std::unique_ptr<Heightmap> PollHeightmap(ZoneID zone);

    //std::unique_ptr<HMBuildData> RequestTerrainBlocking(const ZoneID& zone);
    //std::unique_ptr<HMBuildData> RequestTerrain(const ZoneID& zone);
    //bool IsTerrainReady(const ZoneID& zone);
};

IHeightmapBuilder* HeightmapBuilder();
#endif