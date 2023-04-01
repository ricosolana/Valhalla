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
#include "EventManager.h"

auto WORLD_MANAGER(std::make_unique<IWorldManager>());
IWorldManager* WorldManager() {
	return WORLD_MANAGER.get();
}



World::World(const std::string& name, const std::string& seedName) {
	m_name = name;
	m_seedName = seedName;
	m_seed = VUtils::String::GetStableHashCode(seedName);
	m_uid = VUtils::Random::GenerateUID();
	m_worldGenVersion = VConstants::WORLDGEN;
}

World::World(DataReader reader) {
	reader = reader.SubRead();

	auto worldVersion = reader.Read<int32_t>();

	//if (worldVersion != VConstants::WORLD)
		//LOG(WARNING) << "Loading unsupported world meta version: " << worldVersion;

	m_name = reader.Read<std::string>();
	m_seedName = reader.Read<std::string>();
	//m_seed = reader.Read<int32_t>();
	reader.Read<HASH_t>();
	m_seed = VUtils::String::GetStableHashCode(m_seedName);
	m_uid = reader.Read<OWNER_t>();
	m_worldGenVersion = worldVersion >= 26 ? reader.Read<int32_t>() : 0;
}

BYTES_t World::SaveMeta() {
	BYTES_t bytes;
	DataWriter writer(bytes);
	writer.SubWrite([&]() {
		writer.Write(VConstants::WORLD);
		writer.Write(m_name);
		writer.Write(m_seedName);
		writer.Write(VUtils::String::GetStableHashCode(m_seedName));
		writer.Write(m_uid);
		writer.Write(m_worldGenVersion);
	});

	return bytes;
}

void World::WriteFileMeta() {
	// Create folders (throw otherwise)
	fs::create_directories(WorldManager()->GetWorldsPath());

	// Create .old (backup of original .fwl)
	//auto metaPath = GetWorldMetaPath(m_name);
	auto metaPath = WorldManager()->GetWorldMetaPath(m_name);

	// Backup file if exists (no throw)
	//std::error_code ec;
	//fs::rename(metaPath, metaPath.string() + ".old", ec);

	BYTES_t bytes = SaveMeta();

	// create fwl
	if (!VUtils::Resource::WriteFile(metaPath, bytes))
		LOG(ERROR) << "Failed to write world meta file";
}



World* IWorldManager::GetWorld() {
	return m_world.get();
}



fs::path IWorldManager::GetWorldsPath() const {
	return "./worlds";
}

fs::path IWorldManager::GetWorldMetaPath(const std::string& name) const {
	return GetWorldsPath() / (name + ".fwl");
}

fs::path IWorldManager::GetWorldDBPath(const std::string& name) const {
	return GetWorldsPath() / (name + ".db");
}



std::unique_ptr<World> IWorldManager::RetrieveWorld(const std::string& name, const std::string& fallbackSeedName) const {
	// load world from file

	auto opt = VUtils::Resource::ReadFile<BYTES_t>(GetWorldMetaPath(name));

	std::unique_ptr<World> world;

	if (opt) {
		try {
			world = std::make_unique<World>(DataReader(opt.value()));
		}
		catch (const std::runtime_error& e) {
			LOG(ERROR) << "Failed to load world meta: " << e.what();
		}
	}

	if (!world) {
		LOG(INFO) << "Creating a new world meta";
		world = std::make_unique<World>(name, fallbackSeedName);

		try {
			world->WriteFileMeta();
		}
		catch (const std::exception& e) {
			LOG(ERROR) << "Failed to write world meta: " << e.what();
		}
	}

	LOG(INFO) << "Loaded world meta with seed " 
		<< world->m_seedName << " (" << world->m_seed << ")";

	return world;
}



void IWorldManager::LoadFileWorldDB(const std::string &name) const {
	LOG(INFO) << "Loading world '" << name << "'";
	auto dbpath = GetWorldDBPath(name);

	auto&& opt = VUtils::Resource::ReadFile<BYTES_t>(dbpath);

	auto now(steady_clock::now());

	if (opt) {
		try {
			DataReader reader(opt.value());

			auto worldVersion = reader.Read<int32_t>();
			if (worldVersion != VConstants::WORLD) {
				LOG(WARNING) << "Loading unsupported world version " << worldVersion;
				if (!SERVER_SETTINGS.worldModern)
					LOG(WARNING) << "Legacy ZDOs enabled. Networked objects might not behave as expected";
			}
			else
				LOG(INFO) << "Loading world version " << worldVersion;

			if (worldVersion >= 4)
				Valhalla()->m_worldTime = reader.Read<double>();

			VLOG(1) << "World time: " << Valhalla()->m_worldTime;

			ZDOManager()->Load(reader, worldVersion);

			if (worldVersion >= 12)
				ZoneManager()->Load(reader, worldVersion);

			if (worldVersion >= 15)
				EventManager()->Load(reader, worldVersion);

		} catch (const std::runtime_error& e) {
			LOG(ERROR) << "Failed to load world: " << e.what();
		}
	}

	LOG(INFO) << "World loading took " << duration_cast<seconds>(steady_clock::now() - now).count() << "s";
}



void IWorldManager::BackupFileWorldDB(const std::string& name) const {
	auto path = GetWorldDBPath(name);

	if (fs::exists(path)) {
		if (auto oldSave = VUtils::Resource::ReadFile<BYTES_t>(path)) {
			auto compressed = ZStdCompressor().Compress(*oldSave);
			if (!compressed) {
				LOG(ERROR) << "Failed to compress world backup " << path;
				return;
			}

			auto now(std::to_string(steady_clock::now().time_since_epoch().count()));
			auto backup = path.string() + "-" + now + ".zstd";
			if (VUtils::Resource::WriteFile(backup, *compressed))
				LOG(INFO) << "Saved world backup as '" << backup << "'";
			else
				LOG(ERROR) << "Failed to save world backup to " << backup;
		}
		else {
			LOG(ERROR) << "Failed to load old world for backup";
		}
	}
}

BYTES_t IWorldManager::SaveWorldDB() const {
	BYTES_t bytes;
	DataWriter writer(bytes);
	
	writer.Write(VConstants::WORLD);
	writer.Write(Valhalla()->GetWorldTime());

	ZDOManager()->Save(writer);
	ZoneManager()->Save(writer);	
	EventManager()->Save(writer);
	
	return bytes;
}

void IWorldManager::WriteFileWorldDB(bool sync) {
	if (m_saveThread.joinable()) {
		LOG(WARNING) << "Save thread is still active, joining...";
		m_saveThread.join();
	}

	LOG(INFO) << "World saving";

	auto start(steady_clock::now());
	BYTES_t bytes = SaveWorldDB();
	auto now(steady_clock::now());

	LOG(INFO) << "World serialize took " << duration_cast<milliseconds>(now - start).count() << "ms";

	m_saveThread = std::jthread([](const std::string &name, const BYTES_t& bytes) {
		try {
			el::Helpers::setThreadName("save");

			auto path = WorldManager()->GetWorldDBPath(name);

			auto start(steady_clock::now());
			WorldManager()->BackupFileWorldDB(name);

			if (!VUtils::Resource::WriteFile(path, bytes))
				LOG(WARNING) << "Failed to save world";
			else
				LOG(INFO) << "World save took " << duration_cast<milliseconds>(steady_clock::now() - start).count() << "ms";
		}
		catch (const std::exception& e) {
			LOG(ERROR) << "Severe error while saving world: " << e.what();
		}
	}, m_world->m_name, bytes);

	if (sync && m_saveThread.joinable())
		m_saveThread.join();
}



void IWorldManager::Init() {
	LOG(INFO) << "Initializing WorldManager";

	m_world = RetrieveWorld(SERVER_SETTINGS.worldName, SERVER_SETTINGS.worldSeed);

	LoadFileWorldDB(m_world->m_name);
}
