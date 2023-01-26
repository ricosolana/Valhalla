#include "WorldManager.h"
#include "VUtils.h"
#include "VUtilsResource.h"
#include "NetPackage.h"
#include "ValhallaServer.h"
#include "VUtilsRandom.h"
#include "VUtilsString.h"
#include "ZDOManager.h"
#include "ZoneManager.h"

auto WORLD_MANAGER(std::make_unique<IWorldManager>());
IWorldManager* WorldManager() {
	return WORLD_MANAGER.get();
}



std::unique_ptr<World> m_world;

World* IWorldManager::GetWorld() {
	return m_world.get();
}

fs::path IWorldManager::GetWorldsPath() {
	return "worlds";
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

void IWorldManager::LoadWorldDB() {
	LOG(INFO) << "Loading world: " << m_world->m_name;
	auto dbpath = GetWorldDBPath(m_world->m_name);

	auto&& opt = VUtils::Resource::ReadFileBytes(dbpath);

	if (!opt) {
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
				double netTime = 0;
				if (worldVersion >= 4)
					netTime = pkg.Read<double>();

				ZDOManager()->Load(pkg, worldVersion);

				if (worldVersion >= 12)
					ZoneManager()->Load(pkg, worldVersion);

				assert(false);

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
			LOG(ERROR) << "Unable to load world";
		}
	}
}



void IWorldManager::SaveWorldDB() {
	auto now(steady_clock::now());
	try {
		auto metaPath = GetWorldMetaPath(m_world->m_name);

		//DateTime now2 = DateTime.Now;
		auto now2(steady_clock::now());
		bool flag = true;
		auto dbpath3 = GetWorldDBPath(m_world->m_name);
		auto text = flag ? (dbpath3.string() + ".new") : dbpath3;
		auto oldFile = dbpath3.string() + ".old";
		LOG(INFO) << "World saving";

		NetPackage binary;

		//FileWriter fileWriter = new FileWriter(text, FileHelpers.FileHelperType.Binary, ZNet.m_world.m_fileSource);
		//ZLog.Log("World save writing started");
		//BinaryWriter binary = fileWriter.m_binary;

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



		VUtils::Resource::WriteFileBytes(text, binary.m_stream.m_buf);

		LOG(INFO) << "World save writing finishing";

		//ZLog.Log("World save writing finished");
		//bool flag2;
		//FileWriter fileWriter2;



		SaveWorldMeta(m_world.get());
		//ZNet.m_world.SaveWorldMetaData(now, out flag2, out fileWriter2);


		//  move dbpath3 to oldFile
		//	move text to dbpath3
		//if (flag)
			//FileHelpers.ReplaceOldFile(dbpath3, text, oldFile, ZNet.m_world.m_fileSource);

		// The current file is now the old file
		fs::rename(dbpath3, oldFile);

		// Save the new file 
		fs::rename(text, dbpath3);

		LOG(INFO) << "World saved ( " << duration_cast<milliseconds>(steady_clock::now() - now2).count() << "ms )";
		//now2 = DateTime.Now;
		//if (ZNet.ConsiderAutoBackup(dbpath3, ZNet.m_world.m_fileSource, now))
			//ZLog.Log("World auto backup saved ( " + (DateTime.Now - now2).ToString() + "ms )");			
	}
	catch (const std::exception& ex) {
		LOG(ERROR) << "Error saving world! " << ex.what();
	}
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
	}

	SaveWorldMeta(world.get());

	return world;
}

void IWorldManager::Init() {
	m_world = GetOrCreateWorldMeta(SERVER_SETTINGS.worldName);
}
