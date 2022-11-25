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
	OWNER_t uid
	//int32_t worldGenVersion
)
	: m_name(name), m_seedName(seedName), m_seed(seed), m_uid(uid) //m_worldGenVersion(worldGenVersion)
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

		pkg.Write(VALHEIM_WORLD_VERSION);
		pkg.Write(world->m_name);
		pkg.Write(world->m_seedName);
		pkg.Write(world->m_seed);
		pkg.Write(world->m_uid);
		pkg.Write(VALHEIM_WORLDGEN_VERSION);

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
			try
			{
				NetPackage pkg(opt.value());

				auto worldVersion = pkg.Read<int32_t>();
				if (worldVersion != VALHEIM_WORLD_VERSION) {
					LOG(ERROR) << "Incompatible data version " << worldVersion;
				}
				else {
					double netTime = 0;
					if (worldVersion >= 4)
						netTime = pkg.Read<double>();

					NetSyncManager::Load(pkg, worldVersion);

					if (worldVersion >= 12)
						ZoneSystem::Load(pkg, worldVersion);

					if (worldVersion >= 15)
						RandEventManager::Load(pkg, worldVersion);
				}
			}
			catch (const std::exception& e) {
				LOG(ERROR) << "Unable to load world";
			}
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
			if (worldVersion != VALHEIM_WORLD_VERSION) {
				LOG(ERROR) << "Incompatible world version " << worldVersion;
			}
			else {
				try {
					if (worldVersion >= 26) {
						if (zpackage.Read<int32_t>() != VALHEIM_WORLDGEN_VERSION)
							LOG(ERROR) << "Unsupported world gen version";
					}
					else {
						world = std::make_unique<World>(
							zpackage.Read<std::string>(),	// name
							zpackage.Read<std::string>(),	// seedname
							zpackage.Read<int32_t>(),		// seed
							zpackage.Read<OWNER_t>()		// uid
						);
					}
				}
				catch (const std::range_error& e) {
					LOG(ERROR) << "World is corrupted or unsupported " << name;
				}
			}
		}
		
		LOG(INFO) << "Creating new world";

		// set default world
		if (!world) {
			std::string chars = "abcdefghijklmnpqrstuvwxyzABCDEFGHIJKLMNPQRSTUVWXYZ023456789";
			std::string seedName;
			VUtils::Random::State state;
			for (int i = 0; i < 10; i++) {
				seedName += chars[state.Range(0, chars.length())];
			}
			world = std::make_unique<World>(name,
				seedName,
				VUtils::String::GetStableHashCode(seedName),
				VUtils::String::GetStableHashCode(name) + VUtils::GenerateUID());
		}

		SaveWorldMeta(world.get());

		return world;
	}

	void Init() {
		m_world = GetOrCreateWorldMeta(SERVER_SETTINGS.worldName);
	}

}