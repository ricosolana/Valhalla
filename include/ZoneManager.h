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
		std::vector<Prefab::Instance> m_pieces;

		bool operator==(const Feature& other) const {
			return this->m_name == other.m_name;
		}

		// Rename this
		class Instance {
		public:
			std::reference_wrapper<const Feature> m_feature;
			const Vector3 m_pos;
			//bool m_placed = false; // if m_generatedZones contains position

			Instance(const Feature& location, const Vector3 &pos)
				: m_feature(location), m_pos(pos) {}
		};
	};

	// Rename this to VegetationFeature
	class Foliage {
	public:
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
	static constexpr int WORLD_RADIUS_IN_ZONES = 157;
	static constexpr int WORLD_DIAMETER_IN_ZONES = WORLD_RADIUS_IN_ZONES * 2;

private:
	// All Features within a world capable of generation
	std::vector<std::unique_ptr<const Feature>> m_features;

	// All Features within a world hashed by name
	robin_hood::unordered_map<HASH_t, std::reference_wrapper<const Feature>> m_featuresByHash;

	// All Foliage within a world capable of generation
	std::vector<std::unique_ptr<const Foliage>> m_foliage;

	// All the generated Features in a world
	robin_hood::unordered_map<ZoneID, std::unique_ptr<Feature::Instance>> m_generatedFeatures;

	// Which Zones have already been generated
	robin_hood::unordered_set<ZoneID> m_generatedZones;

	// Game-state global keys
	robin_hood::unordered_set<std::string> m_globalKeys;

private:
	void SendGlobalKeys(OWNER_t peer);
	void SendLocationIcons(OWNER_t peer);
	void OnNewPeer(Peer& peer);

	void CreateGhostZones(const Vector3& pos);

	bool SpawnZone(const ZoneID& zone);
	std::vector<ClearArea> GenerateFeatures(const ZoneID& zone);
	void GenerateFoliage(const ZoneID& zone, Heightmap& heightmap, const std::vector<ClearArea>& clearAreas);

	bool HaveLocationInRange(const Feature& feature, const Vector3& pos);
	Vector3 GetRandomPointInZone(VUtils::Random::State& state, const ZoneID &zone, float range);
	Vector3 GetRandomPointInRadius(VUtils::Random::State& state, const Vector3& pos, float range);
	bool InsideClearArea(const std::vector<ClearArea>& areas, const Vector3& pos);
	bool OverlapsClearArea(const std::vector<ClearArea>& areas, const Vector3& pos, float range);

	const Feature* GetFeature(HASH_t hash);
	const Feature* GetFeature(const std::string& name);

	void PrepareFeatures(const Feature &feature);
	ZoneID GetRandomZone(VUtils::Random::State &state, float range);

	void RemoveUnplacedLocations(const Feature& feature);
	void GenerateFeature(const Feature& feature, HASH_t seed, const Vector3 &pos, const Quaternion &rot);

	void GetTerrainDelta(VUtils::Random::State& state, const Vector3& pos, float range, float& delta, Vector3& slopeDirection);

	bool IsZoneGenerated(const ZoneID& zone);

	void GenerateLocationProxy(const Feature& feature, HASH_t seed, const Vector3& pos, const Quaternion& rot);
	void GenerateZoneCtrl(const ZoneID& zone);

public:
	void PrepareAllFeatures();

	void Init();

	void Update();

	void Save(DataWriter& pkg);
	void Load(DataReader& reader, int32_t version);

	std::list<std::reference_wrapper<Feature::Instance>> GetFeatureIcons();

	// Get world height at location
	float GetGroundHeight(const Vector3& pos);

	// Get specific height information at position
	Heightmap& GetGroundData(Vector3& pos, Vector3& normal, Biome& biome, BiomeArea& biomeArea);

	// Find the nearest location
	//	Nullable
	Feature::Instance *GetNearestGeneratedFeature(const std::string& name, const Vector3& pos);

	static ZoneID WorldToZonePos(const Vector3& pos);
	static Vector3 ZoneToWorldPos(const ZoneID& zone);

	bool ZonesOverlap(const ZoneID& zone, const Vector3& areaPoint);
	bool ZonesOverlap(const ZoneID& zone, const ZoneID& areaZone);

	bool IsPeerNearby(const ZoneID& zone, OWNER_t uid);
};

IZoneManager* ZoneManager();
