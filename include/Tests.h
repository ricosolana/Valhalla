#pragma once

#include "ZDO.h"
#include "NetPackage.h"

namespace Tests {

    void Test_NetSync() {
        ZDO zdo;

        zdo.Set("health", (float) 16.894);
        zdo.Set("weight", (int) 435);
        zdo.Set("slot", (int) 3);
        zdo.Set("name", "byeorgssen");
        zdo.Set("faction", "player");
        zdo.Set("uid", (HASH_t) 189347813489);

        NetPackage savePkg; zdo.Save(savePkg); savePkg.m_stream.SetPos(0);

        ZDO loadedZDO(savePkg, Version::PGW);

        assert(zdo.GetFloat(        "health"    ) == (float) 16.894);
        assert(zdo.GetInt(            "weight"    ) == (int) 435);
        assert(zdo.GetInt(            "slot"        ) == (int) 3);
        assert(zdo.GetString(        "name"        ) == "byeorgssen");
        assert(zdo.GetString(        "faction"    ) == "player");
        assert(zdo.GetInt(            "uid"        ) == (HASH_t) 189347813489);

        assert(loadedZDO.GetFloat(    "health"    ) == (float) 16.894);
        assert(loadedZDO.GetInt(    "weight"    ) == (int) 435);
        assert(loadedZDO.GetInt(    "slot"        ) == (int) 3);
        assert(loadedZDO.GetString(    "name"        ) == "byeorgssen");
        assert(loadedZDO.GetString(    "faction"    ) == "player");
        assert(loadedZDO.GetInt(    "uid"        ) == (HASH_t) 189347813489);
    }

    void Test_ResourceReadWrite() {
        std::vector<std::string> linesToWrite;
        int itr = rand();
        for (int i = 0; i < itr; ++i) linesToWrite.push_back(VUtils::Random::GenerateAlphaNum((rand() % 10) + 10));
        VUtils::Resource::WriteFileLines("test_write_lines.txt", linesToWrite);

        // now read file
        auto opt = VUtils::Resource::ReadFileLines("test_write_lines.txt");
        assert(opt && "file not found");
        assert(opt.value() == linesToWrite);
    }

    void Test_Random() {
        // The plan is to read in the Unity random file with many values, then compare against it

        // read in integers separated by a \n
        auto opt = VUtils::Resource::ReadFileLines("random_values.txt");

        assert(opt && "file not found");

        auto&& values = opt.value();

        // the first line is the seed
        VUtils::Random::State state(std::stoi(values[0]));

        for (int i = 0; i < 100; ++i) {
            assert(state.Range(std::numeric_limits<int>::min(), std::numeric_limits<int>::max())
                == std::stoi(values[i + 1]));
        }
    }

    void Test_Perlin() {
        auto opt = VUtils::Resource::ReadFileLines("perlin_values.txt");

        assert(opt && "file not found");

        auto&& values = opt.value();

        // the first line is the seed
        int next = 0;
        for (float y = -1.1f; y < 1.1f; y += .3f)
        {
            for (float x = -1.1f; x < 1.1f; x += .1f) {
                // Negative floats being truncated cause the mismatch
                //if (x >= 0 && y >= 0)
                {
                    float calc = VUtils::Math::PerlinNoise(x, y);
                    float other = std::stof(values[next]);
                    // not ideal because floating point has en epsilon diff
                    //assert(calc == other)

                    static constexpr float EPS = 0.0001f;
                    assert((calc - EPS < other && calc + EPS > other));
                }
                next++;
            }
        }
    }

}
