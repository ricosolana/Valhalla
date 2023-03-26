#pragma once

#include <future>
#include <thread>

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
    std::jthread m_saveThread;

public:
    World* GetWorld();

    // Get root path of worlds
    //  threadsafe
    fs::path GetWorldsPath() const;
    // Get meta path of world
    //  threadsafe
    fs::path GetWorldMetaPath(const std::string& name) const;
    // Get db path of world
    //  threadsafe
    fs::path GetWorldDBPath(const std::string& name) const;

    std::unique_ptr<World> GetWorld(const std::string& name) const;

    BYTES_t SaveWorldDB() const;
    void LoadFileWorldDB(const std::string& name) const;

    // Create a copy of a world by name
    //  threadsafe
    void BackupFileWorldDB(const std::string& name) const;

    // Threadsafe
    void WriteFileWorldDB(bool sync);

    void Init();
};

// Manager class for everything related to world file loading and file saving
IWorldManager* WorldManager();
