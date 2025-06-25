#pragma once

#include <functional>
#include <future>
#include <thread>

#include "DataStream.h"
#include "Types.h"
#include "VUtils.h"

class IWorldManager;

class World
{
    friend class IWorldManager;

  public:
    std::string m_name;
    std::string m_seedName;
    avledet::util::Hash m_seed;
    std::int64_t m_uid;
    std::int32_t m_worldGenVersion;
    avledet::util::Set<std::string, ankerl::unordered_dense::string_hash, std::equal_to<>>
            m_startingGlobalKeys;

  public:
    World(std::string name, std::string seedName);
    World(DataReader reader);

  public:
    avledet::util::Bytes SaveMeta();
    //avledet::util::Bytes SaveDB();

    void WriteFileMeta(std::filesystem::path const &root);
    void WriteFileDB(std::filesystem::path const &root);
    void LoadFileDB(std::filesystem::path const &root);
    void CopyCompressDB(std::filesystem::path const &root);
    void WriteFiles(std::filesystem::path const &root);

    void WriteFileMeta();
    void WriteFileDB();
    void LoadFileDB();
    void WriteFiles();
};

class IWorldManager
{
  private:
    std::unique_ptr<World> m_world;
    //std::jthread m_saveThread;

  public:
    World *GetWorld();

    // Get root path of worlds
    //  threadsafe
    std::filesystem::path GetWorldsPath() const;
    // Get meta path of world
    //  threadsafe
    //std::filesystem::path GetWorldMetaPath(const std::string& name) const;
    // Get db path of world
    //  threadsafe
    //std::filesystem::path GetWorldDBPath(const std::string& name) const;

    bool LoadWorldMeta(std::filesystem::path const &root);

    std::unique_ptr<World> RetrieveWorld(std::string_view name, std::string_view fallbackSeedName) const;

    avledet::util::Bytes SaveWorldDB() const;
    //void LoadFileWorldDB(const std::filesystem::path& path) const;

    // Create a copy of a world by name
    //  threadsafe
    //void BackupFileWorldDB() const;

    // Write the db of the current world to disk
    //  The world is only saved
    //  Threadsafe
    //void WriteFileWorldDB(const std::filesystem::path& path, bool sync);

    // Write the db of the current world to disk
    //  This is the go-to method to save the world to disk
    //  The world is backed up then saved
    //  Threadsafe
    //void WriteFileWorldDB(bool sync);

    void PostZoneInit();

    void PostInit();
};

// Manager class for everything related to world file loading and file saving
IWorldManager *WorldManager();
