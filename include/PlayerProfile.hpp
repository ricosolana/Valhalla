#pragma once

#include "Utils.hpp"
#include "ZPackage.hpp"
#include <robin_hood.h>
#include "Player.hpp"

struct PlayerStats {
	int m_kills;
	int m_deaths;
	int m_crafts;
	int m_builds;
};

// Token: 0x020000B0 RID: 176
class PlayerProfile {
	struct WorldPlayerData {
		Vector3 m_spawnPoint;
		bool m_haveCustomSpawnPoint;
		Vector3 m_logoutPoint;
		bool m_haveLogoutPoint;
		Vector3 m_deathPoint;
		bool m_haveDeathPoint;
		Vector3 m_homePoint;
		//byte[] m_mapData;
		std::vector<byte> m_mapData;
	};

	//bool SavePlayerToDisk();
	//bool LoadPlayerFromDisk();
	//ZPackage LoadPlayerDataFromDisk();
	//WorldPlayerData GetWorldData(UID_t worldUID);

	robin_hood::unordered_map<int64_t, WorldPlayerData> m_worldData;
	std::vector<byte> m_playerData;

public:
	PlayerProfile(std::string_view filename = "");

	//bool Load();
	//bool Save();
	//bool HaveIncompatiblePlayerData();

	//void SavePlayerData(Player player);
	//void LoadPlayerData(Player player);
	//void SaveLogoutPoint();

	//void SetLogoutPoint(Vector3& point);
	//void SetDeathPoint(Vector3& point);
	//void SetMapData(std::vector<byte>& data); // allocate new 
	//std::vector<byte>& GetMapData();
	//void ClearLogoutPoint();
	//bool HaveLogoutPoint();
	//Vector3& GetLogoutPoint();
	//bool HaveDeathPoint();
	//Vector3& GetDeathPoint();
	//void SetCustomSpawnPoint(Vector3& point);
	//Vector3& GetCustomSpawnPoint();
	//bool HaveCustomSpawnPoint();
	//void ClearCustomSpawnPoint();
	//void SetHomePoint(Vector3& point);
	//Vector3& GetHomePoint();
	//UID_t GetPlayerID();
	//static std::vector<PlayerProfile> GetAllPlayerProfiles();
	//static void RemoveProfile(std::string& name);
	//static bool HaveProfile(std::string& name);
	//std::string& GetFilename();

	std::string m_filename;
	std::string m_playerName;
	UID_t m_playerID = 0;
	std::string m_startSeed;

	// figure out usages
	//static Vector3 m_originalSpawnPoint(-676.f, 50.f, 299.f);
	PlayerStats m_playerStats;
};
