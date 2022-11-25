#include "World.h"

#include "VUtilsResource.h"
#include "NetPackage.h"

namespace WorldManager {



	std::unique_ptr<World> LoadWorld(const std::string& name) {
		// load world from file

		// TODO go over this all again

		auto bytes = VUtils::Resource::ReadFileBytes(
			VUtils::Resource::GetPath(std::string("worlds/") + name + ".fwl").string()
		);

		if (!bytes) {
			LOG(ERROR) << "Failed to load world " << name;
			return std::unique_ptr<World>(new World{ name, "", 0, 0 });
		} 

		NetPackage binary;

		//int count = binary.Read<int32_t>();
		auto zpackage = binary.Read<NetPackage>();
		//ZPackage zpackage = new ZPackage(binary.ReadBytes(count));
		int num = zpackage.Read<int32_t>();
		//if (!global::Version.IsWorldVersionCompatible(num))
		//{
		//	ZLog.Log("incompatible world version " + num.ToString());
		//	result = new World(name, false, true, fileSource);
		//}
		//else
		//{
			World world = new World();
			world.m_fileSource = fileSource;
			world.m_fileName = name;
			world.m_name = zpackage.ReadString();
			world.m_seedName = zpackage.ReadString();
			world.m_seed = zpackage.ReadInt();
			world.m_uid = zpackage.ReadLong();
			if (num >= 26)
			{
				world.m_worldGenVersion = zpackage.ReadInt();
			}
			result = world;

			return std::unique_ptr<World>(new World{ 
				zpackage.Read<std::string>(), zpackage.Read<std::string>(), zpackage.Read<int32_t>(), zpackage.Read<OWNER_t>() });
		//}

		catch
		{
			ZLog.LogWarning("  error loading world " + name);
			result = new World(name, true, false, fileSource);
		}
		finally
		{
			if (fileReader != null)
			{
				fileReader.Dispose();
			}
		}
		return result;
	}

	std::unique_ptr<World> GetCreateWorld(const std::string& name) {
		LOG(INFO) << "Get create world " << name;
		//if (source == FileHelpers.FileSource.Local)
		//{
		//	string metaPath = World.GetMetaPath(name, source);
		//	string metaPath2 = World.GetMetaPath(name, FileHelpers.FileSource.Legacy);
		//	if (!File.Exists(metaPath) && File.Exists(metaPath2))
		//	{
		//		ZLog.Log("Local world doesn't exist but legacy does, using legacy path.");
		//		source = FileHelpers.FileSource.Legacy;
		//	}
		//}
		World world = World.LoadWorld(name, source);
		if (!world.m_loadError && !world.m_versionError)
		{
			return world;
		}
		ZLog.Log(" creating");
		world = new World(name, World.GenerateSeed());
		world.m_fileSource = source;
		world.SaveWorldMetaData(DateTime.Now);
		return world;
	}


}