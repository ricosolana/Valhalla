#include <future>

#include "WorldManager.h"
#include "VUtils.h"
#include "VUtilsResource.h"
#include "NetPackage.h"
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



World* IWorldManager::GetWorld() {
	return m_world.get();
}

fs::path IWorldManager::GetWorldsPath() {
	return "./worlds";
}

fs::path IWorldManager::GetWorldMetaPath(const std::string& name) {
	return GetWorldsPath() / (name + ".fwl");
}

fs::path IWorldManager::GetWorldDBPath(const std::string& name) {
	return GetWorldsPath() / (name + ".db");
}

//void SaveWorldMetaData(DateTime backupTimestamp) {
//	bool flag = false;
//	FileWriter fileWriter;
//	SaveWorldMetaData(backupTimestamp, out flag, out fileWriter);
//}

void IWorldManager::SaveWorldMeta(World* world) {
	//auto dbpath = GetWorldDBPath(world->m_name);

	NetPackage pkg;

	pkg.Write(VConstants::WORLD);
	pkg.Write(world->m_name);
	pkg.Write(world->m_seedName);
	pkg.Write(world->m_seed);
	pkg.Write(world->m_uid);
	pkg.Write(world->m_worldGenVersion);

	fs::create_directories(GetWorldsPath());

	// Create .old (backup of original .fwl)
	auto metaPath = GetWorldMetaPath(world->m_name);

	std::error_code ec;
	fs::rename(metaPath, metaPath.string() + ".old", ec);

	NetPackage fileWriter;
	fileWriter.Write(pkg);

	// create fwl
	VUtils::Resource::WriteFileBytes(metaPath, fileWriter.m_stream.m_buf);

	//ZNet.ConsiderAutoBackup(metaPath, this.m_fileSource, now);

	//metaWriter.Write(writer);
}

void IWorldManager::LoadWorldDB(const std::string &name) {
	LOG(INFO) << "Loading world: " << name;
	auto dbpath = GetWorldDBPath(name);

	auto&& opt = VUtils::Resource::ReadFileBytes(dbpath);

	if (!opt) {
		//throw std::runtime_error("world db missing");
		LOG(INFO) << "missing " << dbpath;
	}
	else {
		try {
			NetPackage pkg(opt.value());

			auto worldVersion = pkg.Read<int32_t>();
			if (worldVersion != VConstants::WORLD) {
				LOG(ERROR) << "Incompatible data version " << worldVersion;
			}
			else {
				if (worldVersion >= 4)
					Valhalla()->m_netTime = pkg.Read<double>();

				ZDOManager()->Load(pkg, worldVersion);

				if (worldVersion >= 12)
					ZoneManager()->Load(pkg, worldVersion);

				if (worldVersion >= 15) {
					// inlined RandEventManager::Load
					//RandEventManager::Load(pkg, worldVersion);

					auto eventTimer = pkg.Read<float>();
					if (worldVersion >= 25) {
						auto text = pkg.Read<std::string>();
						auto time = pkg.Read<float>();
						Vector3 pos = pkg.Read<Vector3>();

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
			}
		}
		catch (const std::exception& e) {
			LOG(ERROR) << "Unable to load world: " << e.what();
		}
	}
}



NetPackage IWorldManager::SaveWorldDB() {
	NetPackage binary;

	assert(false);
	binary.Write(VConstants::WORLD);
	binary.Write(Valhalla()->NetTime());
	ZDOManager()->Save(binary);
	ZoneManager()->Save(binary);

	// Temporary inlined RandEventSystem::SaveAsync:
	//RandEventSystem::SaveAsync(binary);
	binary.Write((float)0);
	binary.Write("");
	binary.Write((float)0);
	binary.Write(Vector3());
}

void IWorldManager::SaveWorld(const std::string& name, bool sync) {
	auto now(steady_clock::now());

	LOG(INFO) << "World saving";

	auto path = GetWorldDBPath(m_world->m_name);

	static NetPackage binary;
	binary = SaveWorldDB();

	LOG(INFO) << "World serialize took " << duration_cast<milliseconds>(steady_clock::now() - now).count() << "ms";

	m_saveThread = std::thread([path]() {
		el::Helpers::setThreadName("save");

		auto now(steady_clock::now());

		// backup the old file
		if (fs::exists(path)) {
			if (auto oldSave = VUtils::Resource::ReadFileBytes(path)) {
				auto compressed = VUtils::CompressGz(*oldSave);
				if (!compressed) {
					LOG(ERROR) << "Failed to compress world backup " << path;
					return;
				}

				auto ms = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
				auto backup = path.string() + std::to_string(ms) + ".gz";
				if (VUtils::Resource::WriteFileBytes(backup, *compressed))
					LOG(INFO) << "Saved world backup as '" << backup << "'";
				else
					LOG(ERROR) << "Failed to save world backup to " << backup;
			}
			else {
				LOG(ERROR) << "Failed to load old world for backup";
			}
		}

		if (!VUtils::Resource::WriteFileBytes(path, binary.m_stream.m_buf))
			LOG(ERROR) << "Failed to save world";
		else
			LOG(INFO) << "World save took " << duration_cast<milliseconds>(steady_clock::now() - now).count() << "ms";
	});

	if (sync)
		m_saveThread.join();
}



std::unique_ptr<World> IWorldManager::GetOrCreateWorldMeta(const std::string& name) {
	// load world from file

	auto opt = VUtils::Resource::ReadFileBytes(GetWorldMetaPath(name));

	auto world(std::make_unique<World>());

	bool success = false;

	if (opt) {
		NetPackage binary(opt.value());
		auto zpackage = binary.Read<NetPackage>();

		auto worldVersion = zpackage.Read<int32_t>();
		if (worldVersion != VConstants::WORLD) {
			LOG(ERROR) << "Incompatible world version " << worldVersion;
		}
		else {
			try {
				world->m_name = zpackage.Read<std::string>();
				world->m_seedName = zpackage.Read<std::string>();
				world->m_seed = zpackage.Read<int32_t>();
				world->m_uid = zpackage.Read<OWNER_t>();
				world->m_worldGenVersion = worldVersion >= 26 ? zpackage.Read<int32_t>() : 0;
				LOG(INFO) << "Loaded world '" << world->m_name << "' with seed " <<
					world->m_seed << " (" << world->m_seedName << ")";
				success = true;
			}
			catch (const std::range_error& e) {
				LOG(ERROR) << "World is corrupted or unsupported " << name;
			}
		}
	}
	else {
		LOG(INFO) << "Creating a new world";
	}

	// set default world
	if (!success) {
		std::string seedName = VUtils::Random::GenerateAlphaNum(10);

		world->m_name = name;
		world->m_seedName = seedName;
		world->m_seed = VUtils::String::GetStableHashCode(seedName);
		world->m_uid = VUtils::Random::GenerateUID();
		world->m_worldGenVersion = VConstants::WORLDGEN;

		SaveWorldMeta(world.get());
	}

	return world;
}

void IWorldManager::Init() {
	LOG(INFO) << "Initializing WorldManager";

	m_world = GetOrCreateWorldMeta(SERVER_SETTINGS.worldName);
}
