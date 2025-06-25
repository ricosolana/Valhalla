#include <future>
#include <quill/std/FilesystemPath.h>
#include <string>

#include "CompileSettings.h"
#include "NetManager.h"
#include "RandomEventManager.h"
#include "ValhallaServer.h"
#include "VUtils.h"
#include "VUtilsRandom.h"
#include "VUtilsResource.h"
#include "VUtilsString.h"
#include "WorldManager.h"
#include "ZDOManager.h"
#include "ZoneManager.h"

auto WORLD_MANAGER = std::make_unique<IWorldManager>();

IWorldManager *WorldManager()
{
    return WORLD_MANAGER.get();
}

World::World(std::string name, std::string seedName)
{
    m_name            = std::move(name);
    m_seedName        = std::move(seedName);
    m_seed            = avledet::util::get_stable_hash(seedName);
    m_uid             = VUtils::Random::GenerateUID();
    m_worldGenVersion = VConstants::WORLDGEN;
}

World::World(DataReader reader)
{
    reader = DataReader(reader.read<std::vector<char>>());

    auto worldVersion = reader.read<std::int32_t>();

    if (worldVersion != VConstants::WORLD) {
        LOG_WARNING(AVL_LOGGER, "Loading unsupported world meta version: {}", worldVersion);
    }

    m_name     = reader.read<std::string>();
    m_seedName = reader.read<std::string>();
    //reader.read<avledet::util::Hash>();// seed
    m_seed            = reader.read<avledet::util::Hash>();//avledet::util::get_stable_hash(m_seedName);
    m_uid             = reader.read<std::int64_t>();
    m_worldGenVersion = worldVersion >= 26 ? reader.read<std::int32_t>() : 0;
    bool needsDB      = worldVersion >= 30 ? reader.read<bool>() : false;
    if (worldVersion >= 32) {
        m_startingGlobalKeys = reader.read<decltype(m_startingGlobalKeys)>();
    }
}

avledet::util::Bytes World::SaveMeta()
{
    DataWriter writer;
    //assert(false); //TODO
    writer.write([this](DataWriter &writer) {
        writer.write(VConstants::WORLD);
        writer.write(m_name);
        writer.write(m_seedName);
        writer.write(avledet::util::get_stable_hash(m_seedName));
        writer.write(m_uid);
        writer.write(m_worldGenVersion);
        writer.write(true);

        // TODO write starting keys
        writer.write(m_startingGlobalKeys);
    });

    return writer.get_buf();
}

/*
avledet::util::Bytes World::SaveDB() {
	avledet::util::Bytes bytes;
	DataWriter writer(bytes);

	writer.write(VConstants::WORLD);
	writer.write(Avledet()->GetWorldTime());

	ZDOManager()->Save(writer);
	ZoneManager()->Save(writer);
	EventManager()->Save(writer);

	return bytes;
}*/


void World::WriteFileMeta(std::filesystem::path const &root)
{
    std::filesystem::create_directories(root);

    avledet::util::Bytes bytes = SaveMeta();

    auto path(root / (m_name + ".fwl"));

    // create fwl
    if (VUtils::Resource::WriteFile(path, bytes)) {
        LOG_INFO(AVL_LOGGER, "Wrote world meta to {}", path.string());
    } else {
        LOG_ERROR(AVL_LOGGER, "Failed to write world meta to {}", path.string());
    }
}

void World::WriteFileDB(std::filesystem::path const &root)
{
    std::filesystem::create_directories(root);

    auto startTime(std::chrono::steady_clock::now());
    avledet::util::Bytes bytes = WorldManager()->SaveWorldDB();
    auto finishTime            = (std::chrono::steady_clock::now());

    auto path(root / (m_name + ".db"));

    if (VUtils::Resource::WriteFile(path, bytes)) {
        LOG_INFO(AVL_LOGGER, "World save {} took {}ms", path.string(),
                 std::chrono::duration_cast<std::chrono::milliseconds>(finishTime - startTime).count());
    } else {
        LOG_WARNING(AVL_LOGGER, "Failed to save world to {}", path.string());
    }
}

void World::LoadFileDB(std::filesystem::path const &root)
{
    auto now(std::chrono::steady_clock::now());

    auto path(root / (m_name + ".db"));
    if (auto opt = VUtils::Resource::ReadFile<avledet::util::Bytes>(path)) {
        try {
            DataReader reader(std::move(opt.value()));

            auto worldVersion = reader.read<std::int32_t>();
            if (worldVersion < VConstants::WORLD) {
#if AVL_IS_ON(AVL_LEGACY_WORLD_LOADING)
                LOG_WARNING(AVL_LOGGER, "Loading legacy world with version {}", worldVersion);
#else // !AVL_LEGACY_WORLD_LOADING
                LOG_ERROR(AVL_LOGGER, "Requires AVL_LEGACY_WORLD_COMPATIBILITY to load legacy worlds");
                throw std::runtime_error("legacy world loading unsupported with current compile settings");
#endif// AVL_LEGACY_WORLD_LOADING
            } else if (worldVersion > VConstants::WORLD) {
                LOG_WARNING(AVL_LOGGER, "Loading world with a newer version than we support {}",
                            worldVersion);
            } else {
                LOG_NOTICE(AVL_LOGGER, "Loading world version {}", worldVersion);
            }

#if AVL_IS_ON(AVL_LEGACY_WORLD_LOADING)
            if (worldVersion >= 4)
#endif// AVL_LEGACY_WORLD_LOADING
            {
                Avledet()->m_worldTime = reader.read<double>();
            }

            ZDOManager()->Load(reader, worldVersion);

#if AVL_IS_ON(AVL_LEGACY_WORLD_LOADING)
            if (worldVersion >= 12)
#endif// AVL_LEGACY_WORLD_LOADING
            {
                ZoneManager()->Load(reader, worldVersion);
            }

#if AVL_IS_ON(AVL_RANDOM_EVENTS)
    #if AVL_IS_ON(AVL_LEGACY_WORLD_LOADING)
            if (worldVersion >= 15)
    #endif// AVL_LEGACY_WORLD_LOADING
            {
                RandomEventManager()->Load(reader, worldVersion);
            }
#endif// AVL_RANDOM_EVENTS
            LOG_INFO(AVL_LOGGER, "World loading took {}s",
                     std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - now)
                             .count());
        } catch (std::runtime_error const &e) {
            LOG_ERROR(AVL_LOGGER, "Failed to load world: {}", e.what());
        }
    }
}

void World::CopyCompressDB(std::filesystem::path const &root)
{
    auto path = root / (m_name + ".db");

    if (std::filesystem::exists(path)) {
        if (auto oldSave = VUtils::Resource::ReadFile<avledet::util::Bytes>(path)) {
            auto compressed = ZStdCompressor().Compress(*oldSave);
            if (!compressed) {
                LOG_ERROR(AVL_LOGGER, "Failed to compress world backup {}", path.string());
                return;
            }

            auto now(std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
            auto backup = path.string() + "-" + now + ".zstd";
            if (VUtils::Resource::WriteFile(backup, *compressed)) {
                LOG_INFO(AVL_LOGGER, "Saved world backup as '{}'", backup);
            } else {
                LOG_ERROR(AVL_LOGGER, "Failed to save world backup to {}", backup);
            }
        } else {
            LOG_ERROR(AVL_LOGGER, "Failed to load old world for backup");
        }
    }
}

void World::WriteFiles(std::filesystem::path const &root)
{
    WriteFileMeta(root);
    WriteFileDB(root);
    CopyCompressDB(root);
}

void World::WriteFileMeta()
{
    WriteFileMeta(WorldManager()->GetWorldsPath());
}

void World::WriteFileDB()
{
    WriteFileDB(WorldManager()->GetWorldsPath());
}

void World::LoadFileDB()
{
    LoadFileDB(WorldManager()->GetWorldsPath());
}

void World::WriteFiles()
{
    WriteFiles(WorldManager()->GetWorldsPath());
}

World *IWorldManager::GetWorld()
{
    return m_world.get();
}

std::filesystem::path IWorldManager::GetWorldsPath() const
{
    return "./worlds";
}

/*
std::filesystem::path IWorldManager::GetWorldMetaPath(const std::string& name) const {
	return GetWorldsPath() / (name + ".fwl");
}

std::filesystem::path IWorldManager::GetWorldDBPath(const std::string& name) const {
	return GetWorldsPath() / (name + ".db");
}*/


bool IWorldManager::LoadWorldMeta(std::filesystem::path const &root)
{
    if (auto opt
        = VUtils::Resource::ReadFile<avledet::util::Bytes>(root / (AVL_SETTINGS.worldName + ".fwl"))) {
        try {
            this->m_world = std::make_unique<World>(DataReader(*opt));
        } catch (std::runtime_error const &e) {
            LOG_ERROR(AVL_LOGGER, "Failed to load world meta: {}", e.what());
        }
    }

    return m_world.get();
}

std::unique_ptr<World> IWorldManager::RetrieveWorld(std::string_view name,
                                                    std::string_view fallbackSeedName) const
{
    // load world from file

    LOG_NOTICE(AVL_LOGGER, "Locating world meta \'{}\'", name);

    std::unique_ptr<World> world;

    if (auto opt
        = VUtils::Resource::ReadFile<avledet::util::Bytes>(GetWorldsPath() / (std::string(name) + ".fwl"))) {
        try {
            world = std::make_unique<World>(DataReader(*opt));
        } catch (std::runtime_error const &e) {
            LOG_ERROR(AVL_LOGGER, "Failed to load world meta: {}", e.what());
        }
    }

    if (!world) {
        LOG_INFO(AVL_LOGGER, "World not found, creating new world meta");
        world = std::make_unique<World>(std::string(name), std::string(fallbackSeedName));

        try {
            world->WriteFileMeta();
        } catch (std::exception const &e) {
            LOG_ERROR(AVL_LOGGER, "Failed to write world meta: {}", e.what());
        }
    }

    LOG_NOTICE(AVL_LOGGER, "Loaded world meta with seed {} ({})", world->m_seedName, world->m_seed);

    return world;
}

avledet::util::Bytes IWorldManager::SaveWorldDB() const
{
    DataWriter writer;

    writer.write(VConstants::WORLD);
    writer.write(Avledet()->GetWorldTime());

    ZDOManager()->Save(writer);
    ZoneManager()->Save(writer);
    // This omission only works because random events happen to be saved/loaded last
#if AVL_IS_ON(AVL_RANDOM_EVENTS)
    RandomEventManager()->Save(writer);
#else
    writer.write(0.f);
    writer.write("");
    writer.write(0.f);
    writer.write(Vector3f::ZERO);
#endif

    return writer.get_buf();
}

/*
void IWorldManager::WriteFileWorldDB(const std::filesystem::path& path, bool sync) const {
	if (m_saveThread.joinable()) {
		//LOG(WARNING) << "Save thread is still active, joining...";
		m_saveThread.join();
	}

	//LOG(INFO) << "World saving";

	auto start(steady_clock::now());
	avledet::util::Bytes bytes = SaveWorldDB();
	auto now(steady_clock::now());

	//LOG(INFO) << "World serialize took " << duration_cast<milliseconds>(now - start);

	m_saveThread = std::jthread([path](avledet::util::Bytes bytes) {
		try {
			el::Helpers::setThreadName("save");

			auto start(steady_clock::now());

			if (VUtils::Resource::WriteFile(path, bytes))
				//LOG(INFO) << "World save to " << path.c_str() << " in " << duration_cast<milliseconds>(steady_clock::now() - start);
			else
				//LOG(WARNING) << "Failed to save world to " << path.c_str();
		}
		catch (const std::exception& e) {
			//LOG(ERROR) << "Severe error while saving world: " << e.what();
		}
	}, std::move(bytes));

	if (sync && m_saveThread.joinable())
		m_saveThread.join();
}*/

/*
void IWorldManager::WriteFileWorldDB(bool sync) {
	WriteFileWorldDB(WorldManager()->GetWorldDBPath(m_world->m_name), sync);
}*/

/*
void IWorldManager::WriteWorldFiles(const std::filesystem::path& root) {
	m_world->WriteFileMeta(root);
	m_world->WriteFileDB(root);
}*/


void IWorldManager::PostZoneInit()
{
    LOG_NOTICE(AVL_LOGGER, "Initializing WorldManager");

    m_world = RetrieveWorld(AVL_SETTINGS.worldName, AVL_SETTINGS.worldSeed);

#ifdef AVL_OPTION_ENABLE_CAPTURE
    if (AVL_SETTINGS.packetMode == PacketMode::PLAYBACK) {
        std::filesystem::path root = std::filesystem::path(AVL_CAPTURE_PATH) / m_world->m_name
                                     / std::to_string(AVL_SETTINGS.packetPlaybackSessionIndex);

        if (LoadWorldMeta(root))
            m_world->LoadFileDB(root);
        else
            LOG_FATAL(AVL_LOGGER, "Failed to load world for playback");
    } else
#endif//AVL_OPTION_ENABLE_CAPTURE
    {
        m_world->LoadFileDB();
    }
}

void IWorldManager::PostInit()
{
#ifdef AVL_OPTION_ENABLE_CAPTURE
    if (AVL_SETTINGS.packetMode == PacketMode::CAPTURE) {
        // then save world as a copy to captures
        auto world(WorldManager()->GetWorld());
        std::filesystem::path root = std::filesystem::path(AVL_CAPTURE_PATH) / world->m_name
                                     / std::to_string(AVL_SETTINGS.packetCaptureSessionIndex);

        std::filesystem::create_directories(root);

        // save world
        world->WriteFiles(root);
    }
#endif//AVL_OPTION_ENABLE_CAPTURE
}
