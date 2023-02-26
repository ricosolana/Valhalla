#pragma once

#include "ZDO.h"
#include "WorldManager.h"
#include "DataWriter.h"
#include "NetManager.h"

class Tests {
public:   
    void Test_QuaternionLook() {
        auto opt = VUtils::Resource::ReadFileLines("lookrotation_values.txt");

        assert(opt && "file not found");

        auto&& values = opt.value();
        int index = 0;

        for (float z = -1; z < 1; z += .05f)
        {
            for (float y = -1; y < 1; y += .05f)
            {
                for (float x = -1; x < 1; x += .05f)
                {
                    auto vec = Vector3(x, y, z);
                    if (vec.Magnitude() <= 0.0001f)
                        continue;

                    auto calc = Quaternion::LookRotation(Vector3(x, y, z));
                    auto expect = Quaternion(
                        std::stof(values[index + 0]),
                        std::stof(values[index + 1]),
                        std::stof(values[index + 2]),
                        std::stof(values[index + 3])
                    );

                    index += 4;

                    //if (y < 180 || x < 60)
                        //continue;

                    static constexpr float EPS = 0.0001f;
                    assert((calc.x - EPS < expect.x&& calc.x + EPS > expect.x));
                    assert((calc.y - EPS < expect.y&& calc.y + EPS > expect.y));
                    assert((calc.z - EPS < expect.z&& calc.z + EPS > expect.z));
                    assert((calc.w - EPS < expect.w&& calc.w + EPS > expect.w));
                }
            }
        }
    }

    void Test_QuaternionEuler() {
        auto opt = VUtils::Resource::ReadFileLines("euler_values.txt");

        assert(opt && "file not found");

        auto&& values = opt.value();
        int index = 0;

        for (float z = 0; z < 360; z += 45)
        {
            for (float y = 0; y < 360; y += 30)
            {
                for (float x = 0; x < 360; x += 10)
                {
                    auto calc = Quaternion::Euler(x, y, z);
                    auto expect = Quaternion(
                        std::stof(values[index + 0]),
                        std::stof(values[index + 1]),
                        std::stof(values[index + 2]),
                        std::stof(values[index + 3])
                    );

                    index += 4;

                    //if (y < 180 || x < 60)
                        //continue;

                    static constexpr float EPS = 0.0001f;
                    assert((calc.x - EPS < expect.x && calc.x + EPS > expect.x));
                    assert((calc.y - EPS < expect.y && calc.y + EPS > expect.y));
                    assert((calc.z - EPS < expect.z && calc.z + EPS > expect.z));
                    assert((calc.w - EPS < expect.w && calc.w + EPS > expect.w));

                    
                }
            }
        }
    }
    
    void Test_PeerLuaConnect() {
        Peer ref(nullptr, 123456789, "eikthyr", Vector3::ZERO);

        Peer* peer = &ref;

        // tests a fake player
        ModManager()->Init();

        ModManager()->CallEvent(VUtils::String::GetStableHashCode("PeerInfo"), peer);
    }

    void Test_DataBuffer() {
        //constexpr int has = is_container<std::vector<int>>::value;
        //constexpr int has = is_container<int>::value;
        //
        //constexpr int has = is_container_of<int, int>::value;
        //constexpr int has1 = is_container_of<std::vector<int>, int>::value;
        //
        //constexpr int has2 = has_value_type<std::vector<int>, int>::value;

        //std::indirectly_readable_traits<std::vector<int>>::

        //constexpr int i = has_value_type<int>::value;
        //constexpr int i = has_value_type<std::vector<int>>::value;

        //std::iter_value_t<int>

        BYTES_t bytes;
        DataWriter writer(bytes);

        //int count = 1114111;
        //int count = 111111;

        int count = 0xFFF1;

        writer.WriteChar(count);

        assert(count == DataReader(bytes).ReadChar());

    }

    void Test_World() {
        WorldManager()->BackupFileWorldDB("world");

        auto world = WorldManager()->GetWorld("privUWorld");
        WorldManager()->LoadFileWorldDB("02129");
    }

    void Test_ZDO() {

        // Unique hash tests
        {
            {
#ifdef RUN_TESTS
                const HASH_t hash1 = 14516234;
                assert(ZDO::FromShiftHash<float>(ZDO::ToShiftHash<float>(hash1)) == hash1);

                const HASH_t hash2 = 56827231;
                assert(ZDO::FromShiftHash<int>(ZDO::ToShiftHash<int>(hash2)) == hash2);

                const HASH_t hash3 = 906582783;
                assert(ZDO::FromShiftHash<std::string>(ZDO::ToShiftHash<std::string>(hash3)) == hash3);
#endif
            }



        }

        // Valheim-sourced Load tests
        {
            auto opt = VUtils::Resource::ReadFileBytes("zdo.sav");
            assert(opt);

            ZDO zdo;
            BYTES_t bytes;
            DataReader pkg(opt.value());
            zdo.Load(pkg, VConstants::WORLD);

            assert(zdo.GetFloat("health", 0) == 3.1415926535f);
            assert(zdo.GetInt("weight", 0) == 435);
            assert(zdo.GetInt("slot", 0) == 3);
            assert(zdo.GetString("name", "") == "byeorgssen");
            assert(zdo.GetString("faction", "") == "player");
            assert(zdo.GetInt("uid", 0) == 189341389);
        }

        // Set/Get tests
        {
            ZDO zdo;

            zdo.Set("health", 3.1415926535f);
            zdo.Set("weight", 435);
            zdo.Set("slot", 3);
            zdo.Set("name", "byeorgssen");
            zdo.Set("faction", "player");
            zdo.Set("uid", 189341389);

            assert(zdo.GetFloat("health", 0) == 3.1415926535f);
            assert(zdo.GetInt("weight", 0) == 435);
            assert(zdo.GetInt("slot", 0) == 3);
            assert(zdo.GetString("name", "") == "byeorgssen");
            assert(zdo.GetString("faction", "") == "player");
            assert(zdo.GetInt("uid", 0) == 189341389);
        }

        // Save/Load tests
        {
            ZDO zdo;

            //zdo.Set("health", 3.1415926535f);
            //zdo.Set("weight", 435);
            //zdo.Set("slot", 3);
            //zdo.Set("name", "byeorgssen");
            zdo.Set("faction", "player");
            //zdo.Set("uid", 189341389);
            
            BYTES_t bytes;
            {
                DataWriter pkg(bytes);
                zdo.Save(pkg);
            }

            ZDO zdo2;
            DataReader reader(bytes);
            zdo2.Load(reader, VConstants::WORLD);

            //assert(zdo2.GetFloat("health", 0) == 3.1415926535f);
            //assert(zdo2.GetInt("weight", 0) == 435);
            //assert(zdo2.GetInt("slot", 0) == 3);
            //assert(zdo2.GetString("name", "") == "byeorgssen");
            assert(zdo2.GetString("faction", "") == "player");
            //assert(zdo2.GetInt("uid", 0) == 189341389);
        }

        // Serialize/Deserialize tests
        {
            ZDO zdo;

            zdo.Set("health", 3.1415926535f);
            zdo.Set("weight", 435);
            zdo.Set("slot", 3);
            zdo.Set("name", "byeorgssen");
            zdo.Set("faction", "player");
            zdo.Set("uid", 189341389);

            BYTES_t bytes;
            DataWriter writer(bytes);
            zdo.Serialize(writer);
            writer.SetPos(0);

            DataReader reader(bytes);
            ZDO zdo2;
            zdo2.Deserialize(reader);

            assert(zdo2.GetFloat("health", 0) == 3.1415926535f);
            assert(zdo2.GetInt("weight", 0) == 435);
            assert(zdo2.GetInt("slot", 0) == 3);
            assert(zdo2.GetString("name", "") == "byeorgssen");
            assert(zdo2.GetString("faction", "") == "player");
            assert(zdo2.GetInt("uid", 0) == 189341389);
        }
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
        // brute force tested
        //VUtils::Math::BruteForcePerlinNoise(-1.1f, -1.1f, 0.597924829f);



        auto opt = VUtils::Resource::ReadFileLines("perlin_values.txt");

        assert(opt && "file not found");

        auto&& values = opt.value();

        // the first line is the seed
        int next = 0;
        for (float y = -1.1f; y < 1.1f; y += .3f)
        {
            for (float x = -1.1f; x < 1.1f; x += .1f) {
                // Negative floats being truncated cause the mismatch
                if (x >= 0 && y >= 0)
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
};
