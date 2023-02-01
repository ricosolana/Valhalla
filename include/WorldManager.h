#pragma once

#include "VUtils.h"

class NetPackage;

class World {
public:
    std::string m_name;
    std::string m_seedName;
    int32_t m_seed;
    OWNER_t m_uid;
    int32_t m_worldGenVersion;
};

class IWorldManager {
private:
    std::unique_ptr<World> m_world;
    std::thread m_saveThread;

public:
    World* GetWorld();

    fs::path GetWorldsPath();

    fs::path GetWorldMetaPath(const std::string& name);

    fs::path GetWorldDBPath(const std::string& name);

    void SaveWorldMeta(World* world);

    void LoadWorldDB(const std::string& name);

    NetPackage SaveWorldDB();

    void SaveWorld(bool sync);

    std::unique_ptr<World> GetOrCreateWorldMeta(const std::string& name);

    void Init();
};

IWorldManager* WorldManager();
