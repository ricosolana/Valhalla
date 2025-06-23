#pragma once

#include <cmath>
#include <cstdint>
#include <list>
#include <string>

#include "DataStream.h"
#include "Prefab.h"
#include "Quaternion.h"
#include "Types.h"
#include "Vector.h"
#include "VUtils.h"
#include "VUtilsRandom.h"

enum class GlobalKey
{
    PlayerDamage,
    EnemyDamage,
    WorldLevel,
    EventRate,
    ResourceRate,
    StaminaRate,
    MoveStaminaRate,
    StaminaRegenRate,
    SkillGainRate,
    SkillReductionRate,
    EnemySpeedSize,
    PlayerEvents,
    Fire,
    DeathKeepEquip,
    DeathDeleteItems,
    DeathDeleteUnequipped,
    DeathSkillsReset,
    NoBuildCost,
    NoCraftCost,
    AllPiecesUnlocked,
    NoWorkbench,
    AllRecipesUnlocked,
    WorldLevelLockedTools,
    PassiveMobs,
    NoMap,
    NoPortals,
    NoBossPortals,
    DungeonBuild,
    TeleportAll,
    Preset,
    NonServerOption,// ?
    defeated_eikthyr,
    defeated_dragon,
    defeated_goblinking,
    defeated_gdking,
    defeated_bonemass,
    activeBosses,
    KilledTroll,
    killed_surtling,
    KilledBat,
    MAX
};

using ZoneID = Vector2s;

class Heightmap;
class Peer;

class IZoneManager
{
    friend class INetManager;
    friend class IScriptManager;

  public:
#if AVL_IS_ON(AVL_ZONE_GENERATION)
    class Feature
    {
      public:
        std::string m_name;
        avledet::util::Hash m_hash;

        avledet::util::Biome m_biome;
        avledet::util::BiomeArea m_biomeArea = avledet::util::BiomeArea::Everything;
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
        std::int32_t m_spawnAttempts;// 200000 or 100000 depending on priority
        std::int32_t m_quantity;
        bool m_randomRotation = true;
        //std::vector<RandomSpawn> m_randomSpawns;
        bool m_slopeRotation;
        bool m_snapToWater;
        bool m_unique;
        std::vector<Prefab::Instance> m_pieces;

        bool operator==(Feature const &other) const
        {
            return this->m_name == other.m_name;
        }

        class Instance
        {
            friend class IScriptManager;

          public:
            std::reference_wrapper<Feature const> m_feature;
            Vector3f const m_pos;

            //bool m_placed = false; // if m_generatedZones contains position

            Instance(Feature const &location, Vector3f const &pos) :
                m_feature(location),
                m_pos(pos)
            {
            }
        };
    };

    // Rename this to VegetationFeature
    class Foliage
    {
      public:
        Prefab const *m_prefab = nullptr;

        avledet::util::Biome m_biome         = avledet::util::Biome::None;
        avledet::util::BiomeArea m_biomeArea = avledet::util::BiomeArea::Everything;
        float m_radius                       = 0;// My custom impl
        float m_min                          = 0;
        float m_max                          = 10;
        float m_minTilt                      = 0;
        float m_maxTilt                      = 90;
        float m_groupRadius                  = 0;
        bool m_forcePlacement                = false;
        std::int32_t m_groupSizeMin          = 1;
        std::int32_t m_groupSizeMax          = 1;
        float m_scaleMin                     = 1;
        float m_scaleMax                     = 1;
        float m_randTilt                     = 0;
        bool m_blockCheck                    = true;
        float m_minAltitude                  = -1000;
        float m_maxAltitude                  = 1000;
        float m_minOceanDepth                = 0;
        float m_maxOceanDepth                = 0;
        float m_terrainDeltaRadius           = 0;
        float m_minTerrainDelta              = 0;
        float m_maxTerrainDelta              = 2;
        bool m_inForest                      = false;
        float m_forestTresholdMin            = 0;
        float m_forestTresholdMax            = 1;
        bool m_snapToWater                   = false;
        bool m_snapToStaticSolid             = false;
        float m_groundOffset                 = 0;
        float m_chanceToUseGroundTilt        = 0;
        float m_minVegetation                = 0;
        float m_maxVegetation                = 0;
    };

    struct ClearArea
    {
        Vector3f m_center;
        float m_semiWidth;
    };

    Prefab const *LOCATION_PROXY_PREFAB = nullptr;
    Prefab const *ZONE_CTRL_PREFAB      = nullptr;
#endif

    static constexpr int NEAR_ZRADIUS    = 2;
    static constexpr int DISTANT_ZRADIUS = 2;
    static constexpr int UNITS_PER_ZONE  = 64;
    static constexpr float WATER_LEVEL   = 30;

    // INNER will be the fixed array
    //static constexpr int WORLD_INNER_ZRADIUS = 118 ((10500)/UNITS_PER_ZONE) + 1; //(164+3)×sin(45)
    static constexpr int WORLD_INNER_ZRADIUS   = 10500 / UNITS_PER_ZONE;
    static constexpr int WORLD_INNER_ZDIAMETER = WORLD_INNER_ZRADIUS * 2;

    //static constexpr int WORLD_MAX_ZRADIUS = WORLD_INNER_ZRADIUS + NEAR_ZRADIUS + DISTANT_ZRADIUS + /*the +1 is for error*/ 1;
    //static constexpr int WORLD_MAX_ZDIAMETER = WORLD_MAX_ZRADIUS * 2;

    //static constexpr int WORLD_MAX_RADIUS = WORLD_MAX_ZRADIUS * UNITS_PER_ZONE;
    //static constexpr int WORLD_MAX_DIAMETER = WORLD_MAX_ZDIAMETER * UNITS_PER_ZONE;

    bool is_inside_world_radius(ZoneID const &zone)
    {
        return (zone.x * zone.x + zone.y * zone.y
                < IZoneManager::WORLD_INNER_ZRADIUS * IZoneManager::WORLD_INNER_ZRADIUS);
    }

  private:
#if AVL_IS_ON(AVL_ZONE_GENERATION)
    // All Features within a world capable of generation
    std::vector<std::unique_ptr<Feature const>> m_features;

    // All Features within a world hashed by name
    //avledet::util::Map<avledet::util::Hash, std::reference_wrapper<const Feature>> m_featuresByHash;

    // All Foliage within a world capable of generation
    std::vector<std::unique_ptr<Foliage const>> m_foliage;

    // All the generated Features in a world
    avledet::util::Map<ZoneID, std::unique_ptr<Feature::Instance>> m_generatedFeatures;

    // Which Zones have already been generated
    avledet::util::Set<ZoneID> m_generatedZones;
#else
    /*
	enum class SigFeature : std::uint8_t {
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

    static constexpr std::array<char const *, 8> m_features
            = {"StartTemple", "Vendor_BlackForest", "Eikthyrnir", "GDKing",
               "Bonemass",    "Dragonqueen",        "GoblinKing", "DvergrBoss"};

    avledet::util::Map<ZoneID, std::pair<std::uint8_t, Vector3f>> m_generatedFeatures;
#endif

    // Game-state global keys
    avledet::util::Set<std::string, ankerl::unordered_dense::string_hash, std::equal_to<>> m_globalKeys;

  private:
    void SendGlobalKeys();
    void SendGlobalKeys(Peer &peer);

#if AVL_IS_ON(AVL_ZONE_GENERATION)
    void SendLocationIcons();
#endif
    void SendLocationIcons(Peer &peer);

    void OnNewPeer(Peer &peer);

#if AVL_IS_ON(AVL_ZONE_GENERATION)
    void TryGenerateNearbyZones(Vector3f pos);


    // Generate a zone if it is not already generated
    //	Returns whether the zone was successfully generated
    bool GenerateZoneBlocking(ZoneID zone);
    // Generate a zone if it is not already geenrated
    //	Returns if zone was successfully generated given heightmap is ready
    bool TryPollGenerateZone(ZoneID zone);
    void PopulateZone(Heightmap &heightmap);
    std::vector<ClearArea> TryGenerateFeature(ZoneID zone);
    void PopulateFoliage(Heightmap &heightmap, std::vector<ClearArea> const &clearAreas);

    bool HaveLocationInRange(Feature const &feature, Vector3f pos);
    Vector3f GetRandomPointInZone(VUtils::Random::State &state, ZoneID zone, float range);
    Vector3f GetRandomPointInRadius(VUtils::Random::State &state, Vector3f pos, float range);
    bool InsideClearArea(std::vector<ClearArea> const &areas, Vector3f pos);
    bool OverlapsClearArea(std::vector<ClearArea> const &areas, Vector3f pos, float range);

    Feature const *GetFeature(std::string_view name);

    void PrepareFeatures(Feature const &feature);
    ZoneID GetRandomZone(VUtils::Random::State &state, float range);

    void RemoveUngeneratedFeatures(Feature const &feature);
    void GenerateFeature(Feature const &feature, avledet::util::Hash seed, Vector3f pos, Quaternion rot);

    void GetTerrainDelta(VUtils::Random::State &state, Vector3f pos, float range, float &delta,
                         Vector3f &slopeDirection);

    bool IsZoneGenerated(ZoneID zone);

    void GenerateLocationProxy(Feature const &feature, avledet::util::Hash seed, Vector3f pos,
                               Quaternion rot);
#endif

  public:
    void PostPrefabInit();
    void Update();

#if AVL_IS_ON(AVL_ZONE_GENERATION)
    void PostGeoInit();
#endif

    void Save(DataWriter &pkg);
    void Load(DataReader &reader, std::int32_t version);

    auto &GlobalKeys()
    {
        return m_globalKeys;
    }

    //void RegenerateZone(ZoneID zone);

#if AVL_IS_ON(AVL_ZONE_GENERATION)
    void PopulateZone(ZoneID zone);

    // Get the client based icons for minimap
    std::list<std::reference_wrapper<Feature::Instance>> GetFeatureIcons();

    // Get world height at location
    float GetGroundHeight(Vector3f pos);

    // Get specific height information at position
    Heightmap &GetGroundData(Vector3f &pos, Vector3f &normal, avledet::util::Biome &biome,
                             avledet::util::BiomeArea &biomeArea);

    // Find the nearest location
    //	Nullable
    Feature::Instance *GetNearestFeature(std::string_view name, Vector3f pos);
#else
    bool GetNearestFeature(std::string_view name, Vector3f in, Vector3f &out);
#endif

    static ZoneID WorldToZonePos(Vector3f pos);
    static Vector3f ZoneToWorldPos(ZoneID zone);

    bool ZonesOverlap(ZoneID zone, Vector3f areaPoint);
    bool ZonesOverlap(ZoneID zone, ZoneID areaZone);

    bool IsPeerNearby(ZoneID zone, avledet::util::UserID uid);
};

// Manager class for everything related to world generation
IZoneManager *ZoneManager();
