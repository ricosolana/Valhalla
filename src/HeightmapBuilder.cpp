#include <mutex>
#include <future>

#include "HeightmapBuilder.h"
#include "GeoManager.h"
#include "HashUtils.h"
#include "TerrainModifier.h"
#include "VUtilsMathf.h"

auto HEIGHTMAP_BUILDER(std::make_unique<IHeightmapBuilder>());
IHeightmapBuilder* HeightmapBuilder() {
    return HEIGHTMAP_BUILDER.get();
}

// public
void IHeightmapBuilder::PostGeoInit() {
    //int TC = std::max(1, (int)std::thread::hardware_concurrency() - 2);

    for (int i = 0; i < VH_SETTINGS.worldHeightmapThreads; i++) {
        auto&& insert = m_builders.insert(std::end(m_builders), std::make_unique<Shared>());

        Shared* shared = insert->get();

        shared->m_thread = std::jthread([this, i, shared](std::stop_token token) {
            std::string name = "HMBuilder" + std::to_string(i);

            tracy::SetThreadName(name.c_str());
            //el::Helpers::setThreadName(name);

            std::vector<ZoneID> next;

            std::vector<std::unique_ptr<Heightmap>> baked;
           

            LOG_INFO(LOGGER, "Builder thread started");
            while (!token.stop_requested()) {
                FrameMarkStart(name.c_str());

                // Reassign pending heightmaps
                if (next.empty()) {
                    std::scoped_lock<std::mutex> scoped(shared->m_mux);
                    next = std::move(shared->m_waiting);
                }

                baked.clear();

                // Bake any pending heightmaps 
                for (int i = next.size() - 1; i >= 0; --i) {
                    auto&& zone = next[i];
                    auto base(std::make_unique<BaseHeightmap>());
                    Build(base.get(), zone);
                    baked.push_back(std::make_unique<Heightmap>(zone, std::move(base)));
                    //if (baked.size() > next.size() / 10)
                    if (baked.size() > 10)
                        break;

                    // early stop
                    if (token.stop_requested()) {
                        FrameMarkEnd(name.c_str());
                        return;
                    }
                }

                next.resize(next.size() - baked.size());

                /*
                for (auto&& zone : next) {
                    auto base(std::make_unique<BaseHeightmap>());
                    Build(base.get(), zone);
                    baked.push_back(std::make_unique<Heightmap>(zone, std::move(base)));
                    if (baked.size() > next.size() / 10)
                        break;
                }*/

                // Add to the pool of ready heightmaps
                {
                    std::scoped_lock<std::mutex> scoped(m_mux);
                    for (auto&& heightmap : baked)
                        m_ready[heightmap->GetZone()] = std::move(heightmap);
                }

                FrameMarkEnd(name.c_str());

                std::this_thread::sleep_for(1ms);
            }
        });
    }

    m_nextBuilder = m_builders.begin();
}

void IHeightmapBuilder::Uninit() {
    // First request all to stop
    for (auto&& shared : m_builders) {
        shared->m_thread.request_stop();
    }

    // Then join each 
    for (auto&& builder : m_builders) {
        if (builder->m_thread.joinable())
            builder->m_thread.join();
    }
}

void IHeightmapBuilder::Update() {
    PERIODIC_NOW(1min, {
        {
            std::scoped_lock<std::mutex> scoped(m_mux);
            m_ready.clear();
        }
    });
}

// private
void IHeightmapBuilder::Build(BaseHeightmap *base, ZoneID zone) {
    //OPTICK_EVENT();

    auto baseWorldPos = IZoneManager::ZoneToWorldPos(zone) + Vector3f((float)IZoneManager::ZONE_SIZE * -0.5f, 0., (float)IZoneManager::ZONE_SIZE * -0.5f);

    auto GEO(GeoManager());

    //WorldGenerator worldGen = data.m_worldGen;
    //data.m_cornerBiomes = new Heightmap.Biome[4];
    base->m_cornerBiomes[0] = GEO->GetBiome(baseWorldPos.x, baseWorldPos.z);
    base->m_cornerBiomes[1] = GEO->GetBiome(baseWorldPos.x + IZoneManager::ZONE_SIZE, baseWorldPos.z);
    base->m_cornerBiomes[2] = GEO->GetBiome(baseWorldPos.x, baseWorldPos.z + (float)IZoneManager::ZONE_SIZE);
    base->m_cornerBiomes[3] = GEO->GetBiome(baseWorldPos.x + IZoneManager::ZONE_SIZE, baseWorldPos.z + IZoneManager::ZONE_SIZE);

    const auto biome1 = base->m_cornerBiomes[0];
    const auto biome2 = base->m_cornerBiomes[1];
    const auto biome3 = base->m_cornerBiomes[2];
    const auto biome4 = base->m_cornerBiomes[3];

    base->m_baseHeights.resize(Heightmap::E_WIDTH * Heightmap::E_WIDTH);
    base->m_vegMask.resize(IZoneManager::ZONE_SIZE * IZoneManager::ZONE_SIZE);

    for (int ry = 0; ry < Heightmap::E_WIDTH; ry++) {
        const float world_y = baseWorldPos.z + ry;
        const float ty = VUtils::Mathf::SmoothStep(0, 1, (float) ry / IZoneManager::ZONE_SIZE);

        for (int rx = 0; rx < Heightmap::E_WIDTH; rx++) {
            const float world_x = baseWorldPos.x + rx;
            const float tx = VUtils::Mathf::SmoothStep(0, 1, (float) rx / IZoneManager::ZONE_SIZE);

            //Color color = Colors::BLACK;
            float mistlandsMask = 0;
            float height;
            
            assert(tx >= 0 && tx <= 1);
            assert(ty >= 0 && ty <= 1);

            // slight optimization case
            if (biome1 == biome2 && biome1 == biome3 && biome1 == biome4) {
                height = GEO->GetBiomeHeight(biome1, world_x, world_y, mistlandsMask);
            }
            else {
                float mask1 = 0, mask2 = 0, mask3 = 0, mask4 = 0;
                float height1 = GEO->GetBiomeHeight(biome1, world_x, world_y, mask1);
                float height2 = GEO->GetBiomeHeight(biome2, world_x, world_y, mask2);
                float height3 = GEO->GetBiomeHeight(biome3, world_x, world_y, mask3);
                float height4 = GEO->GetBiomeHeight(biome4, world_x, world_y, mask4);

                // this does nothing if no biomes are mistlands
                float c1 = std::lerp(mask1, mask2, tx);
                float c2 = std::lerp(mask3, mask4, tx);
                mistlandsMask = std::lerp(c1, c2, ty);
                
                float h1 = std::lerp(height1, height2, tx);
                float h2 = std::lerp(height3, height4, tx);
                height = std::lerp(h1, h2, ty);
            }

            base->m_baseHeights[ry * Heightmap::E_WIDTH + rx] = height;

            // color mask is a bit smaller, so check bounds
            if (rx < IZoneManager::ZONE_SIZE && ry < IZoneManager::ZONE_SIZE) {
                base->m_vegMask[ry * IZoneManager::ZONE_SIZE + rx] = mistlandsMask;
            }
        }
    }
}

/*
// public
std::unique_ptr<HMBuildData> IHeightmapBuilder::RequestTerrainBlocking(const ZoneID& zone) {
    std::unique_ptr<HMBuildData> hmbuildData;
    do {
        hmbuildData = RequestTerrain(zone);
        if (!hmbuildData) std::this_thread::sleep_for(1ms); // sleep instead of spinlock
    } while (!hmbuildData);
    return hmbuildData;
}*/

/*
void IHeightmapBuilder::QueueBatch(const ZoneID& zone) {
    for (auto&& shared : m_builders) {
        std::scoped_lock<std::mutex> scoped(shared.m_mux);

        for (int i=0; i < )
        shared.m_waiting.push_back
    }
}*/

std::unique_ptr<Heightmap> IHeightmapBuilder::PollHeightmap(ZoneID zone) {
    {
        std::unique_ptr<Heightmap> result;
        {
            std::scoped_lock<std::mutex> scoped(m_mux);
            auto&& find = m_ready.find(zone);
            if (find != m_ready.end()) {
                result = std::move(find->second);
                m_ready.erase(find);
            }
        }

        if (result) {
            m_building.erase(zone);
            return result;
        }
    }

    auto&& insert = m_building.insert(zone);    
    if (insert.second) {
        if (m_nextBuilder == m_builders.end()) m_nextBuilder = m_builders.begin();

        // Give the next builder a job
        //  Ideally, the builder with the least work should be doing this job...
        //  solve problems first
        {
            std::scoped_lock<std::mutex> scoped((*m_nextBuilder)->m_mux);
            (*m_nextBuilder)->m_waiting.push_back(zone);
        }

        ++m_nextBuilder;
    }

    return nullptr;
}

// public
// This is never externally wtf?
// This entire multithreaded (single thread really) chunkbuilder is not even used
// for its intended purpose
/*
std::unique_ptr<HMBuildData> IHeightmapBuilder::RequestTerrain(const ZoneID& zone) {
    std::scoped_lock<std::mutex> scoped(m_lock);
    auto&& find = m_ready.find(zone);
    if (find != m_ready.end()) {
        std::unique_ptr<HMBuildData> data = std::move(find->second);
        m_ready.erase(find);
        return data;
    }

    // Will not insert if absent, which is intended
    m_toBuild.insert(zone);
        
    return nullptr;
}*/

/*
// public
bool IHeightmapBuilder::IsTerrainReady(const ZoneID& zone) {
    std::scoped_lock<std::mutex> scoped(m_lock);

    if (m_ready.contains(zone))
        return true;

    // Will not insert if absent, which is intended
    m_toBuild.insert(zone);

    return false;
}
*/
