#pragma once

#include "VUtils.h"
#include "Vector.h"
#include "Quaternion.h"
#include "HashUtils.h"
#include "Peer.h"
#include "Biome.h"
#include "VUtilsRandom.h"

class Prefab;
class Heightmap;

using ZoneID = Vector2i;

// TODO rename Location to Feature, as this better describes
//	its purpose
class IZoneManager {
	friend class INetManager;

	// Rename this to ZoneFeature
	struct ZoneLocation {
		struct Piece {
			const Prefab* m_prefab;
			Vector3 m_pos;
			Quaternion m_rot = Quaternion::IDENTITY; // hmm
		};

		std::string m_name;
		HASH_t m_hash;
		//const Prefab* m_prefab;

		Biome m_biome;
		BiomeArea m_biomeArea = BiomeArea::Everything;
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
		//bool m_prioritized;
		int32_t m_spawnAttempts; // 200000 or 100000 depending on priority
		int32_t m_quantity;
		bool m_randomRotation = true;
		//std::vector<RandomSpawn> m_randomSpawns;
		bool m_slopeRotation;
		bool m_snapToWater;
		bool m_unique;
		//std::vector<std::tuple<HASH_t, Vector3, Quaternion>> m_prefabs; // m_netViews;
		std::vector<Piece> m_pieces;
	};

	// Rename this to VegetationFeature
	struct ZoneVegetation {
		//std::string m_name = "veg";
		//std::string m_prefabName;
		const Prefab* m_prefab;

		Biome m_biome = Biome::None;
		BiomeArea m_biomeArea = BiomeArea::Everything;
		float m_radius = 0; // My custom impl
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

	// Rename this
	struct LocationInstance {
		const ZoneLocation *m_location;
		Vector3 m_position;
		//bool m_placed; // not needed, assuming locations are placed only once during zone spawn (when player enters a new zone)
	};

	struct ClearArea {
		Vector3 m_center;
		float m_semiWidth;
	};

	const Prefab* LOCATION_PROXY_PREFAB = nullptr;
	const Prefab* ZONE_CTRL_PREFAB = nullptr;

public:
	static constexpr int NEAR_ACTIVE_AREA = 2;
	static constexpr int DISTANT_ACTIVE_AREA = 2;
	static constexpr int ZONE_SIZE = 64;
	static constexpr float WATER_LEVEL = 30;

private:
	// All templated ZoneLocations, sorted by priority
	std::vector<std::unique_ptr<const ZoneLocation>> m_locations;
	robin_hood::unordered_map<HASH_t, const ZoneLocation*> m_locationsByHash;

	// All templated ZoneVegetation
	// Generally unimplemented ATM
	std::vector<std::unique_ptr<const ZoneVegetation>> m_vegetation;

	// The generated ZoneLocations per world
	robin_hood::unordered_map<Vector2i, LocationInstance> m_locationInstances;

	// Which Zones have already been generated (locations and vegetation placed)
	robin_hood::unordered_set<Vector2i> m_generatedZones;

	// Game-state global keys
	// Used only by client (and saved on the server for syncing to other clients)
	// TODO They dont really belong here, but I'm just following Valheim
	robin_hood::unordered_set<std::string> m_globalKeys;

private:
	void SendGlobalKeys(OWNER_t peer);
	void SendLocationIcons(OWNER_t peer);
	void OnNewPeer(Peer* peer);

	void CreateGhostZones(const Vector3& refPoint);

	bool SpawnZone(const ZoneID& zone);
	std::vector<ClearArea> PlaceLocations(const ZoneID& zone);
	void PlaceVegetation(const ZoneID& zone, Heightmap& heightmap, const std::vector<ClearArea>& clearAreas);
	void PlaceZoneCtrl(const ZoneID& zone);

	bool HaveLocationInRange(const ZoneLocation* loc, const Vector3& p);
	Vector3 GetRandomPointInZone(VUtils::Random::State& state, const ZoneID &zone, float locationRadius);
	Vector3 GetRandomPointInRadius(VUtils::Random::State& state, const Vector3& center, float radius);
	bool InsideClearArea(const std::vector<ClearArea>& areas, const Vector3& point);
	bool OverlapsClearArea(const std::vector<ClearArea>& areas, const Vector3& point, float radius);
	const ZoneLocation* GetLocation(HASH_t hash);
	const ZoneLocation* GetLocation(const std::string& name);

	void GenerateLocations(const ZoneLocation *location);
	Vector2i GetRandomZone(VUtils::Random::State &state, float range);

	void RemoveUnplacedLocations(const ZoneLocation* location);
	void SpawnLocation(const ZoneLocation* location, HASH_t seed, const Vector3 &pos, const Quaternion &rot);
	void CreateLocationProxy(const ZoneLocation* location, HASH_t seed, const Vector3 &pos, const Quaternion &rot);
	void GetTerrainDelta(VUtils::Random::State& state, const Vector3& center, float radius, float& delta, Vector3& slopeDirection);

	// inlined 
	//void SetZoneGenerated(const Vector2i& zoneID);

	bool IsZoneGenerated(const ZoneID& zoneID);

public:
	void GenerateLocations();

	void Init();

	void Update();

	void Save(DataWriter& pkg);
	void Load(DataReader& reader, int32_t version);

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
	Heightmap& GetGroundData(Vector3& p, Vector3& normal, Biome& biome, BiomeArea& biomeArea);
	bool FindClosestLocation(const std::string& name, const Vector3& point, LocationInstance& closest);

	static ZoneID WorldToZonePos(const Vector3& point);
	static Vector3 ZoneToWorldPos(const ZoneID& id);

	bool ZonesOverlap(const ZoneID& zone, const Vector3& areaPoint);
	bool ZonesOverlap(const ZoneID& zone, const ZoneID& areaZone);

	bool IsInPeerActiveArea(const ZoneID& zone, OWNER_t uid);

	void ResetGlobalKeys();

	//bool GetWorldNormal(const Vector3& worldPos, Vector3& normal);
};

IZoneManager* ZoneManager();
