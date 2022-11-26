#include "World.h"

#include "VUtilsResource.h"
#include "NetPackage.h"
#include "VServer.h"
#include "VUtilsRandom.h"
#include "VUtilsString.h"
#include "NetSyncManager.h"
#include "ZoneSystem.h"

World::World(std::string name,
	std::string seedName,
	int32_t seed,
	OWNER_t uid,
	int32_t worldGenVersion
)
	: m_name(name), m_seedName(seedName), m_seed(seed), m_uid(uid), m_worldGenVersion(worldGenVersion)
{

}

namespace WorldManager {

	std::unique_ptr<World> m_world;

	World* GetWorld() {
		return m_world.get();
	}

	fs::path GetWorldsPath() {
		return "worlds";
	}

	fs::path GetWorldMetaPath(const std::string &name) {
		return GetWorldsPath() / (name + ".fwl");
	}

	fs::path GetWorldDBPath(const std::string& name) {
		return GetWorldsPath() / (name + ".db");
	}

	//void SaveWorldMetaData(DateTime backupTimestamp) {
	//	bool flag = false;
	//	FileWriter fileWriter;
	//	SaveWorldMetaData(backupTimestamp, out flag, out fileWriter);
	//}

	void SaveWorldMeta(World *world) {
		//auto dbpath = GetWorldDBPath(world->m_name);
		
		NetPackage pkg;

		pkg.Write(Version::WORLD);
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

	void LoadWorldDB() {
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
				if (worldVersion != Version::WORLD) {
					LOG(ERROR) << "Incompatible data version " << worldVersion;
				}
				else {
					double netTime = 0;
					if (worldVersion >= 4)
						netTime = pkg.Read<double>();

					NetSyncManager::Load(pkg, worldVersion);

					if (worldVersion >= 12)
						ZoneSystem::Load(pkg, worldVersion);

					assert(false);

					//if (worldVersion >= 15)
					//	RandEventManager::Load(pkg, worldVersion);
				}
			}
			catch (const std::exception& e) {
				LOG(ERROR) << "Unable to load world";
			}
		}
	}



	void SaveWorldDB() {
		//DateTime now = DateTime.Now;
		auto now(steady_clock::now());
		try {
			auto metaPath = GetWorldMetaPath(m_world->m_name);

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
			binary.Write(Version::WORLD);
			binary.Write((double) m_netTime);
			NetSyncManager::SaveAsync(binary);
			ZoneSystem::SaveAsync(binary);
			RandEventSystem::SaveAsync(binary);

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
			
			LOG(INFO) << "World saved ( " << duration_cast<milliseconds>(steady_clock::now()-now2).count() << "ms )";
			//now2 = DateTime.Now;
			//if (ZNet.ConsiderAutoBackup(dbpath3, ZNet.m_world.m_fileSource, now))
				//ZLog.Log("World auto backup saved ( " + (DateTime.Now - now2).ToString() + "ms )");			
		}
		catch (const std::exception& ex) {
			LOG(ERROR) << "Error saving world! " << ex.what();
		}
	}





	std::unique_ptr<World> GetOrCreateWorldMeta(const std::string& name) {
		// load world from file

		auto bytes = VUtils::Resource::ReadFileBytes(GetWorldMetaPath(name));

		std::unique_ptr<World> world;

		if (bytes) {
			NetPackage binary(bytes.value());
			auto zpackage = binary.Read<NetPackage>();

			auto worldVersion = zpackage.Read<int32_t>();
			if (worldVersion != Version::WORLD) {
				LOG(ERROR) << "Incompatible world version " << worldVersion;
			}
			else {
				try {
					world = std::make_unique<World>(
						zpackage.Read<std::string>(),	// name
						zpackage.Read<std::string>(),	// seedname
						zpackage.Read<int32_t>(),		// seed
						zpackage.Read<OWNER_t>(),		// uid
						worldVersion >= 26 ? zpackage.Read<int32_t>() : 0
					);					
				}
				catch (const std::range_error& e) {
					LOG(ERROR) << "World is corrupted or unsupported " << name;
				}
			}
		}
		
		LOG(INFO) << "Creating new world";

		// set default world
		if (!world) {
			std::string seedName = VUtils::Random::GenerateAlphaNum(10);

			world = std::make_unique<World>(name,
				seedName,
				VUtils::String::GetStableHashCode(seedName),
				VUtils::String::GetStableHashCode(name) + VUtils::Random::GenerateUID(),
				Version::WORLDGEN);
		}

		SaveWorldMeta(world.get());

		return world;
	}

	void Init() {
		m_world = GetOrCreateWorldMeta(SERVER_SETTINGS.worldName);
	}

}