#pragma once

#include "VUtils.h"
#include "Vector.h"
#include "Quaternion.h"
#include "HeightMap.h"
#include "GameObject.h"

using ZoneID = Vector2i;

class IZoneManager {

public:
	static constexpr int NEAR_ACTIVE_AREA = 1;
	static constexpr int DISTANT_ACTIVE_AREA = 1;
	static constexpr int ZONE_SIZE = 64;
	static constexpr float WATER_LEVEL = 30;

private:
	float ZONE_TTL = 4;
	float ZONE_TTS = 4;

	std::vector<std::string> m_locationScenes;

	std::vector<ZoneVegetation> m_vegetation;

	std::vector<std::unique_ptr<ZoneLocation>> m_locations;

	//[HideInInspector]
	robin_hood::unordered_map<Vector2i, LocationInstance> m_locationInstances;



	// Private vars

	robin_hood::unordered_map<Vector3, std::string> tempIconList;

	robin_hood::unordered_map<int, ZoneLocation*> m_locationsByHash;

	robin_hood::unordered_map<Vector2i, ZoneData> m_zones;

	robin_hood::unordered_set<Vector2i> m_generatedZones;

	robin_hood::unordered_map<Vector3, std::string> m_locationIcons;

	robin_hood::unordered_set<std::string> m_globalKeys;

private:
	void Init();

	void SendGlobalKeys(OWNER_t peer);
	void SendLocationIcons(OWNER_t peer);
	void OnNewPeer(OWNER_t peerID);

	void Update();
	void CreateGhostZones(const Vector3& refPoint);

	void SpawnZone(const ZoneID& zone);
	void PlaceZoneCtrl(const ZoneID& zone);
	void PlaceVegetation(const ZoneID& zone, Heightmap* hmap, std::vector<ClearArea>& clearAreas);
	void PlaceLocations(const ZoneID& zone, std::vector<ClearArea>& clearAreas);

	Vector3 GetRandomPointInRadius(VUtils::Random::State& state, const Vector3& center, float radius);
	bool InsideClearArea(const std::vector<ClearArea>& areas, const Vector3& point);
	ZoneLocation* GetLocation(int32_t hash);
	ZoneLocation* GetLocation(const std::string& name);
	void ClearNonPlacedLocations();
	void CheckLocationDuplicates();
	//int32_t CountNrOfLocation(ZoneLocation* location);
	void GenerateLocations(ZoneLocation *location);
	//Vector2i GetRandomZone(float range);
	//Vector3 GetRandomPointInZone(const Vector2i& zone, float locationRadius);
	//Vector3 GetRandomPointInZone(float locationRadius);

	void RemoveUnplacedLocations(ZoneLocation* location);
	GameObject SpawnLocation(ZoneLocation* location, int32_t seed, const Vector3 &pos, const Quaternion &rot);
	void CreateLocationProxy(ZoneLocation* location, int32_t seed, const Vector3 &pos, const Quaternion &rot);
	void RegisterLocation(ZoneLocation* location, const Vector3& pos, bool generated);
	bool HaveLocationInRange(const std::string& prefabName, const std::string& group, const Vector3& p, float radius);
	void GetTerrainDelta(const Vector3& center, float& radius, float& delta, Vector3& slopeDirection);
	void UpdateTTL(float dt);

	// inlined 
	//void SetZoneGenerated(const Vector2i& zoneID);

	bool IsZoneGenerated(const Vector2i& zoneID);

public:

	void Save(NetPackage& pkg);
	void Load(NetPackage& reader, int32_t version);
	bool IsZoneLoaded(const Vector3& point);
	bool IsZoneLoaded(const Vector2i& zoneID);

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

	static ZoneID WorldToZonePos(const Vector3& point);
	static Vector3 ZoneToWorldPos(const Vector2i& id);

	static bool InActiveArea(const Vector2i& zone, const Vector3& areaPoint);
	static bool InActiveArea(const Vector2i& zone, const Vector2i& areaZone);

	void ResetGlobalKeys();


};

IZoneManager* ZoneManager();
