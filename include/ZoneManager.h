#pragma once

#include "VUtils.h"
#include "VUtilsRandom.h"
#include "Types.h"
#include "HashUtils.h"
#include "DataReader.h"
#include "DataWriter.h"
#include "Vector.h"
#include "Quaternion.h"
#include "Prefab.h"
#include "RandomSpawn.h"

using ZoneID = Vector2s;

class Heightmap;
class Peer;

class IZoneManager {
	friend class INetManager;
	friend class IModManager;

#if VH_IS_ON(VH_ZONE_GENERATION)
	class Feature {
		friend class IModManager;

	public:
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
		//Vector3f m_interiorPosition;
		//Vector3f m_generatorPosition;
		std::string m_group;
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
		std::vector<RandomSpawn> m_randomSpawns;
		bool m_slopeRotation;
		bool m_snapToWater;
		bool m_unique;
		std::vector<Prefab::Instance> m_pieces;

		bool operator==(const Feature& other) const {
			return this->m_name == other.m_name;
		}

		class Instance {
			friend class IModManager;

		public:
			std::reference_wrapper<const Feature> m_feature;
			const Vector3f m_pos;
			//bool m_placed = false; // if m_generatedZones contains position

			Instance(const Feature& location, const Vector3f& pos)
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
		Vector3f m_center;
		float m_semiWidth;
	};

	const Prefab* LOCATION_PROXY_PREFAB = nullptr;
	const Prefab* ZONE_CTRL_PREFAB = nullptr;
#endif

public:
	static constexpr int NEAR_ACTIVE_AREA = 2;
	static constexpr int DISTANT_ACTIVE_AREA = 2;
	static constexpr int ZONE_SIZE = 64;
	static constexpr float WATER_LEVEL = 30;
	static constexpr int WORLD_RADIUS_IN_ZONES = 157;
	static constexpr int WORLD_DIAMETER_IN_ZONES = WORLD_RADIUS_IN_ZONES * 2;

	static constexpr int WORLD_RADIUS_IN_METERS = WORLD_RADIUS_IN_ZONES * ZONE_SIZE;
	static constexpr int WORLD_DIAMETER_IN_METERS = WORLD_DIAMETER_IN_ZONES * ZONE_SIZE;

private:
#if VH_IS_ON(VH_ZONE_GENERATION)
	// All Features within a world capable of generation
	std::vector<std::unique_ptr<const Feature>> m_features;

	// All Features within a world hashed by name
	//UNORDERED_MAP_t<HASH_t, std::reference_wrapper<const Feature>> m_featuresByHash;

	// All Foliage within a world capable of generation
	std::vector<std::unique_ptr<const Foliage>> m_foliage;

	// All the generated Features in a world
	UNORDERED_MAP_t<ZoneID, std::unique_ptr<Feature::Instance>> m_generatedFeatures;

	// Which Zones have already been generated
	UNORDERED_SET_t<ZoneID> m_generatedZones;
#else
	/*
	enum class SigFeature : uint8_t {
		SPAWN,
		HALDOR,
		EIKTHYR,
		ELDER,
		BONEMASS,
		MODER,
		YAGLUTH,
		QUEEN
	};*/

	//std::array<std::string> m_features = {"StartTemple", };

	static constexpr std::array<const char*, 8> m_features = {
		"StartTemple",
		"Vendor_BlackForest",
		"Eikthyrnir",
		"GDKing",
		"Bonemass",
		"Dragonqueen",
		"GoblinKing",
		"DvergrBoss"
	};

	UNORDERED_MAP_t<ZoneID, std::pair<uint8_t, Vector3f>> m_generatedFeatures;
#endif

	// Game-state global keys
	UNORDERED_SET_t<std::string, ankerl::unordered_dense::string_hash, std::equal_to<>> m_globalKeys;

private:
	void SendGlobalKeys();
	void SendGlobalKeys(Peer& peer);

#if VH_IS_ON(VH_ZONE_GENERATION)
	void SendLocationIcons();
#endif
	void SendLocationIcons(Peer& peer);

	void OnNewPeer(Peer& peer);

#if VH_IS_ON(VH_ZONE_GENERATION)
	void TryGenerateNearbyZones(Vector3f pos);


	// Generate a zone if it is not already generated
	//	Returns whether the zone was successfully generated
	bool GenerateZone(ZoneID zone);
	// Generate a zone if it is not already geenrated
	//	Returns if zone was successfully generated given heightmap is ready
	bool TryGenerateZone(ZoneID zone);
	void PopulateZone(Heightmap& heightmap);
	std::vector<ClearArea> TryGenerateFeature(ZoneID zone);
	void PopulateFoliage(Heightmap& heightmap, const std::vector<ClearArea>& clearAreas);

	bool HaveLocationInRange(const Feature& feature, Vector3f pos);
	Vector3f GetRandomPointInZone(VUtils::Random::State& state, ZoneID zone, float range);
	Vector3f GetRandomPointInRadius(VUtils::Random::State& state, Vector3f pos, float range);
	bool InsideClearArea(const std::vector<ClearArea>& areas, Vector3f pos);
	bool OverlapsClearArea(const std::vector<ClearArea>& areas, Vector3f pos, float range);

	const Feature* GetFeature(std::string_view name);

	void PrepareFeatures(const Feature& feature);
	ZoneID GetRandomZone(VUtils::Random::State& state, float range);

	void RemoveUngeneratedFeatures(const Feature& feature);
	void GenerateFeature(const Feature& feature, HASH_t seed, Vector3f pos, Quaternion rot);

	void GetTerrainDelta(VUtils::Random::State& state, Vector3f pos, float range, float& delta, Vector3f& slopeDirection);

	bool IsZoneGenerated(ZoneID zone);

	void GenerateLocationProxy(const Feature& feature, HASH_t seed, Vector3f pos, Quaternion rot);
#endif

public:
	void PostPrefabInit();
	void Update();

#if VH_IS_ON(VH_ZONE_GENERATION)
	void PostGeoInit();
#endif

	void Save(DataWriter& pkg);
	// Load the world from file with a given version
	//	if resilient is true, ZoneManager will begin 
	//	loading from a specific point in file
	// Useful for working around badly formatted worlds
	void Load(DataReader& reader, int32_t version, bool resilient);
	//void ResilientLoad(DataReader& reader, int32_t version);

	auto& GlobalKeys() {
		return m_globalKeys;
	}

	//void RegenerateZone(ZoneID zone);

#if VH_IS_ON(VH_ZONE_GENERATION)
	void PopulateZone(ZoneID zone);

	// Get the client based icons for minimap
	std::list<std::reference_wrapper<Feature::Instance>> GetFeatureIcons();

	// Get world height at location
	float GetGroundHeight(Vector3f pos);

	// Get specific height information at position
	Heightmap& GetGroundData(Vector3f& pos, Vector3f& normal, Biome& biome, BiomeArea& biomeArea);

	// Find the nearest location
	//	Nullable
	Feature::Instance* GetNearestFeature(std::string_view name, Vector3f pos);
#else
	bool GetNearestFeature(std::string_view name, Vector3f in, Vector3f &out);
#endif

	static ZoneID WorldToZonePos(Vector3f pos);
	static Vector3f ZoneToWorldPos(ZoneID zone);

	bool ZonesOverlap(ZoneID zone, Vector3f areaPoint);
	bool ZonesOverlap(ZoneID zone, ZoneID areaZone);

	bool IsPeerNearby(ZoneID zone, OWNER_t uid);
};

// Manager class for everything related to world generation
IZoneManager* ZoneManager();
