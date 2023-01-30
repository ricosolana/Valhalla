#include <mutex>
#include <future>

#include "HeightmapBuilder.h"
#include "GeoManager.h"
#include "HashUtils.h"
#include "TerrainModifier.h"

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
                        while (m_ready.size() > 16) {
                            m_ready.erase(m_ready.begin());
                        }
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
    void Build(HMBuildData *data, const ZoneID& center) {
        constexpr int num = IZoneManager::ZONE_SIZE + 1;
        constexpr int num2 = num * num;
        constexpr int num3 = IZoneManager::ZONE_SIZE * IZoneManager::ZONE_SIZE;
        auto vector = center + Vector2i((float)IZoneManager::ZONE_SIZE * -0.5f, (float)IZoneManager::ZONE_SIZE * -0.5f);

        auto GEO(GeoManager());

        //WorldGenerator worldGen = data.m_worldGen;
        //data.m_cornerBiomes = new Heightmap.Biome[4];
        data->m_cornerBiomes[0] = GEO->GetBiome(vector.x, vector.y);
        data->m_cornerBiomes[1] = GEO->GetBiome(vector.x + IZoneManager::ZONE_SIZE, vector.y);
        data->m_cornerBiomes[2] = GEO->GetBiome(vector.x, vector.y + (float)IZoneManager::ZONE_SIZE);
        data->m_cornerBiomes[3] = GEO->GetBiome(vector.x + IZoneManager::ZONE_SIZE, vector.y + IZoneManager::ZONE_SIZE);

        auto biome = data->m_cornerBiomes[0];
        auto biome2 = data->m_cornerBiomes[1];
        auto biome3 = data->m_cornerBiomes[2];
        auto biome4 = data->m_cornerBiomes[3];

        data->m_baseHeights.resize(num2);
        data->m_baseMask.resize(num3);

        for (int32_t k = 0; k < num; k++) {
            float wy = vector.y + (float)k;
            float t = VUtils::Math::SmoothStep(0, 1, (float)k / (float)IZoneManager::ZONE_SIZE);
            for (int32_t l = 0; l < num; l++) {
                float wx = vector.x + (float)l;
                float t2 = VUtils::Math::SmoothStep(0, 1, (float)l / (float)IZoneManager::ZONE_SIZE);
                Color color;
                float value;

                if (biome == biome2 && biome == biome3 && biome == biome4) {
                    value = GEO->GetBiomeHeight(biome, wx, wy, color);
                }
                else {
                    Color colors[4];
                    float biomeHeight = GEO->GetBiomeHeight(biome, wx, wy, colors[0]);
                    float biomeHeight2 = GEO->GetBiomeHeight(biome2, wx, wy, colors[1]);
                    float biomeHeight3 = GEO->GetBiomeHeight(biome3, wx, wy, colors[2]);
                    float biomeHeight4 = GEO->GetBiomeHeight(biome4, wx, wy, colors[3]);

                    float a = VUtils::Math::Lerp(biomeHeight, biomeHeight2, t2);
                    float b = VUtils::Math::Lerp(biomeHeight3, biomeHeight4, t2);
                    value = VUtils::Math::Lerp(a, b, t);

                    Color a2 = colors[0].Lerp(colors[1], t2);
                    Color b2 = colors[2].Lerp(colors[3], t2);
                    color = a2.Lerp(b2, t);
                }

                data->m_baseHeights[k * num + l] = value;

                if (l < IZoneManager::ZONE_SIZE && k < IZoneManager::ZONE_SIZE) {
                    //if (color.a > .5f) {
                    //    data->m_baseMask[k * Heightmap::WIDTH + l] = TerrainModifier::PaintType::Reset;
                    //}
                    //else if (color.g > .5f) {
                    //    data->m_baseMask[k * Heightmap::WIDTH + l] = TerrainModifier::PaintType::Cultivate;
                    //}
                    data->m_baseMask[k * IZoneManager::ZONE_SIZE + l] = color;
                }
                else {
                    //data->m_baseMask[k * IZoneManager::ZONE_SIZE + l] = Colors::BLACK;
                    //data->m_baseMask[k * Heightmap::WIDTH + l] = TerrainModifier::PaintType::Reset;
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
