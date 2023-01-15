#pragma once

#include "VUtils.h"
#include "Vector.h"
#include "Quaternion.h"
#include "HeightMap.h"
#include "ZDO.h"
#include "NetObject.h"
#include "GameObject.h"

namespace ZoneSystem {
	// eventually redundant?
	//void GenerateLocationsIfNeeded();
	// 
	//void PrepareNetViews(GameObject root, std::vector<ZNetView> &views);
	//void PrepareRandomSpawns(GameObject root, std::vector<RandomSpawn> &randomSpawns);
	void PrepareSave();
	void SaveAsync(NetPackage& writer);
	void Load(NetPackage& reader, int32_t version);
	bool IsZoneLoaded(const Vector3& point);
	bool IsZoneLoaded(const Vector2i& zoneID);

	// client only because ZNet ref pos
	//bool IsActiveAreaLoaded();
	//void GenerateLocations();

	// client only
	//bool TestSpawnLocation(const std::string& name, const Vector3& pos, bool disableSave = true);
	GameObject SpawnProxyLocation(int32_t hash, int32_t seed, const Vector3& pos, const Quaternion& rot);
	bool GetLocationIcon(const std::string& name, Vector3& pos);
	void GetLocationIcons(robin_hood::unordered_map<Vector3, std::string> icons);
	bool IsBlocked(const Vector3& p);
	float GetAverageGroundHeight(const Vector3& p, float radius);
	float GetGroundHeight(const Vector3& p);
	bool GetGroundHeight(const Vector3& p, float& height);
	float GetSolidHeight(const Vector3& p);
	bool GetSolidHeight(const Vector3& p, float& height, int32_t heightMargin = 1000);
	bool GetSolidHeight(const Vector3& p, float& radius, float height, Transform ignore);
	bool GetSolidHeight(const Vector3& p, float& height, const Vector3& normal);
	bool GetSolidHeight(const Vector3& p, float& height, const Vector3& normal, GameObject go);
	bool GetStaticSolidHeight(const Vector3& p, float& height, const Vector3& normal);
	bool FindFloor(const Vector3& p, float& height);
	Heightmap* GetGroundData(Vector3& p, Vector3& normal, Heightmap::Biome& biome, Heightmap::BiomeArea& biomeArea);
	bool FindClosestLocation(const std::string& name, const Vector3& point, LocationInstance& closest);


	Vector2i WorldToZonePos(const Vector3& point);
	Vector3 ZoneToWorldPos(const Vector2i& id);

	bool SkipSaving();
	// Used only for backups and
	//float TimeSinceStart();
	void ResetGlobalKeys();
	//void SetGlobalKey(const std::string& name);
	//bool GetGlobalKey(const std::string& name);
	//void RemoveGlobalKey(const std::string& name);
	//std::vector<std::string> GetGlobalKeys();

	// used by client terminal command
	//robin_hood::unordered_map<Vector2i, LocationInstance>.ValueCollection GetLocationList();

	static constexpr int ACTIVE_AREA = 1;
	static constexpr int ACTIVE_DISTANT_AREA = 1;

	static constexpr float ZONE_SIZE = 64;

	static constexpr float WATER_LEVEL = 30;

}