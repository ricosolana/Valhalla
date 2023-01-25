#pragma once

#include "VUtils.h"
#include "Vector.h"
#include "Quaternion.h"
#include "HeightMap.h"

class Prefab;

using ZoneID = Vector2i;

class IZoneManager {
	struct ZoneLocation {
		struct Piece {
			const Prefab* m_prefab;
			Vector3 m_pos;
			Quaternion m_rot = Quaternion::IDENTITY; // hmm
		};

		//std::string m_prefabName;
		const Prefab* m_prefab;

		Heightmap::Biome m_biome;
		Heightmap::BiomeArea m_biomeArea = Heightmap::BiomeArea::Everything;
		bool m_applyRandomDamage;
		bool m_centerFirst;
		bool m_clearArea;
		bool m_useCustomInteriorTransform;

		float m_exteriorRadius = 10;
		float m_interiorRadius = 10;
		float m_forestTresholdMin;
		float m_forestTresholdMax = 1;
		Vector3 m_interiorPosition;
		Vector3 m_generatorPosition;
		std::string m_group = "";
		bool m_iconAlways;
		bool m_iconPlaced;
		bool m_inForest;
		float m_minAltitude = -1000;
		float m_maxAltitude = 1000;
		float m_minDistance;
		float m_maxDistance;
		float m_minTerrainDelta;
		float m_maxTerrainDelta = 2;
		float m_minDistanceFromSimilar;
		bool m_prioritized;
		int32_t m_quantity;
		bool m_randomRotation = true;
		//std::vector<RandomSpawn> m_randomSpawns;
		bool m_slopeRotation;
		bool m_snapToWater;
		bool m_unique;
		//std::vector<std::tuple<HASH_t, Vector3, Quaternion>> m_prefabs; // m_netViews;
		std::vector<Piece> m_pieces;
	};

	struct ZoneVegetation {
		//std::string m_name = "veg";
		//std::string m_prefabName;
		const Prefab* m_prefab;

		Heightmap::Biome m_biome = Heightmap::Biome::None;
		Heightmap::BiomeArea m_biomeArea = Heightmap::BiomeArea::Everything;
		float m_min = 0;
		float m_max = 10;
		float m_minTilt = 0;
		float m_maxTilt = 90;
		float m_groupRadius = 0;
		bool m_forcePlacement = false;
		int32_t m_groupSizeMin = 1;
		int32_t m_groupSizeMax = 1;
		float m_scaleMin = 1;
		float m_scaleMax = 1;
		float m_randTilt = 0;
		bool m_blockCheck = true;
		float m_minAltitude = -1000;
		float m_maxAltitude = 1000;
		float m_minOceanDepth = 0;
		float m_maxOceanDepth = 0;
		float m_terrainDeltaRadius = 0;
		float m_minTerrainDelta = 0;
		float m_maxTerrainDelta = 2;
		bool m_inForest = false;
		float m_forestTresholdMin = 0;
		float m_forestTresholdMax = 1;
		bool m_snapToWater = false;
		bool m_snapToStaticSolid = false;
		float m_groundOffset = 0;
		float m_chanceToUseGroundTilt = 0;
		float m_minVegetation = 0;
		float m_maxVegetation = 0;
	};

	// can be private
	struct LocationInstance {
		const ZoneLocation *m_location;
		Vector3 m_position;

		// hmm; since locations position are pregenerated, 
		// then spawned once zone is loaded
		// this is needed
		bool m_placed; 		
	};

	struct ClearArea {
		Vector3 m_center;
		float m_radius;
	};

	const Prefab* LOCATION_PROXY_PREFAB = nullptr;
	const Prefab* ZONE_CTRL_PREFAB = nullptr;

public:
	static constexpr int NEAR_ACTIVE_AREA = 1;
	static constexpr int DISTANT_ACTIVE_AREA = 1;
	static constexpr int ZONE_SIZE = 64;
	static constexpr float WATER_LEVEL = 30;

private:
	float ZONE_TTL = 4;
	float ZONE_TTS = 4;

	std::vector<std::string> m_locationScenes;

	std::vector<std::unique_ptr<ZoneVegetation>> m_vegetation;

	//std::vector<std::unique_ptr<ZoneLocation>> m_locations;

	robin_hood::unordered_map<HASH_t, std::unique_ptr<ZoneLocation>> m_locationsByHash;

	//[HideInInspector]
	robin_hood::unordered_map<Vector2i, LocationInstance> m_locationInstances;



	// Private vars

	robin_hood::unordered_map<Vector3, std::string> tempIconList;

	//robin_hood::unordered_map<Vector2i, ZoneData> m_zones;

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

	void GenerateLocations();

	void GenerateLocations(const ZoneLocation *location);
	Vector2i GetRandomZone(VUtils::Random::State &state, float range);

	void RemoveUnplacedLocations(const ZoneLocation* location);
	void SpawnLocation(const ZoneLocation* location, int32_t seed, const Vector3 &pos, const Quaternion &rot);
	void CreateLocationProxy(const ZoneLocation* location, int32_t seed, const Vector3 &pos, const Quaternion &rot);
	void RegisterLocation(const ZoneLocation* location, const Vector3& pos, bool generated);
	bool HaveLocationInRange(const std::string& prefabName, const std::string& group, const Vector3& p, float radius);
	void GetTerrainDelta(const Vector3& center, float radius, float& delta, Vector3& slopeDirection);

	// inlined 
	//void SetZoneGenerated(const Vector2i& zoneID);

	bool IsZoneGenerated(const Vector2i& zoneID);

public:

	void Save(NetPackage& pkg);
	void Load(NetPackage& reader, int32_t version);

	void GetLocationIcons(robin_hood::unordered_map<Vector3, std::string> &icons);
	bool IsBlocked(const Vector3& p);
	float GetGroundHeight(const Vector3& p);
	bool GetGroundHeight(const Vector3& p, float& height);
	float GetSolidHeight(const Vector3& p);
	// ?? client only ??
	//bool GetSolidHeight(const Vector3& p, float& height, int32_t heightMargin = 1000);
	//bool GetSolidHeight(const Vector3& p, float& radius, float height, Transform ignore);
	bool GetStaticSolidHeight(const Vector3& p, float& height, const Vector3& normal);
	//bool FindFloor(const Vector3& p, float& height);
	Heightmap* GetGroundData(Vector3& p, Vector3& normal, Heightmap::Biome& biome, Heightmap::BiomeArea& biomeArea);
	bool FindClosestLocation(const std::string& name, const Vector3& point, LocationInstance& closest);

	static ZoneID WorldToZonePos(const Vector3& point);
	static Vector3 ZoneToWorldPos(const ZoneID& id);

	static bool ZonesOverlap(const ZoneID& zone, const Vector3& areaPoint);
	static bool ZonesOverlap(const ZoneID& zone, const ZoneID& areaZone);

	void ResetGlobalKeys();
};

IZoneManager* ZoneManager();
