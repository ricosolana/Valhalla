#include <mutex>

#include "HeightmapBuilder.h"
#include "AsyncDeque.h"
#include "WorldGenerator.h"
#include "HashUtils.h"

namespace HeightmapBuilder {

    void Build(HMBuildData *data, const Vector2i &center);



    robin_hood::unordered_map<Vector2i, std::unique_ptr<HMBuildData>, HashUtils::Hasher> m_toBuild;
    robin_hood::unordered_map<Vector2i, std::unique_ptr<HMBuildData>, HashUtils::Hasher> m_ready;

    std::thread m_builder;

    std::mutex m_lock;

    std::atomic_bool m_stop;



    // public
    void Init() {
        m_builder = std::thread([]() {
            LOG(INFO) << "Builder started";
            while (!m_stop) {
                bool flag;
                {
                    std::scoped_lock<std::mutex> scoped(m_lock);
                    flag = !m_toBuild.empty();
                }

                if (flag) {
                    std::unique_ptr<HMBuildData> hmbuildData;
                    Vector2i center;
                    {
                        std::scoped_lock<std::mutex> scoped(m_lock);
                        auto&& begin = m_toBuild.begin();
                        hmbuildData = std::move(begin->second);
                        center = begin->first;
                        m_toBuild.erase(begin);
                    }

                    Build(hmbuildData.get(), center);

                    {
                        std::scoped_lock<std::mutex> scoped(m_lock);
                        m_ready[center] = std::move(hmbuildData);

                        // start dropping unpolled heightmaps (what a shame...)
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
    void Build(HMBuildData *data, const Vector2i& center) {
        const int32_t num = Heightmap::WIDTH + 1;
        const int32_t num2 = num * num;
        auto vector = center + Vector2i((float)Heightmap::WIDTH * -0.5f, (float)Heightmap::WIDTH * -0.5f);

        //WorldGenerator worldGen = data.m_worldGen;
        //data.m_cornerBiomes = new Heightmap.Biome[4];
        data->m_cornerBiomes[0] = WorldGenerator::GetBiome(vector.x, vector.y);
        data->m_cornerBiomes[1] = WorldGenerator::GetBiome(vector.x + (float)Heightmap::WIDTH, vector.y);
        data->m_cornerBiomes[2] = WorldGenerator::GetBiome(vector.x, vector.y + (float)Heightmap::WIDTH);
        data->m_cornerBiomes[3] = WorldGenerator::GetBiome(vector.x + (float)Heightmap::WIDTH, vector.y + (float)Heightmap::WIDTH);

        auto biome = data->m_cornerBiomes[0];
        auto biome2 = data->m_cornerBiomes[1];
        auto biome3 = data->m_cornerBiomes[2];
        auto biome4 = data->m_cornerBiomes[3];

        // This also fills in 0's
        data->m_baseHeights.resize(num2);

        const int32_t num3 = Heightmap::WIDTH * Heightmap::WIDTH;
        //data.m_baseMask = new Color[num3];
        //for (int32_t j = 0; j < num3; j++) {
            //data.m_baseMask[j] = new Color(0f, 0f, 0f, 0f);
        //}

        for (int32_t k = 0; k < num; k++) {
            float wy = vector.y + (float)k;
            float t = VUtils::Math::SmoothStep(0, 1, (float)k / (float)Heightmap::WIDTH);
            for (int32_t l = 0; l < num; l++) {
                float wx = vector.x + (float)l;
                float t2 = VUtils::Math::SmoothStep(0, 1, (float)l / (float)Heightmap::WIDTH);
                Color color;
                float value;

                if (Heightmap::DISTANT_LOD) {
                    auto biome5 = WorldGenerator::GetBiome(wx, wy);
                    value = WorldGenerator::GetBiomeHeight(biome5, wx, wy, color);
                }
                else if (biome3 == biome && biome2 == biome && biome4 == biome) {
                    value = WorldGenerator::GetBiomeHeight(biome, wx, wy, color);
                }
                else {
                    Color colors[4];
                    float biomeHeight = WorldGenerator::GetBiomeHeight(biome, wx, wy, colors[0]);
                    float biomeHeight2 = WorldGenerator::GetBiomeHeight(biome2, wx, wy, colors[1]);
                    float biomeHeight3 = WorldGenerator::GetBiomeHeight(biome3, wx, wy, colors[2]);
                    float biomeHeight4 = WorldGenerator::GetBiomeHeight(biome4, wx, wy, colors[3]);
                    float a = VUtils::Math::Lerp(biomeHeight, biomeHeight2, t2);
                    float b = VUtils::Math::Lerp(biomeHeight3, biomeHeight4, t2);
                    value = VUtils::Math::Lerp(a, b, t);

                    Color a2 = colors[0].Lerp(colors[1], t2);
                    Color b2 = colors[2].Lerp(colors[3], t2);
                    color = a2.Lerp(b2, t);
                }
                data->m_baseHeights[k * num + l] = value;
                if (l < Heightmap::WIDTH && k < Heightmap::WIDTH) {
                    data->m_baseMask[k * Heightmap::WIDTH + l] = color;
                }
            }
        }

        if (Heightmap::DISTANT_LOD) {
            for (int32_t m = 0; m < 4; m++) {
                std::vector<float> list(data->m_baseHeights);

                for (int32_t n = 1; n < num - 1; n++) {
                    for (int32_t num4 = 1; num4 < num - 1; num4++) {
                        #define PITCH(vec, x, y) vec[(y)*num + (x)];

                        float num5 = PITCH(list, num4 + 0, n + 0); // center

                        float num6 = PITCH(list, num4 + 0, n - 1); // up
                        float num7 = PITCH(list, num4 + 0, n + 1); // down
                        float num8 = PITCH(list, num4 - 1, n + 0); // left
                        float num9 = PITCH(list, num4 + 1, n + 0); // right

                        //float num5 = list[n * num + num4];
                        //float num6 = list[(n - 1) * num + num4];
                        //float num7 = list[(n + 1) * num + num4];
                        //float num8 = list[n * num + num4 - 1];
                        //float num9 = list[n * num + num4 + 1];

                        // Compare neighboring height deltas
                        if (abs(num5 - num6) > 10) {
                            num5 = (num5 + num6) * 0.5f;
                        }
                        if (abs(num5 - num7) > 10) {
                            num5 = (num5 + num7) * 0.5f;
                        }
                        if (abs(num5 - num8) > 10) {
                            num5 = (num5 + num8) * 0.5f;
                        }
                        if (abs(num5 - num9) > 10) {
                            num5 = (num5 + num9) * 0.5f;
                        }
                        data->m_baseHeights[n * num + num4] = num5;
                    }
                }
            }
        }
    }

    // public
    std::unique_ptr<HMBuildData> RequestTerrainBlocking(const Vector2i& center) {
        std::unique_ptr<HMBuildData> hmbuildData;
        do {
            hmbuildData = RequestTerrain(center);
        } while (!hmbuildData);
        return hmbuildData;
    }

    // public
    // This is never externally wtf?
    // This entire multithreaded (single thread really) chunkbuilder is not even used
    // for its intended purpose
    std::unique_ptr<HMBuildData> RequestTerrain(const Vector2i& center) {
        std::scoped_lock<std::mutex> scoped(m_lock);
        auto&& find = m_ready.find(center);
        if (find != m_ready.end()) {
            std::unique_ptr<HMBuildData> data = std::move(find->second);
            m_ready.erase(find);
            return data;
        }

        if (!m_toBuild.contains(center)) {
            m_toBuild[center] = std::make_unique<HMBuildData>();
        }
        
        return nullptr;
    }

    // public
    bool IsTerrainReady(const Vector2i& center) {
        std::scoped_lock<std::mutex> scoped(m_lock);
        if (m_ready.contains(center))
            return true;

        if (!m_toBuild.contains(center)) {
            m_toBuild[center] = std::make_unique<HMBuildData>();
        }

        return false;
    }

}
