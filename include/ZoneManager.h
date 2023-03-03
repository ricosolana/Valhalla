#pragma once

#include "VUtils.h"
#include "Vector.h"
#include "Quaternion.h"
#include "HashUtils.h"
#include "Peer.h"
#include "Biome.h"
#include "VUtilsRandom.h"
#include "Prefab.h"

class Heightmap;

using ZoneID = Vector2i;

// TODO rename Location to Feature, as this better describes
//	its purpose
class IZoneManager {
	friend class INetManager;

	// Rename this to ZoneFeature
	struct Feature {
		std::string m_name;
		HASH_t m_hash;

		Biome m_biome;
		BiomeArea m_biomeArea = BiomeArea::Everything;
		bool m_applyRandomDamage;
		bool m_centerFirst;
		bool m_clearArea;
		//bool m_useCustomInteriorTransform;

		float m_exteriorRadius = 10;
		float m_interiorRadius = 10;
		float m_forestTresholdMin;
		float m_forestTresholdMax = 1;
		//Vector3 m_interiorPosition;
		//Vector3 m_generatorPosition;
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
		std::vector<Prefab::Instance> m_pieces;

		bool operator==(const Feature& other) const {
			return this->m_name == other.m_name;
		}

		// Rename this
		class Instance {
		public:
			std::reference_wrapper<const Feature> m_location;
			const Vector3 m_position;
			//bool m_placed = false; // if m_generatedZones contains position, it can be considered placed

			Instance(const Feature& location, Vector3 pos)
				: m_location(location), m_position(pos) {}
		};
	};

	// Rename this to VegetationFeature
	class Foliage {
	public:
		//std::string m_name = "veg";
		//std::string m_prefabName;
		const Prefab* m_prefab = nullptr;

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
	//static constexpr int WORLD_SIZE_IN_ZONES = 316;
	static constexpr int WORLD_RADIUS_IN_ZONES = 324/2;
	static constexpr int WORLD_DIAMETER_IN_ZONES = WORLD_RADIUS_IN_ZONES * 2;

	//static_assert(WORLD_RADIUS_IN_ZONES % 2 == 0, "World size must be even");

private:
	// All templated ZoneLocations, sorted by priority
	std::vector<std::unique_ptr<const Feature>> m_locations;
	robin_hood::unordered_map<HASH_t, std::reference_wrapper<const Feature>> m_locationsByHash;

	// All templated Foliage
	// Generally unimplemented ATM
	std::vector<std::unique_ptr<const Foliage>> m_vegetation;

	// The generated ZoneLocations per world
	robin_hood::unordered_map<Vector2i, std::unique_ptr<Feature::Instance>> m_locationInstances;

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

	bool HaveLocationInRange(const Feature& loc, const Vector3& p);
	Vector3 GetRandomPointInZone(VUtils::Random::State& state, const ZoneID &zone, float locationRadius);
	Vector3 GetRandomPointInRadius(VUtils::Random::State& state, const Vector3& center, float radius);
	bool InsideClearArea(const std::vector<ClearArea>& areas, const Vector3& point);
	bool OverlapsClearArea(const std::vector<ClearArea>& areas, const Vector3& point, float radius);
	const Feature* GetLocation(HASH_t hash);
	const Feature* GetLocation(const std::string& name);

	void GenerateLocations(const Feature &location);
	Vector2i GetRandomZone(VUtils::Random::State &state, float range);

	void RemoveUnplacedLocations(const Feature& location);
	void SpawnLocation(const Feature& location, HASH_t seed, const Vector3 &pos, const Quaternion &rot);
	void CreateLocationProxy(const Feature& location, HASH_t seed, const Vector3 &pos, const Quaternion &rot);
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

	std::list<std::reference_wrapper<Feature::Instance>> GetLocationIcons();
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

	// Find the nearest location
	//	Nullable
	Feature::Instance *FindClosestLocation(const std::string& name, const Vector3& point);

	static ZoneID WorldToZonePos(const Vector3& point);
	static Vector3 ZoneToWorldPos(const ZoneID& id);

	bool ZonesOverlap(const ZoneID& zone, const Vector3& areaPoint);
	bool ZonesOverlap(const ZoneID& zone, const ZoneID& areaZone);

	bool IsPeerNearby(const ZoneID& zone, OWNER_t uid);

	//bool IsPeerNearby(const Vector3& pos, OWNER_t uid) {
	//	return IsPeerNearby(WorldToZonePos(pos), uid);
	//}

	void ResetGlobalKeys();

	//bool GetWorldNormal(const Vector3& worldPos, Vector3& normal);
};

IZoneManager* ZoneManager();
