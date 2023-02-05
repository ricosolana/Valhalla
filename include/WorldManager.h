#pragma once

#include "VUtils.h"
#include "DataReader.h"
#include "DataWriter.h"

class IWorldManager;

class World {
    friend class IWorldManager;

public:
    std::string m_name;
    std::string m_seedName;
    HASH_t m_seed;
    OWNER_t m_uid;
    int32_t m_worldGenVersion;

public:
    World(const std::string& name);
    World(DataReader reader);

public:
    BYTES_t SaveMeta();
    void WriteFileMeta();
};

class IWorldManager {
private:
    std::unique_ptr<World> m_world;
    std::thread m_saveThread;

public:
    World* GetWorld();

    fs::path GetWorldsPath() const;
    fs::path GetWorldMetaPath(const std::string& name) const;
    fs::path GetWorldDBPath(const std::string& name) const;

    std::unique_ptr<World> GetWorld(const std::string& name) const;

    BYTES_t SaveWorldDB() const;
    void LoadFileWorldDB(const std::string& name) const;
    void BackupFileWorldDB(const std::string& name) const;

    void WriteFileWorldDB(bool sync);

    void Init();
};

IWorldManager* WorldManager();
