#include <mutex>
#include <future>

#include "HeightmapBuilder.h"
#include "GeoManager.h"
#include "HashUtils.h"
#include "TerrainModifier.h"
#include "VUtilsMathf.h"

namespace HeightmapBuilder {

    void Build(HMBuildData *data, const ZoneID &center);

    robin_hood::unordered_set<ZoneID> m_toBuild;
    robin_hood::unordered_map<ZoneID, std::unique_ptr<HMBuildData>> m_ready;

    std::thread m_builder;

    std::mutex m_lock;

    std::atomic_bool m_stop;



    // public
    void Init() {
        m_builder = std::thread([]() {
            OPTICK_THREAD("HMBuilder");
            el::Helpers::setThreadName("HMBuilder");

            LOG(INFO) << "Builder started";
            while (!m_stop) {
                bool buildMore;
                {
                    std::scoped_lock<std::mutex> scoped(m_lock);
                    buildMore = !m_toBuild.empty();
                }

                if (buildMore) {
                    Vector2i center;
                    {
                        std::scoped_lock<std::mutex> scoped(m_lock);

                        auto&& begin = m_toBuild.begin();

                        center = *begin;
                        m_toBuild.erase(begin);
                    }

                    auto data(std::make_unique<HMBuildData>());
                    Build(data.get(), center);

                    {
                        std::scoped_lock<std::mutex> scoped(m_lock);
                        m_ready[center] = std::move(data);

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

    void Uninit() {
        m_stop = true;
        assert(m_builder.joinable());
        m_builder.join();
    }



    // private
    void Build(HMBuildData *data, const ZoneID& zone) {
        auto baseWorldPos = IZoneManager::ZoneToWorldPos(zone) + Vector3((float)IZoneManager::ZONE_SIZE * -0.5f, 0., (float)IZoneManager::ZONE_SIZE * -0.5f);

        auto GEO(GeoManager());

        //WorldGenerator worldGen = data.m_worldGen;
        //data.m_cornerBiomes = new Heightmap.Biome[4];
        data->m_cornerBiomes[0] = GEO->GetBiome(baseWorldPos.x, baseWorldPos.z);
        data->m_cornerBiomes[1] = GEO->GetBiome(baseWorldPos.x + IZoneManager::ZONE_SIZE, baseWorldPos.z);
        data->m_cornerBiomes[2] = GEO->GetBiome(baseWorldPos.x, baseWorldPos.z + (float)IZoneManager::ZONE_SIZE);
        data->m_cornerBiomes[3] = GEO->GetBiome(baseWorldPos.x + IZoneManager::ZONE_SIZE, baseWorldPos.z + IZoneManager::ZONE_SIZE);

        const auto biome1 = data->m_cornerBiomes[0];
        const auto biome2 = data->m_cornerBiomes[1];
        const auto biome3 = data->m_cornerBiomes[2];
        const auto biome4 = data->m_cornerBiomes[3];

        data->m_baseHeights.resize(Heightmap::E_WIDTH * Heightmap::E_WIDTH);
        data->m_baseMask.resize(IZoneManager::ZONE_SIZE * IZoneManager::ZONE_SIZE);

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

                data->m_baseHeights[ry * Heightmap::E_WIDTH + rx] = height;

                // color mask is a bit smaller, so check bounds
                if (rx < IZoneManager::ZONE_SIZE && ry < IZoneManager::ZONE_SIZE) {
                    data->m_baseMask[ry * IZoneManager::ZONE_SIZE + rx] = color;
                }
            }
        }
    }

    // public
    std::unique_ptr<HMBuildData> RequestTerrainBlocking(const ZoneID& zone) {
        std::unique_ptr<HMBuildData> hmbuildData;
        do {
            hmbuildData = RequestTerrain(zone);
            if (!hmbuildData) std::this_thread::sleep_for(1ms); // sleep instead of spinlock
        } while (!hmbuildData);
        return hmbuildData;
    }

    // public
    // This is never externally wtf?
    // This entire multithreaded (single thread really) chunkbuilder is not even used
    // for its intended purpose
    std::unique_ptr<HMBuildData> RequestTerrain(const ZoneID& zone) {
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
    }

    // public
    bool IsTerrainReady(const ZoneID& zone) {
        std::scoped_lock<std::mutex> scoped(m_lock);

        if (m_ready.contains(zone))
            return true;

        // Will not insert if absent, which is intended
        m_toBuild.insert(zone);

        return false;
    }

}
