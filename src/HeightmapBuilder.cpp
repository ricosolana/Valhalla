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
void IHeightmapBuilder::Init() {
    m_builder = std::thread([this]() {
        OPTICK_THREAD("HMBuilder");
        el::Helpers::setThreadName("HMBuilder");

        LOG(INFO) << "Builder started";
        while (!m_stop) {
            OPTICK_FRAME("BuilderThread");
            bool buildMore;
            {
                std::scoped_lock<std::mutex> scoped(m_lock);
                buildMore = !m_toBuild.empty();
            }

            if (buildMore) {
                ZoneID zone;
                {
                    std::scoped_lock<std::mutex> scoped(m_lock);

                    auto&& begin = m_toBuild.begin();

                    zone = *begin;
                    m_toBuild.erase(begin);
                }

                auto base(std::make_unique<BaseHeightmap>());
                Build(base.get(), zone);

                {
                    std::scoped_lock<std::mutex> scoped(m_lock);
                    m_ready[zone] = std::make_unique<Heightmap>(zone, std::move(base));

                    //static auto prev(steady_clock::now());
                    //auto now(steady_clock::now());
                    //if (now - prev > 1min && m_ready.size() > 32) {
                    //    m_ready.clear();
                    //    prev = now;
                    //}

                        

                    // start dropping uneaten heightmaps (poor heightmaps)
                    // not really necessary? Why dorp heightmaps?
                    //while (m_ready.size() > 16) {
                    //    m_ready.erase(m_ready.begin());
                    //}
                }
            }  
                std::this_thread::sleep_for(1ms);
        }
    });
}

void IHeightmapBuilder::Uninit() {
    m_stop = true;
    if (m_builder.joinable())
        m_builder.join();
}



// private
void IHeightmapBuilder::Build(BaseHeightmap *base, const ZoneID& zone) {
    //OPTICK_EVENT();

    auto baseWorldPos = IZoneManager::ZoneToWorldPos(zone) + Vector3((float)IZoneManager::ZONE_SIZE * -0.5f, 0., (float)IZoneManager::ZONE_SIZE * -0.5f);

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
    base->m_baseMask.resize(IZoneManager::ZONE_SIZE * IZoneManager::ZONE_SIZE);

    for (int ry = 0; ry < Heightmap::E_WIDTH; ry++) {
        const float world_y = baseWorldPos.z + ry;
        const float ty = VUtils::Mathf::SmoothStep(0, 1, (float) ry / IZoneManager::ZONE_SIZE);

        for (int rx = 0; rx < Heightmap::E_WIDTH; rx++) {
            const float world_x = baseWorldPos.x + rx;
            const float tx = VUtils::Mathf::SmoothStep(0, 1, (float) rx / IZoneManager::ZONE_SIZE);

            Color color;
            float height;
            if (biome1 == biome2 && biome1 == biome3 && biome1 == biome4) {
                height = GEO->GetBiomeHeight(biome1, world_x, world_y, color);
            }
            else {
                Color colors[4];
                float biomeHeight1 = GEO->GetBiomeHeight(biome1, world_x, world_y, colors[0]);
                float biomeHeight2 = GEO->GetBiomeHeight(biome2, world_x, world_y, colors[1]);
                float biomeHeight3 = GEO->GetBiomeHeight(biome3, world_x, world_y, colors[2]);
                float biomeHeight4 = GEO->GetBiomeHeight(biome4, world_x, world_y, colors[3]);

                Color c1 = colors[0].Lerp(colors[1], tx);
                Color c2 = colors[2].Lerp(colors[3], tx);
                color = c1.Lerp(c2, ty);

                float h1 = VUtils::Mathf::Lerp(biomeHeight1, biomeHeight2, tx);
                float h2 = VUtils::Mathf::Lerp(biomeHeight3, biomeHeight4, tx);
                height = VUtils::Mathf::Lerp(h1, h2, ty);
            }

            base->m_baseHeights[ry * Heightmap::E_WIDTH + rx] = height;

            // color mask is a bit smaller, so check bounds
            if (rx < IZoneManager::ZONE_SIZE && ry < IZoneManager::ZONE_SIZE) {
                base->m_baseMask[ry * IZoneManager::ZONE_SIZE + rx] = color;
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

std::unique_ptr<Heightmap> IHeightmapBuilder::PollHeightmap(const ZoneID& zone) {
    std::scoped_lock<std::mutex> scoped(m_lock);
    auto&& find = m_ready.find(zone);
    if (find != m_ready.end()) {
        auto heightmap = std::move(find->second);
        m_ready.erase(find);
        return heightmap;;
    }

    // Will not insert if absent, which is intended
    m_toBuild.insert(zone);

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
