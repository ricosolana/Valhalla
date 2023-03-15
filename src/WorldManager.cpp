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



World::World(const std::string& name) {
	std::string seedName = VUtils::Random::GenerateAlphaNum(10);

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
	writer.SubWrite([&writer, this]() {
		writer.Write(VConstants::WORLD);
		writer.Write(m_name);
		writer.Write(m_seedName);
		//writer.Write(m_seed);
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



std::unique_ptr<World> IWorldManager::GetWorld(const std::string& name) const {
	// load world from file

	auto opt = VUtils::Resource::ReadFile(GetWorldMetaPath(name));

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
		world = std::make_unique<World>(name);

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

	auto&& opt = VUtils::Resource::ReadFile(dbpath);

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



			ZDOManager()->Load(reader, worldVersion);

			if (worldVersion >= 12)
				ZoneManager()->Load(reader, worldVersion);

			if (worldVersion >= 15) {
				// inlined RandEventManager::Load
				//RandEventManager::Load(pkg, worldVersion);

				auto eventTimer = reader.Read<float>();
				if (worldVersion >= 25) {
					auto text = reader.Read<std::string>();
					auto time = reader.Read<float>();
					Vector3 pos = reader.Read<Vector3>();

					//if (!text.empty()) {
					//	this.SetRandomEventByName(text, pos);
					//	if (this.m_randomEvent != null)
					//	{
					//		this.m_randomEvent.m_time = time;
					//		this.m_randomEvent.m_pos = pos;
					//	}
					//}
				}
			}
		} catch (const std::runtime_error& e) {
			LOG(ERROR) << "Failed to load world: " << e.what();
		}
	}

	LOG(INFO) << "World loading took " << duration_cast<seconds>(steady_clock::now() - now).count() << "s";
}



void IWorldManager::BackupFileWorldDB(const std::string& name) const {
	auto path = GetWorldDBPath(name);

	if (fs::exists(path)) {
		if (auto oldSave = VUtils::Resource::ReadFile(path)) {
			auto compressed = VUtils::CompressGz(*oldSave);
			if (!compressed) {
				LOG(ERROR) << "Failed to compress world backup " << path;
				return;
			}

			auto ms = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
			auto backup = path.string() + std::to_string(ms) + ".gz";
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
	
	// Temporary inlined RandEventSystem::SaveAsync:
	//RandEventSystem::SaveAsync(binary);
	writer.Write((float)0);
	writer.Write("");
	writer.Write((float)0);
	writer.Write(Vector3());
	
	return bytes;
}

void IWorldManager::WriteFileWorldDB(bool sync) {
	if (m_saveThread.joinable()) {
		LOG(WARNING) << "Save thread is still active, joining...";
		m_saveThread.join();
	}

	auto now(steady_clock::now());

	LOG(INFO) << "World saving";

	auto path = GetWorldDBPath(m_world->m_name);

	static BYTES_t bytes;
	bytes = SaveWorldDB();

	LOG(INFO) << "World serialize took " << duration_cast<milliseconds>(steady_clock::now() - now).count() << "ms";

	m_saveThread = std::thread([path]() {
		try {
			el::Helpers::setThreadName("save");

			auto now(steady_clock::now());

			// backup the old file
			if (fs::exists(path)) {
				if (auto oldSave = VUtils::Resource::ReadFile(path)) {
					auto compressed = VUtils::CompressGz(*oldSave);
					if (!compressed) {
						LOG(WARNING) << "Failed to compress world backup " << path;
						return;
					}

					auto ms = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
					auto backup = path.string().substr(0, path.string().length() - 3) + "-" + std::to_string(ms) + ".db.gz";
					if (VUtils::Resource::WriteFile(backup, *compressed))
						LOG(INFO) << "Saved world backup as '" << backup << "'";
					else
						LOG(WARNING) << "Failed to save world backup to " << backup;
				}
				else {
					LOG(WARNING) << "Failed to load old world for backup";
				}
			}

			if (!VUtils::Resource::WriteFile(path, bytes))
				LOG(WARNING) << "Failed to save world";
			else
				LOG(INFO) << "World save took " << duration_cast<milliseconds>(steady_clock::now() - now).count() << "ms";
		}
		catch (const std::exception& e) {
			LOG(ERROR) << "Severe error while saving world: " << e.what();
		}
	});

	if (sync)
		m_saveThread.join();
}





void IWorldManager::Init() {
	LOG(INFO) << "Initializing WorldManager";

	m_world = GetWorld(SERVER_SETTINGS.worldName);

	LoadFileWorldDB(m_world->m_name);
}
