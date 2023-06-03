#include <future>

#include "WorldManager.h"
#include "VUtils.h"
#include "VUtilsResource.h"
#include "ValhallaServer.h"
#include "VUtilsRandom.h"
#include "VUtilsString.h"
#include "ZDOManager.h"
#include "ZoneManager.h"
#include "NetManager.h"
#include "RandomEventManager.h"

auto WORLD_MANAGER(std::make_unique<IWorldManager>());
IWorldManager* WorldManager() {
	return WORLD_MANAGER.get();
}



World::World(std::string name, std::string seedName) {
	m_name = std::move(name);
	m_seedName = std::move(seedName);
	m_seed = VUtils::String::GetStableHashCode(seedName);
	m_uid = VUtils::Random::GenerateUID();
	m_worldGenVersion = VConstants::WORLDGEN;
}

World::World(DataReader reader) {
	reader = reader.Read<DataReader>();

	auto worldVersion = reader.Read<int32_t>();

	if (worldVersion != VConstants::WORLD) {
		LOG_WARNING(LOGGER, "Loading unsupported world meta version: {}", worldVersion);
	}

	m_name = reader.Read<std::string>();
	m_seedName = reader.Read<std::string>();
	reader.Read<HASH_t>(); // seed
	m_seed = VUtils::String::GetStableHashCode(m_seedName);
	m_uid = reader.Read<int64_t>();
	m_worldGenVersion = worldVersion >= 26 ? reader.Read<int32_t>() : 0;
	bool needsDB = worldVersion >= 30 ? reader.Read<bool>() : false;
}



BYTES_t World::SaveMeta() {
	BYTES_t bytes;
	DataWriter writer(bytes);
	writer.SubWrite([this](DataWriter& writer) {
		writer.Write(VConstants::WORLD);
		writer.Write(std::string_view(m_name));
		writer.Write(std::string_view(m_seedName));
		writer.Write(VUtils::String::GetStableHashCode(m_seedName));
		writer.Write(m_uid);
		writer.Write(m_worldGenVersion);
		writer.Write(true);
	});

	return bytes;
}

/*
BYTES_t World::SaveDB() {
	BYTES_t bytes;
	DataWriter writer(bytes);

	writer.Write(VConstants::WORLD);
	writer.Write(Valhalla()->GetWorldTime());

	ZDOManager()->Save(writer);
	ZoneManager()->Save(writer);
	EventManager()->Save(writer);

	return bytes;
}*/



void World::WriteFileMeta(const fs::path& root) {
	fs::create_directories(root);

	BYTES_t bytes = SaveMeta();

	auto path(root / (m_name + ".fwl"));

	// create fwl
	if (VUtils::Resource::WriteFile(path, bytes)) {
		LOG_INFO(LOGGER, "Wrote world meta to {}", path.string());
	}
	else {
		LOG_ERROR(LOGGER, "Failed to write world meta to {}", path.string());
	}
}

void World::WriteFileDB(const fs::path& root) {
	fs::create_directories(root);

	auto startTime(steady_clock::now());
	BYTES_t bytes = WorldManager()->SaveWorldDB();
	auto finishTime = (steady_clock::now());

	auto path(root / (m_name + ".db"));

	if (VUtils::Resource::WriteFile(path, bytes)) {
		LOG_INFO(LOGGER, "World save {} took {}ms", path.string(), duration_cast<milliseconds>(finishTime - startTime).count());
	}
	else {
		LOG_WARNING(LOGGER, "Failed to save world to {}", path.string());
	}
}

void World::LoadFileDB(const fs::path& root) {
	auto now(steady_clock::now());

	auto path(root / (m_name + ".db"));
	if (auto opt = VUtils::Resource::ReadFile<BYTES_t>(path)) {
		try {
			DataReader reader(opt.value());

			auto worldVersion = reader.Read<int32_t>();
			if (worldVersion != VConstants::WORLD) {
#if VH_IS_ON(VH_LEGACY_WORLD_COMPATABILITY)
				LOG_WARNING(LOGGER, "Loading legacy world with version {}", worldVersion);
#else // !VH_LEGACY_WORLD_COMPATABILITY
				//LOG_ERROR(LOGGER, "Requires VH_LEGACY_WORLD_COMPATIBILITY to loaded legacy worlds");
				throw std::runtime_error("legacy world loading unsupported with current compile settings");
#endif // VH_LEGACY_WORLD_COMPATABILITY
			}
			else {
				LOG_INFO(LOGGER, "Loading world version {}", worldVersion);
			}

#if VH_IS_ON(VH_LEGACY_WORLD_COMPATABILITY)
			if (worldVersion >= 4)
#endif // VH_LEGACY_WORLD_COMPATABILITY
			{
				Valhalla()->m_worldTime = reader.Read<double>();
			}
			static constexpr auto szz = sizeof(IZDOManager);
			//VLOG(1) << "World time: " << Valhalla()->m_worldTime;

			ZDOManager()->Load(reader, worldVersion);

#if VH_IS_ON(VH_LEGACY_WORLD_COMPATABILITY)
			if (worldVersion >= 12)
#endif // VH_LEGACY_WORLD_COMPATABILITY
			{
				ZoneManager()->Load(reader, worldVersion);
			}

#if VH_IS_ON(VH_RANDOM_EVENTS)
#if VH_IS_ON(VH_LEGACY_WORLD_COMPATABILITY)
			if (worldVersion >= 15)
#endif // VH_LEGACY_WORLD_COMPATABILITY
			{
				RandomEventManager()->Load(reader, worldVersion);
			}
#endif // VH_RANDOM_EVENTS
			LOG_INFO(LOGGER, "World loading took {}s", duration_cast<seconds>(steady_clock::now() - now).count());
		}
		catch (const std::runtime_error& e) {
			LOG_ERROR(LOGGER, "Failed to load world: {}", e.what());
		}
	}	
}

void World::CopyCompressDB(const fs::path& root) {
	auto path = root / (m_name + ".db");

	if (fs::exists(path)) {
		if (auto oldSave = VUtils::Resource::ReadFile<BYTES_t>(path)) {
			auto compressed = ZStdCompressor().Compress(*oldSave);
			if (!compressed) {
				LOG_ERROR(LOGGER, "Failed to compress world backup {}", path);
				return;
			}

			auto now(std::to_string(steady_clock::now().time_since_epoch().count()));
			auto backup = path.string() + "-" + now + ".zstd";
			if (VUtils::Resource::WriteFile(backup, *compressed)) {
				LOG_INFO(LOGGER, "Saved world backup as '{}'", backup);
			}
			else {
				LOG_ERROR(LOGGER, "Failed to save world backup to {}", backup);
			}
		}
		else {
			LOG_ERROR(LOGGER, "Failed to load old world for backup");
		}
	}
}

void World::WriteFiles(const fs::path& root) {
	WriteFileMeta(root);
	WriteFileDB(root);
	CopyCompressDB(root);
}



void World::WriteFileMeta() {
	WriteFileMeta(WorldManager()->GetWorldsPath());
}

void World::WriteFileDB() {
	WriteFileDB(WorldManager()->GetWorldsPath());
}

void World::LoadFileDB() {
	LoadFileDB(WorldManager()->GetWorldsPath());
}

void World::WriteFiles() {
	WriteFiles(WorldManager()->GetWorldsPath());
}



World* IWorldManager::GetWorld() {
	return m_world.get();
}



fs::path IWorldManager::GetWorldsPath() const {
	return "./worlds";
}

/*
fs::path IWorldManager::GetWorldMetaPath(const std::string& name) const {
	return GetWorldsPath() / (name + ".fwl");
}

fs::path IWorldManager::GetWorldDBPath(const std::string& name) const {
	return GetWorldsPath() / (name + ".db");
}*/



bool IWorldManager::LoadWorldMeta(const fs::path& root) {
	if (auto opt = VUtils::Resource::ReadFile<BYTES_t>(root / (VH_SETTINGS.worldName + ".fwl"))) {
		try {
			this->m_world = std::make_unique<World>(DataReader(*opt));
		}
		catch (const std::runtime_error& e) {
			LOG_ERROR(LOGGER, "Failed to load world meta: {}", e.what());
		}
	}
		
	return m_world.get();
}

std::unique_ptr<World> IWorldManager::RetrieveWorld(std::string_view name, std::string_view fallbackSeedName) const {
	// load world from file

	std::unique_ptr<World> world;

	if (auto opt = VUtils::Resource::ReadFile<BYTES_t>(GetWorldsPath() / (std::string(name) + ".fwl"))) {
		try {
			world = std::make_unique<World>(DataReader(*opt));
		}
		catch (const std::runtime_error& e) {
			LOG_ERROR(LOGGER, "Failed to load world meta: {}", e.what());
		}
	}

	if (!world) {
		LOG_INFO(LOGGER, "Creating a new world meta");
		world = std::make_unique<World>(std::string(name), std::string(fallbackSeedName));

		try {
			world->WriteFileMeta();
		}
		catch (const std::exception& e) {
			LOG_ERROR(LOGGER, "Failed to write world meta: {}", e.what());
		}
	}

	LOG_INFO(LOGGER, "Loaded world meta with seed {} ({})", world->m_seedName, world->m_seed);

	return world;
}

BYTES_t IWorldManager::SaveWorldDB() const {
	BYTES_t bytes;
	DataWriter writer(bytes);
	
	writer.Write(VConstants::WORLD);
	writer.Write(Valhalla()->GetWorldTime());

	ZDOManager()->Save(writer);
	ZoneManager()->Save(writer);
	// This omission only works because random events happen to be saved/loaded last
#if VH_IS_ON(VH_RANDOM_EVENTS)
	RandomEventManager()->Save(writer);
#else
	writer.Write(0.f);
	writer.Write("");
	writer.Write(0.f);
	writer.Write(Vector3f::Zero());
#endif

	return bytes;
}

/*
void IWorldManager::WriteFileWorldDB(const fs::path& path, bool sync) const {
	if (m_saveThread.joinable()) {
		//LOG(WARNING) << "Save thread is still active, joining...";
		m_saveThread.join();
	}

	//LOG(INFO) << "World saving";

	auto start(steady_clock::now());
	BYTES_t bytes = SaveWorldDB();
	auto now(steady_clock::now());

	//LOG(INFO) << "World serialize took " << duration_cast<milliseconds>(now - start);

	m_saveThread = std::jthread([path](BYTES_t bytes) {
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
void IWorldManager::WriteWorldFiles(const fs::path& root) {
	m_world->WriteFileMeta(root);
	m_world->WriteFileDB(root);
}*/



void IWorldManager::PostZoneInit() {
	LOG_INFO(LOGGER, "Initializing WorldManager");

	m_world = RetrieveWorld(VH_SETTINGS.worldName, VH_SETTINGS.worldSeed);

#ifdef VH_OPTION_ENABLE_CAPTURE
	if (VH_SETTINGS.packetMode == PacketMode::PLAYBACK) {
		fs::path root = fs::path(VH_CAPTURE_PATH) 
			/ m_world->m_name
			/ std::to_string(VH_SETTINGS.packetPlaybackSessionIndex);

		if (LoadWorldMeta(root))
			m_world->LoadFileDB(root);
		else
			LOG_FATAL(LOGGER, "Failed to load world for playback");
	}
	else
#endif //VH_OPTION_ENABLE_CAPTURE
	{
		m_world->LoadFileDB();
	}
}

void IWorldManager::PostInit() {
#ifdef VH_OPTION_ENABLE_CAPTURE
	if (VH_SETTINGS.packetMode == PacketMode::CAPTURE) {
		// then save world as a copy to captures
		auto world(WorldManager()->GetWorld());
		fs::path root = fs::path(VH_CAPTURE_PATH) 
			/ world->m_name 
			/ std::to_string(VH_SETTINGS.packetCaptureSessionIndex);

		fs::create_directories(root);

		// save world
		world->WriteFiles(root);
	}
#endif //VH_OPTION_ENABLE_CAPTURE
}
