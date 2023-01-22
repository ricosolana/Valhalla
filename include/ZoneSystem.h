#pragma once

#include "VUtils.h"
#include "Vector.h"
#include "Quaternion.h"
#include "HeightMap.h"
#include "GameObject.h"

class IZoneManager {




	// All public vars, however a LOT of them are editor visible vars
	//[HideInInspector]
	std::vector<Heightmap::Biome> m_biomeFolded;

	//[HideInInspector]
	std::vector<Heightmap::Biome> m_vegetationFolded;

	//[HideInInspector]
	std::vector<Heightmap::Biome> m_locationFolded;

	//[NonSerialized]
	bool m_drawLocations;

	//[NonSerialized]
	std::string m_drawLocationsFilter = "";

	//[global::Tooltip("Zones to load around center sector")]
	int32_t m_activeArea = 1;

	int32_t m_activeDistantArea = 1;

	//[global::Tooltip("Zone size, should match netscene sector size")]
	//float m_zoneSize = 64;

	//[global::Tooltip("Time before destroying inactive zone")]
	float m_zoneTTL = 4;

	//[global::Tooltip("Time before spawning active zone")]
	float m_zoneTTS = 4;

	GameObject m_zonePrefab;

	GameObject m_zoneCtrlPrefab;

	GameObject m_locationProxyPrefab;

	float m_waterLevel = 30;

	//[Header("Versions")]
	//int32_t m_pgwVersion = 53;

	int32_t m_locationVersion = 1;

	//[Header("Generation data")]
	std::vector<std::string> m_locationScenes;

	std::vector<ZoneVegetation> m_vegetation;

	std::vector<std::unique_ptr<ZoneLocation>> m_locations;

	bool m_didZoneTest;

	//[HideInInspector]
	robin_hood::unordered_map<Vector2i, LocationInstance> m_locationInstances;




	// Private vars

	robin_hood::unordered_map<Vector3, std::string> tempIconList;

	//RaycastHit[] rayHits = new RaycastHit[200];

	robin_hood::unordered_map<int, ZoneLocation*> m_locationsByHash;

	bool m_error;

	int32_t m_terrainRayMask;

	int32_t m_blockRayMask;

	int32_t m_solidRayMask;

	int32_t m_staticSolidRayMask;

	float m_updateTimer;

	// doesnt appear to be used for anything significant
	//float m_startTime;
	//float m_lastFixedTime;

	robin_hood::unordered_map<Vector2i, ZoneData> m_zones;

	robin_hood::unordered_set<Vector2i> m_generatedZones;

	//bool m_locationsGenerated;

	robin_hood::unordered_map<Vector3, std::string> m_locationIcons;

	robin_hood::unordered_set<std::string> m_globalKeys;



	NetPackage m_worldSave;
	/*
	robin_hood::unordered_set<Vector2i> m_tempGeneratedZonesSaveClone;
	robin_hood::unordered_set<std::string> m_tempGlobalKeysSaveClone;
	std::vector<LocationInstance> m_tempLocationsSaveClone;
	bool m_tempLocationsGeneratedSaveClone;
	*/
	// TODO not used for anything complex; just another reuse var...
	// make a local or static...


private:
	void Init();


	//void Awake();

	void Start();
	void SendGlobalKeys(OWNER_t peer);
	void SendLocationIcons(OWNER_t peer);
	void OnNewPeer(OWNER_t peerID);
	//void SetupLocations();
	//void ValidateVegetation();
	void Update();
	void CreateGhostZones(const Vector3& refPoint);
	//bool CreateLocalZones(const Vector3& refPoint);
	//bool PokeLocalZone(const Vector2i& zoneID);

	void SpawnZone(const Vector2i& zoneID);
	void PlaceZoneCtrl(const Vector2i& zoneID, std::vector<GameObject>& spawnedObjects);
	void PlaceVegetation(const Vector2i& zoneID, Heightmap* hmap, std::vector<ClearArea>& clearAreas, std::vector<GameObject>& spawnedObjects);
	void PlaceLocations(const Vector2i& zoneID, std::vector<ClearArea>& clearAreas, std::vector<GameObject>& spawnedObjects);

	Vector3 GetRandomPointInRadius(VUtils::Random::State& state, const Vector3& center, float radius);
	bool InsideClearArea(const std::vector<ClearArea>& areas, const Vector3& point);
	ZoneLocation* GetLocation(int32_t hash);
	ZoneLocation* GetLocation(const std::string& name);
	void ClearNonPlacedLocations();
	void CheckLocationDuplicates();
	//int32_t CountNrOfLocation(ZoneLocation* location);
	//void GenerateLocations(ZoneLocation location);
	//Vector2i GetRandomZone(float range);
	//Vector3 GetRandomPointInZone(const Vector2i& zone, float locationRadius);
	//Vector3 GetRandomPointInZone(float locationRadius);

	void RemoveUnplacedLocations(ZoneLocation* location);
	GameObject SpawnLocation(ZoneLocation* location, int32_t seed, Vector3 pos, Quaternion rot, std::vector<GameObject>& spawnedGhostObjects);
	void CreateLocationProxy(ZoneLocation* location, int32_t seed, Vector3 pos, Quaternion rotation, SpawnMode mode, std::vector<GameObject>& spawnedGhostObjects);
	void RegisterLocation(ZoneLocation* location, const Vector3& pos, bool generated);
	bool HaveLocationInRange(const std::string& prefabName, const std::string& group, const Vector3& p, float radius);
	void GetTerrainDelta(const Vector3& center, float& radius, float& delta, Vector3& slopeDirection);
	void UpdateTTL(float dt);

	// inlined 
	//void SetZoneGenerated(const Vector2i& zoneID);

	bool IsZoneGenerated(const Vector2i& zoneID);

public:
	// alternate names
	//	coord, 
	// ill just forget about this
	//using ZonePoint = Vector2i;

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

	// Vec2 is too ambigious, maybe use more refined 
	//	zone coordinate type?
	static bool InActiveArea(const Vector2i& zone, const Vector3& areaPoint);

	static bool InActiveArea(const Vector2i& zone, const Vector2i& areaZone);


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


	static Vector2i WorldToZonePos(const Vector3& point);
	static Vector3 ZoneToWorldPos(const Vector2i& id);

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
};

IManagerZone* ZoneSystem();
