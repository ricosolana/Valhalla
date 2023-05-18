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
	m_uid = reader.Read<OWNER_t>();
	m_worldGenVersion = worldVersion >= 26 ? reader.Read<int32_t>() : 0;
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
	});

	return bytes;
}



void World::WriteFileMeta(const std::filesystem::path& root) {
	std::filesystem::create_directories(root);

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

void World::WriteFileDB(const std::filesystem::path& root) {
	std::filesystem::create_directories(root);

	auto startTime(steady_clock::now());
	BYTES_t bytes = WorldManager()->SaveWorldDB();
	auto finishTime = (steady_clock::now());

	auto path(root / (m_name + ".db"));

	if (VUtils::Resource::WriteFile(path, bytes)) {
		LOG_INFO(LOGGER, "World save to {} took {}s", path.string(), duration_cast<milliseconds>(finishTime - startTime).count());
	}
	else {
		LOG_WARNING(LOGGER, "Failed to save world to {}", path.string());
	}
}

void World::LoadFileDB(const std::filesystem::path& root) {
	auto now(steady_clock::now());

	auto path(root / (m_name + ".db"));
	if (auto opt = VUtils::Resource::ReadFile<BYTES_t>(path)) {
		try {
			DataReader reader(opt.value());

			auto worldVersion = reader.Read<int32_t>();
			if (worldVersion != VConstants::WORLD) {
				LOG_WARNING(LOGGER, "Loading unsupported world version {}", worldVersion);
				if (!VH_SETTINGS.worldModern) {
					LOG_WARNING(LOGGER, "Legacy ZDOs enabled. Networked objects might not behave as expected");
				}
			}
			else {
				LOG_INFO(LOGGER, "Loading world version {}", worldVersion);
			}

			if (worldVersion >= 4)
				Valhalla()->m_worldTime = reader.Read<double>();

			//VLOG(1) << "World time: " << Valhalla()->m_worldTime;

			ZDOManager()->Load(reader, worldVersion);

			if (worldVersion >= 12)
				ZoneManager()->Load(reader, worldVersion);

			if (worldVersion >= 15)
				RandomEventManager()->Load(reader, worldVersion);

			LOG_INFO(LOGGER, "World loading took {}s", duration_cast<seconds>(steady_clock::now() - now).count());
		}
		catch (const std::runtime_error& e) {
			LOG_ERROR(LOGGER, "Failed to load world: {}", e.what());
		}
	}	
}

void World::CopyCompressDB(const std::filesystem::path& root) {
	auto path = root / (m_name + ".db");

	if (std::filesystem::exists(path)) {
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

void World::WriteFiles(const std::filesystem::path& root) {
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



std::filesystem::path IWorldManager::GetWorldsPath() const {
	return "./worlds";
}




bool IWorldManager::LoadWorldMeta(const std::filesystem::path& root) {
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
	RandomEventManager()->Save(writer);
	
	return bytes;
}



void IWorldManager::PostZoneInit() {
	LOG_INFO(LOGGER, "Initializing WorldManager");

	m_world = RetrieveWorld(VH_SETTINGS.worldName, VH_SETTINGS.worldSeed);

	m_world->LoadFileDB();
}
