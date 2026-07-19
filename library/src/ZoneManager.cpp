#include "RandomSpawn.h"
#include "Types.h"
#include "Vector.h"
#include "VUtils.h"

#include "DungeonGenerator.h"
#include "DungeonManager.h"
#include "GeoManager.h"
#include "Hashes.h"
#include "HeightMap.h"
#include "HeightmapManager.h"
#include "NetManager.h"
#include "PrefabManager.h"
#include "RouteManager.h"
#include "VUtilsResource.h"
#include "ZDOManager.h"
#include "ZoneManager.h"

auto ZONE_MANAGER = std::make_unique<IZoneManager>();// TODO stop constructing in global

IZoneManager *ZoneManager()
{
    return ZONE_MANAGER.get();
}

// private
void IZoneManager::PostPrefabInit()
{
    LOG_NOTICE(AVL_LOGGER, "Initializing ZoneManager");

    {
#if AVL_IS_ON(AVL_ZONE_GENERATION)
        // load ZoneLocations:
        auto opt = VUtils::Resource::ReadFile<avledet::util::Bytes>("features.pkg");
        if (!opt)
            throw std::runtime_error("features.pkg missing");

        DataReader pkg(std::move(*opt));

        auto comment = pkg.read<std::string_view>();// comment
        LOG_DEBUG(AVL_LOGGER, "pkg comment: {}", comment);

        auto ver = pkg.read<std::string_view>();
        if (ver != VConstants::GAME) {
            LOG_WARNING(AVL_LOGGER, "features.pkg uses different game version than server ({})", ver);
        }

        auto count = pkg.read<std::int32_t>();
        for (int i = 0; i < count; i++) {
            // TODO read zoneLocations from file
            auto loc = std::make_unique<Feature>();

            loc->m_name              = pkg.read<std::string>();
            loc->m_hash              = avledet::util::get_stable_hash(loc->m_name);
            loc->m_biome             = (avledet::util::Biome) pkg.read<std::int32_t>();
            loc->m_biomeArea         = (avledet::util::BiomeArea) pkg.read<std::int32_t>();
            loc->m_applyRandomDamage = pkg.read<bool>();
            loc->m_centerFirst       = pkg.read<bool>();
            loc->m_clearArea         = pkg.read<bool>();
            //loc->m_useCustomInteriorTransform = pkg.read<bool>();
            loc->m_exteriorRadius    = pkg.read<float>();
            loc->m_interiorRadius    = pkg.read<float>();
            loc->m_forestTresholdMin = pkg.read<float>();
            loc->m_forestTresholdMax = pkg.read<float>();
            //loc->m_interiorPosition = pkg.read<Vector3f>();
            //loc->m_generatorPosition = pkg.read<Vector3f>();
            loc->m_group                  = pkg.read<std::string>();
            loc->m_iconAlways             = pkg.read<bool>();
            loc->m_iconPlaced             = pkg.read<bool>();
            loc->m_inForest               = pkg.read<bool>();
            loc->m_minAltitude            = pkg.read<float>();
            loc->m_maxAltitude            = pkg.read<float>();
            loc->m_minDistance            = pkg.read<float>();
            loc->m_maxDistance            = pkg.read<float>();
            loc->m_minTerrainDelta        = pkg.read<float>();
            loc->m_maxTerrainDelta        = pkg.read<float>();
            loc->m_minDistanceFromSimilar = pkg.read<float>();
            loc->m_spawnAttempts          = pkg.read<std::int32_t>();
            loc->m_quantity               = pkg.read<std::int32_t>();
            loc->m_randomRotation         = pkg.read<bool>();
            loc->m_slopeRotation          = pkg.read<bool>();
            loc->m_snapToWater            = pkg.read<bool>();
            loc->m_unique                 = pkg.read<bool>();

            // Netview game objects
            auto views = pkg.read<std::int32_t>();
            for (int j = 0; j < views; j++) {
                Prefab::Instance piece;

                auto dbgName = pkg.read<std::string_view>();//For debugging
                (void) dbgName;

                piece.m_prefabHash = pkg.read<avledet::util::Hash>();
                assert(avledet::util::get_stable_hash(dbgName)
                       == piece.m_prefabHash);//TODO dont use debug assert
                piece.m_pos = pkg.read<Vector3f>();
                piece.m_rot = pkg.read<Quaternion>();

                piece.get_prefab();

                loc->m_pieces.push_back(piece);
            }

            // RandomSpawn defs/refs
            loc->m_random_spawns = avledet::gen::RandomSpawn::parse_list(pkg);

            //m_featuresByHash.insert({ loc->m_hash, *loc.get() });
            m_features.push_back(std::move(loc));
        }

        LOG_NOTICE(AVL_LOGGER, "Loaded {} features", count);
#endif
    }

    {
#if AVL_IS_ON(AVL_ZONE_GENERATION)
        // load Foliage:
        auto opt = VUtils::Resource::ReadFile<avledet::util::Bytes>("vegetation.pkg");
        if (!opt)
            throw std::runtime_error("vegetation.pkg missing");

        DataReader pkg(std::move(*opt));

        auto comment = pkg.read<std::string_view>();// comment
        LOG_DEBUG(AVL_LOGGER, "pkg comment: {}", comment);

        auto ver = pkg.read<std::string_view>();
        if (ver != VConstants::GAME) {
            LOG_WARNING(AVL_LOGGER, "vegetation.pkg uses different game version than server ({})", ver);
        }

        auto count = pkg.read<std::int32_t>();
        for (int i = 0; i < count; i++) {
            auto prefabName = pkg.read<std::string>();

            auto veg = std::make_unique<Foliage>(PrefabManager()->get_prefab(prefabName));

            //veg->m_prefab = PrefabManager()->get_prefab(prefabName);

            veg->m_biome                 = (avledet::util::Biome) pkg.read<std::int32_t>();
            veg->m_biomeArea             = (avledet::util::BiomeArea) pkg.read<std::int32_t>();
            veg->m_radius                = pkg.read<float>();
            veg->m_min                   = pkg.read<float>();
            veg->m_max                   = pkg.read<float>();
            veg->m_minTilt               = pkg.read<float>();
            veg->m_maxTilt               = pkg.read<float>();
            veg->m_groupRadius           = pkg.read<float>();
            veg->m_forcePlacement        = pkg.read<bool>();
            veg->m_groupSizeMin          = pkg.read<std::int32_t>();
            veg->m_groupSizeMax          = pkg.read<std::int32_t>();
            veg->m_scaleMin              = pkg.read<float>();
            veg->m_scaleMax              = pkg.read<float>();
            veg->m_randTilt              = pkg.read<float>();
            veg->m_blockCheck            = pkg.read<bool>();
            veg->m_minAltitude           = pkg.read<float>();
            veg->m_maxAltitude           = pkg.read<float>();
            veg->m_minOceanDepth         = pkg.read<float>();
            veg->m_maxOceanDepth         = pkg.read<float>();
            veg->m_terrainDeltaRadius    = pkg.read<float>();
            veg->m_minTerrainDelta       = pkg.read<float>();
            veg->m_maxTerrainDelta       = pkg.read<float>();
            veg->m_inForest              = pkg.read<bool>();
            veg->m_forestTresholdMin     = pkg.read<float>();
            veg->m_forestTresholdMax     = pkg.read<float>();
            veg->m_snapToWater           = pkg.read<bool>();
            veg->m_snapToStaticSolid     = pkg.read<bool>();
            veg->m_groundOffset          = pkg.read<float>();
            veg->m_chanceToUseGroundTilt = pkg.read<float>();
            veg->m_minVegetation         = pkg.read<float>();
            veg->m_maxVegetation         = pkg.read<float>();

            m_foliage.push_back(std::move(veg));
        }

        LOG_NOTICE(AVL_LOGGER, "Loaded {} vegetation", count);
#endif
    }

#if AVL_IS_ON(AVL_ZONE_GENERATION)
    ZONE_CTRL_PREFAB      = PrefabManager()->find_prefab(avledet::util::hashes::Object::_ZoneCtrl);
    LOCATION_PROXY_PREFAB = PrefabManager()->find_prefab(avledet::util::hashes::Object::LocationProxy);

    if (!ZONE_CTRL_PREFAB || !LOCATION_PROXY_PREFAB)
        throw std::runtime_error("prefabs missing");
#endif

    RouteManager()->Register(avledet::util::hashes::Routed::C2S_SetGlobalKey,
                             [this](Peer::Ptr peer, std::string_view name) {
                                 (void) peer;
                                 // TODO limit keys based on peer and the creature killed
                                 //  have this as a compiler macro
                                 if (m_globalKeys.insert(name).second)
                                     SendGlobalKeys();// Notify clients
                             });

    RouteManager()->Register(avledet::util::hashes::Routed::C2S_RemoveGlobalKey,
                             [this](Peer::Ptr peer, std::string_view name) {
                                 (void) peer;
                                 // TODO limit keys based on peer and the creature killed
                                 if (m_globalKeys.erase(name))
                                     SendGlobalKeys();// Notify clients
                             });

    RouteManager()->Register(avledet::util::hashes::Routed::C2S_RequestIcon,
                             [this](Peer::Ptr peer, std::string_view locationName, Vector3f point,
                                    std::string_view pinName, int pinType, bool showMap, bool discoverAll) {
                                    // TODO discoverAll used by certain vesvigirs (which ones?)
#if AVL_IS_ON(AVL_ZONE_GENERATION)
                                 if (auto &&instance = find_nearest_feature(locationName, point)) {
                                     LOG_INFO(AVL_LOGGER, "Found location: '{}'", locationName);
                                     RouteManager()->Invoke(peer->GetUserID(),
                                                            avledet::util::hashes::Routed::S2C_ResponseIcon,
                                                            pinName, pinType, instance->m_pos, showMap);
                                 } else {
                                     LOG_INFO(AVL_LOGGER, "Failed to find location: '{}'", locationName);
                                 }
#else
        Vector3f out;
        if (find_nearest_feature(locationName, point, out)) {
            LOG_INFO(AVL_LOGGER, "Found location: '{}'", locationName);
            RouteManager()->Invoke(peer->m_uuid,
                avledet::util::hashes::Routed::S2C_ResponseIcon,
                pinName,
                pinType,
                out,
                showMap
            );
        }
        else {
            LOG_INFO(AVL_LOGGER, "Failed to find location: '{}'", locationName);
        }
#endif
                             });

    RouteManager()->Register(avledet::util::hashes::Routed::S2C_ResponsePing, [](Peer::Ptr peer, float time) {
        peer->Route(avledet::util::hashes::Routed::Pong, time);
    });
}

// private
void IZoneManager::OnNewPeer(Peer::Ptr peer)
{
    SendGlobalKeys(peer);
    SendLocationIcons(peer);
}

bool IZoneManager::ZonesOverlap(ZoneID zone, Vector3f refPoint)
{
    return ZonesOverlap(zone, WorldToZonePos(refPoint));
}

bool IZoneManager::ZonesOverlap(ZoneID zone, ZoneID refCenterZone)
{
    int num = NEAR_ZRADIUS - 1;
    return zone.x >= refCenterZone.x - num && zone.x <= refCenterZone.x + num
           && zone.y <= refCenterZone.y + num && zone.y >= refCenterZone.y - num;
}

bool IZoneManager::IsPeerNearby(ZoneID zone, avledet::util::UserID uid)
{
    auto &&peer = NetManager()->FindPeerByUserID(uid);
    //assert((peer && uid) || (!peer && uid)); // makes sure no peer is ever found with 0 uid
    if (peer)
        return ZonesOverlap(zone, peer->m_pos);
    return false;
}

// private
void IZoneManager::SendGlobalKeys()
{
    RouteManager()->InvokeAll(avledet::util::hashes::Routed::S2C_UpdateKeys, m_globalKeys);
}

void IZoneManager::SendGlobalKeys(Peer::Ptr peer)
{
    //RouteManager()->Invoke(peer, avledet::util::hashes::Routed::S2C_UpdateKeys, m_globalKeys);
    peer->Route(avledet::util::hashes::Routed::S2C_UpdateKeys, m_globalKeys);
}

#if AVL_IS_ON(AVL_ZONE_GENERATION)
// private
void IZoneManager::SendLocationIcons()
{
    DataWriter writer;

    auto &&icons = GetFeatureIcons();

    writer.write((std::uint32_t) icons.size());
    for (auto &&instance : icons) {
        writer.write(instance.get().m_pos);
        writer.write(std::string_view(instance.get().m_feature.get().m_name));
    }

    RouteManager()->InvokeAll(avledet::util::hashes::Routed::S2C_UpdateIcons, writer.release());
}
#endif

// private
void IZoneManager::SendLocationIcons(Peer::Ptr peer)
{
    LOG_NOTICE(AVL_LOGGER, "Sending location icons to {}", peer->m_name);

#if AVL_IS_ON(AVL_ZONE_GENERATION)
    DataWriter writer;

    auto &&icons = GetFeatureIcons();

    writer.write((std::int32_t) icons.size());
    for (auto &&instance : icons) {
        writer.write(instance.get().m_pos);
        writer.write(instance.get().m_feature.get().m_name);
    }

    peer->Route(avledet::util::hashes::Routed::S2C_UpdateIcons, writer.release());
#else
    peer->SubRoute(avledet::util::hashes::Routed::S2C_UpdateIcons, [this](DataWriter &writer) {
        writer.write<std::int32_t>(1);// dummy count
        for (auto &&pair : m_generatedFeatures) {
            // We only care about StartTemple
            //if (m_features[pair.second.first] == std::string_view(m_features[0])) {
            if (pair.second.first == 0) {
                writer.write(pair.second.second);           // pos
                writer.write(m_features[pair.second.first]);// name
                return;
            }
        }
        assert(false);
    });
#endif
}

// public
void IZoneManager::Save(DataWriter &pkg)
{
#if AVL_IS_ON(AVL_ZONE_GENERATION)
    pkg.write((std::int32_t) m_generatedZones.size());
    for (auto &&zone : m_generatedZones) {
        pkg.write((std::int32_t) zone.x);
        pkg.write((std::int32_t) zone.y);
    }
#else
    LOG_WARNING(AVL_LOGGER, "Saving while AVL_ZONE_GENERATION:0 is not fully portable");

    pkg.write<std::int32_t>(0);// 0 count
#endif
    pkg.write((std::int32_t) 0);// PGW
    pkg.write(VConstants::LOCATION);
    pkg.write(m_globalKeys);
    pkg.write(true);
    pkg.write((std::int32_t) m_generatedFeatures.size());
    for (auto &&pair : m_generatedFeatures) {
#if AVL_IS_ON(AVL_ZONE_GENERATION)
        auto &&inst     = pair.second;
        auto &&location = inst->m_feature.get();

        pkg.write(std::string_view(location.m_name));
        pkg.write(inst->m_pos);
        pkg.write(m_generatedZones.contains(WorldToZonePos(inst->m_pos)));
#else
        static_assert(
                std::is_same_v<Vector3f,
                               decltype(decltype(std::remove_cvref_t<decltype(pair)>::second)::second)>);

        pkg.write(m_features[pair.second.first]);
        pkg.write(pair.second.second);
        pkg.write(true);
#endif
    }
}

// public
void IZoneManager::Load(DataReader &reader, std::int32_t version)
{
    {
        auto count = reader.read<std::uint32_t>();
        for (decltype(count) i = 0; i < count; i++) {
            auto x = reader.read<std::int32_t>();
            auto y = reader.read<std::int32_t>();
#if AVL_IS_ON(AVL_ZONE_GENERATION)
            m_generatedZones.insert(ZoneID(static_cast<std::int16_t>(x), std::int16_t(y)));
#endif// AVL_ZONE_GENERATION
        }
    }

#if AVL_IS_ON(AVL_LEGACY_WORLD_LOADING)
    if (version >= 13)
#endif                              // AVL_LEGACY_WORLD_LOADING
    {
        reader.read<std::int32_t>();// PGW
        auto const locationVersion = (version >= 21) ? reader.read<std::int32_t>() : 0;// 26

#if AVL_IS_ON(AVL_LEGACY_WORLD_LOADING)
        if (version >= 14)
#endif// AVL_LEGACY_WORLD_LOADING
        {
            m_globalKeys = reader.read<decltype(m_globalKeys)>();

#if AVL_IS_ON(AVL_LEGACY_WORLD_LOADING)
            if (version >= 18)
#endif// AVL_LEGACY_WORLD_LOADING
            {
#if AVL_IS_ON(AVL_LEGACY_WORLD_LOADING)
                if (version >= 20)
#endif                                  // AVL_LEGACY_WORLD_LOADING
                {
                    reader.read<bool>();// locationsGenerated
                }

                auto count = reader.read<std::int32_t>();
                for (decltype(count) i = 0; i < count; i++) {
                    auto text = reader.read<std::string_view>();
                    auto pos  = reader.read<Vector3f>();

#if AVL_IS_ON(AVL_LEGACY_WORLD_LOADING)
                    bool generated = (version >= 19) ? reader.read<bool>() : false;
#else // !AVL_LEGACY_WORLD_LOADING
                    bool generated = reader.read<bool>();
#endif// AVL_LEGACY_WORLD_LOADING

#if AVL_IS_ON(AVL_ZONE_GENERATION)
                    auto &&location = GetFeature(text);
                    if (location) {
                        m_generatedFeatures[WorldToZonePos(pos)]
                                = std::make_unique<Feature::Instance>(*location, pos);
                    } else {
                        LOG_ERROR(AVL_LOGGER, "Unknown feature '{}'", text);
                    }
#else // !AVL_ZONE_GENERATION
                    static_assert(std::numeric_limits<std::uint8_t>::max() > m_features.size());
                    for (std::uint8_t i = 0; i < m_features.size(); i++) {
                        if (text == std::string_view(m_features[i])) {
                            // register
                            m_generatedFeatures[WorldToZonePos(pos)]
                                    = std::pair<std::uint8_t, Vector3f>(i, pos);
                            break;
                        }
                    }
#endif// AVL_ZONE_GENERATION
                }

                LOG_NOTICE(AVL_LOGGER, "Loaded {}/{} feature instances ", m_generatedFeatures.size(), count);

                if (locationVersion != VConstants::LOCATION) {
                    // regenerate features?
                    //m_generatedFeatures.clear();
                }
            }
        }
    }
}

// private
void IZoneManager::Update()
{
    ZoneScoped;

#if AVL_IS_ON(AVL_ZONE_GENERATION)
    // TODO 100ms seems a tad too frequent
    //  peers dont even move that fast
    if (VUtils::run_periodic<struct periodic_zone_generation>(100ms)) {
        for (auto &&peer : NetManager()->GetPeers()) {
            if (!peer->IsGated()) {
                // It turns out that zdos generated by whatever ids are different from packets...
                //  although almost everything is the same...
                //  So the best ACTUAL way to capture the world would be to pre-generate the entire world
                //  then disable the world generation during playback
                TryGenerateNearbyZones(peer->m_pos);
            }
        }
    }
#endif
}

/*
void IZoneManager::RegenerateZone(const ZoneID& zone) {
    for (auto&& zdo : ZDOManager()->GetZDOs(zone, 0, Prefab::Flag::NONE, Prefab::Flag::Player))
        ZDOManager()->DestroyZDO(zdo);

    //m_generatedZones.erase(zone);
    PopulateZone(HeightmapManager()->GetHeightmap(zone));
    //m_generatedZones.insert(zone);
}*/
#if AVL_IS_ON(AVL_ZONE_GENERATION)
// Rename?
void IZoneManager::TryGenerateNearbyZones(Vector3f refPoint)
{
    auto zone = WorldToZonePos(refPoint);

    // Prioritize center zone
    if (!TryPollGenerateZone(zone)) {

        // If spawning fails, spawn other neighboring zones
        auto num = NEAR_ZRADIUS + DISTANT_ZRADIUS;
        for (int z = zone.y - num; z <= zone.y + num; z++) {
            for (int x = zone.x - num; x <= zone.x + num; x++) {

                // Skip center zone
                if (x == zone.x && z == zone.y)
                    continue;

                TryPollGenerateZone(ZoneID((std::int16_t) x, (std::int16_t) z));
            }
        }
    }
}

bool IZoneManager::GenerateZoneBlocking(ZoneID zone)
{
    if (is_inside_world_radius(zone)) {
        //if ((zone.x > -WORLD_INNER_ZRADIUS && zone.y > -WORLD_INNER_ZRADIUS
        //&& zone.x < WORLD_INNER_ZRADIUS && zone.y < WORLD_INNER_ZRADIUS))
        //{
        auto &&pair = m_generatedZones.insert(zone);
        if (pair.second) {
            PopulateZone(HeightmapManager()->GetHeightmap(zone));
            return true;
        }
    }
    return false;
}

bool IZoneManager::TryPollGenerateZone(ZoneID zone)
{
    if (is_inside_world_radius(zone) && !IsZoneGenerated(zone)) {
        //if ((zone.x >= -WORLD_INNER_ZRADIUS && zone.y >= -WORLD_INNER_ZRADIUS
        //    && zone.x <= WORLD_INNER_ZRADIUS && zone.y <= WORLD_INNER_ZRADIUS)
        //    && !IsZoneGenerated(zone)) {
        if (auto heightmap = HeightmapManager()->PollHeightmap(zone)) {
            m_generatedZones.insert(zone);

            PopulateZone(*heightmap);

            return true;
        }
    }
    return false;
}

void IZoneManager::PopulateZone(Heightmap &heightmap)
{
    ZoneScoped;

    //#if AVL_IS_ON(AVL_ZONE_GENERATION)
    std::vector<ClearArea> m_tempClearAreas;

    if (AVL_SETTINGS.worldFeatures)
        m_tempClearAreas = TryGenerateFeature(heightmap.get_zone());

    if (AVL_SETTINGS.worldVegetation)
        PopulateFoliage(heightmap, m_tempClearAreas);

    if (AVL_SETTINGS.worldCreatures) {
        ZDOManager()->Instantiate(*ZONE_CTRL_PREFAB, ZoneToWorldPos(heightmap.get_zone()));
    }
    //#endif // AVL_OPTION_ENABLE_ZONE_GENERATION
}

void IZoneManager::PopulateZone(ZoneID zone)
{
    this->PopulateZone(HeightmapManager()->GetHeightmap(zone));
}

// private
Vector3f IZoneManager::GetRandomPointInRadius(VUtils::Random::State &state, Vector3f center, float radius)
{
    float f   = state.next_float() * (float) (VUtils::PI * 2.0);
    float num = state.range(0.f, radius);
    return center + Vector3f(std::sin(f) * num, 0.f, std::cos(f) * num);
}

// private
void IZoneManager::PopulateFoliage(Heightmap &heightmap, std::vector<ClearArea> const &clearAreas)
{
    auto &&zoneID = heightmap.get_zone();

    Vector3f const center = ZoneToWorldPos(zoneID);

    auto const seed = GeoManager()->GetSeed();

    //avledet::util::Biome biomes = GeoManager()->GetBiomes(center.x, center.z);

    std::vector<ClearArea> placedAreas;

    for (auto const &zoneVegetation : m_foliage) {
        // This ultimately serves as a large precheck, assuming heightmap were being used (which it no longer seems good)
        if (!heightmap.HaveBiome(zoneVegetation->m_biome))
            continue;

        // TODO make unique per vegetation instance
        // this state will be the same for all same vegetation within a given zone, in a given world
        VUtils::Random::State state(seed + zoneID.x * 4271 + zoneID.y * 9187
                                    + zoneVegetation->m_prefab.get().m_hash);

        std::int32_t num3 = 1;
        // max is used for both chance, and quantity in conjunction with min
        if (zoneVegetation->m_max < 1) {
            if (state.next_float() > zoneVegetation->m_max) {
                continue;
            }
        } else {
            num3 = state.range((std::int32_t) zoneVegetation->m_min,
                               (std::int32_t) zoneVegetation->m_max + 1);
        }

        // flag should always be true, all vegetation seem to always have a NetView
        //bool flag = zoneVegetation.m_prefab.GetComponent<ZNetView>() != null;
        float maxTilt           = std::cos(zoneVegetation->m_maxTilt * (float) (VUtils::PI / 180.0));
        float minTilt           = std::cos(zoneVegetation->m_minTilt * (float) (VUtils::PI / 180.0));
        float num6              = UNITS_PER_ZONE * .5f - zoneVegetation->m_groupRadius;
        int const spawnAttempts = zoneVegetation->m_forcePlacement ? (num3 * 50) : num3;
        std::int32_t numSpawned = 0;
        for (int i = 0; i < spawnAttempts; i++) {
            float vx = state.range(center.x - num6, center.x + num6);
            float vz = state.range(center.z - num6, center.z + num6);

            Vector3f basePos(vx, 0., vz);

            auto const groupCount
                    = state.range(zoneVegetation->m_groupSizeMin, zoneVegetation->m_groupSizeMax + 1);
            bool generated = false;
            for (std::int32_t j = 0; j < groupCount; j++) {

                Vector3f pos
                        = (j == 0) ? basePos
                                   : GetRandomPointInRadius(state, basePos, zoneVegetation->m_groupRadius);

                // random rotations

                // y rotation in degrees
                float rot_y = state.range(0, 360);
                float scale = state.range(zoneVegetation->m_scaleMin, zoneVegetation->m_scaleMax);
                // x rotation in degrees
                float rot_x = state.range(-zoneVegetation->m_randTilt, zoneVegetation->m_randTilt);
                // z rotation in degrees
                float rot_z = state.range(-zoneVegetation->m_randTilt, zoneVegetation->m_randTilt);

                // Use a method similar to clear area with rectangular regions
                //if (!zoneVegetation->m_blockCheck
                //|| !IsBlocked(vector2)) // no unity   \_(^.^)_/
                {

                    Vector3f normal;
                    avledet::util::Biome biome;
                    avledet::util::BiomeArea biomeArea;
                    Heightmap &otherHeightmap = GetGroundData(pos, normal, biome, biomeArea);

                    if (!((std::to_underlying(zoneVegetation->m_biome) & std::to_underlying(biome))
                          && (std::to_underlying(zoneVegetation->m_biomeArea)
                              & std::to_underlying(biomeArea))))
                        continue;

                    // Mistlands only
                    //  A huge amount of mistlands are angled rock spires (not part of terrain),
                    //  so vegetation spanws on top of these,
                    // I do not have a way to implement this. The client, however does, which updating
                    // objects with static physics every so often while nearby
                    //
                    //float y2 = 0;
                    //Vector3f vector4;
                    //if (zoneVegetation->m_snapToStaticSolid && GetStaticSolidHeight(vector2, y2, vector4)) {
                    //    vector2.y = y2;
                    //    vector3 = vector4;
                    //}

                    float waterDiff = pos.y - WATER_LEVEL;
                    if (waterDiff < zoneVegetation->m_minAltitude
                        || waterDiff > zoneVegetation->m_maxAltitude)
                        continue;

                    // Mistlands only
                    // TODO might be affecting mist (probably is? just a hunch)
                    if (zoneVegetation->m_minVegetation != zoneVegetation->m_maxVegetation) {
                        float vegetationMask = otherHeightmap.GetVegetationMask(pos);
                        if (vegetationMask > zoneVegetation->m_maxVegetation
                            || vegetationMask < zoneVegetation->m_minVegetation) {
                            continue;
                        }
                    }

                    if (zoneVegetation->m_minOceanDepth != zoneVegetation->m_maxOceanDepth) {
                        float oceanDepth = otherHeightmap.GetOceanDepth(pos);
                        if (oceanDepth < zoneVegetation->m_minOceanDepth
                            || oceanDepth > zoneVegetation->m_maxOceanDepth) {
                            continue;
                        }
                    }

                    if (normal.y >= maxTilt && normal.y <= minTilt) {

                        if (zoneVegetation->m_terrainDeltaRadius > 0) {
                            float num12;
                            Vector3f vector5;
                            GetTerrainDelta(state, pos, zoneVegetation->m_terrainDeltaRadius, num12, vector5);
                            if (num12 > zoneVegetation->m_maxTerrainDelta
                                || num12 < zoneVegetation->m_minTerrainDelta) {
                                continue;
                            }
                        }

                        if (zoneVegetation->m_inForest) {
                            float forestFactor = GeoManager()->GetForestFactor(pos);
                            if (forestFactor < zoneVegetation->m_forestTresholdMin
                                || forestFactor > zoneVegetation->m_forestTresholdMax) {
                                continue;
                            }
                        }

                        if (!InsideClearArea(clearAreas, pos)
                            && (zoneVegetation->m_radius == 0
                                || !OverlapsClearArea(placedAreas, pos, zoneVegetation->m_radius)))// custom
                        {

                            if (zoneVegetation->m_snapToWater)
                                pos.y = WATER_LEVEL;

                            pos.y += zoneVegetation->m_groundOffset;
                            Quaternion rotation;

                            if (zoneVegetation->m_chanceToUseGroundTilt > 0
                                && state.next_float() <= zoneVegetation->m_chanceToUseGroundTilt) {
                                auto rotation2 = Quaternion::euler(0, rot_y, 0);
                                rotation       = Quaternion::look_rotation(
                                        normal.cross(rotation2 * Vector3f::FORWARD), normal);
                            } else {
                                rotation = Quaternion::euler(rot_x, rot_y, rot_z);
                            }

                            // TODO rotation during generation are not correct
                            //  this is proven because of the correct world loaded zdos,
                            //  however new generated zone zdos are not correctly rotated

                            auto &&zdo = ZDOManager()->Instantiate(zoneVegetation->m_prefab, pos);
                            zdo->set_rotation(rotation);

                            // basically any solid objects cannot be overlapped
                            //  the exception to this rule is mist, swamp_beacon, silvervein... basically non-physical vegetation
                            if (zoneVegetation->m_radius > 0)
                                placedAreas.push_back({pos, zoneVegetation->m_radius});

                            if (scale != zoneVegetation->m_prefab.get().m_localScale.x) {
                                zdo->set_local_scale(Vector3f(scale, scale, scale), true);
                            }

                            generated = true;
                        }
                    }
                }
            }

            if (generated) {
                numSpawned++;
            }

            if (numSpawned >= num3) {
                break;
            }
        }
    }
}

// private
bool IZoneManager::InsideClearArea(std::vector<ClearArea> const &areas, Vector3f point)
{
    for (auto &&clearArea : areas) {
        if (point.x > clearArea.m_center.x - clearArea.m_semiWidth
            && point.x < clearArea.m_center.x + clearArea.m_semiWidth
            && point.z > clearArea.m_center.z - clearArea.m_semiWidth
            && point.z < clearArea.m_center.z + clearArea.m_semiWidth) {
            return true;
        }
    }
    return false;
}

bool IZoneManager::OverlapsClearArea(std::vector<ClearArea> const &areas, Vector3f point, float radius)
{
    for (auto &&area : areas) {

        float d  = VUtils::Math::sq_distance_to(point.x, point.z, area.m_center.x, area.m_center.z);
        float rd = area.m_semiWidth + radius;

        if (d < rd * rd)
            return true;
    }
    return false;
}

// private
IZoneManager::Feature const *IZoneManager::GetFeature(std::string_view name)
{
    for (auto &&feature : m_features) {
        if (feature->m_name == name)
            return feature.get();
    }
    return nullptr;
}

// public
// call from within ZNet.init or earlier...
void IZoneManager::PostGeoInit()
{
    // Will be empty if world failed to load
    if (!m_generatedFeatures.empty())
        return;

    // Crucially important Location
    // So check that it exists period
    auto &&spawnLoc = GetFeature("StartTemple");
    if (!spawnLoc)
        throw std::runtime_error("World spawnpoint missing (StartTemple)");

    if (!AVL_SETTINGS.worldFeatures) {
        LOG_WARNING(AVL_LOGGER, "Location generation is disabled");
        PrepareFeatures(*spawnLoc);
    } else {
        auto now(std::chrono::steady_clock::now());

        // Already presorted by priority
        for (auto &&loc : m_features) {
            PrepareFeatures(*loc.get());
        }

        LOG_INFO(AVL_LOGGER, "Location generation took {}s",
                 std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - now)
                         .count());
    }

    if (AVL_SETTINGS.TEST_worldPregenerate && m_generatedZones.empty()) {
        auto now(std::chrono::steady_clock::now());
        int prevCount = 0;

        LOG_WARNING(AVL_LOGGER, "Pregenerating world...");
        LOG_WARNING(AVL_LOGGER,
                    "Pregeneration takes up a lot of memory and resources during and after generation!");
        LOG_WARNING(AVL_LOGGER, "This setting is experimental and unoptimized! (I dont know why :(");
        LOG_WARNING(AVL_LOGGER, "This will take a while!");
        while (m_generatedZones.size() < WORLD_INNER_ZDIAMETER * WORLD_INNER_ZDIAMETER) {
            for (std::int16_t y = -WORLD_INNER_ZRADIUS; y < WORLD_INNER_ZRADIUS; y++) {
                for (std::int16_t x = -WORLD_INNER_ZRADIUS; x < WORLD_INNER_ZRADIUS; x++) {
                    auto zone = ZoneID(x, y);
                    if (!is_inside_world_radius(zone)) {
                        continue;
                    }

                    TryPollGenerateZone(zone);

                    if (VUtils::run_periodic<struct periodic_pregen_stats>(3s)) {
                        std::string lines = COLOR_RESET;
                        // print a cool grid
                        for (std::int16_t iy = -WORLD_INNER_ZRADIUS; iy <= WORLD_INNER_ZRADIUS; iy += 6) {
                            for (std::int16_t ix = -WORLD_INNER_ZRADIUS; ix <= WORLD_INNER_ZRADIUS; ix += 6) {
                                if (std::abs(ix - x) < 3 && std::abs(iy - y) < 3) {
                                    lines += COLOR_GOLD;
                                } else if (m_generatedZones.contains({ix, iy})) {
                                    lines += COLOR_GREEN;
                                } else {
                                    lines += COLOR_GRAY;
                                }
                                lines += "O ";
                            }
                            lines += COLOR_RESET + std::string("\n");
                        }

                        std::cout << lines << "\n";

                        //LOG_INFO(AVL_LOGGER, "Zone progress: \n{}", lines);
                        LOG_WARNING(AVL_LOGGER, "{}/{} zones generated \t({} z/s)", m_generatedZones.size(),
                                    (WORLD_INNER_ZRADIUS * 2 * WORLD_INNER_ZRADIUS * 2),
                                    ((m_generatedZones.size() - prevCount) / 3));
                        prevCount = m_generatedZones.size();
                    }
                }
            }
            // TODO; sleep to avoid busy loop?
            //  because many zones will be iterated, just waiting for generation to be successful, in the meantime taking up expensive cycles...
            //std::this_thread::sleep_for(1ms);
        }

        LOG_WARNING(AVL_LOGGER, "Pregeneration took {}s",
                    std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - now)
                            .count());
    }
}

// private
void IZoneManager::PrepareFeatures(Feature const &feature)
{
    int spawnedLocations = 0;

    // CountNrOfLocation: inlined
    for (auto &&inst : m_generatedFeatures) {
        if (inst.second->m_feature.get() == feature)// better to compare locations itself rather than name
            spawnedLocations++;
    }

    unsigned int errLocations       = 0;
    unsigned int errCenterDistances = 0;
    unsigned int errNoneBiomes      = 0;
    unsigned int errBiomeArea       = 0;
    unsigned int errAltitude        = 0;
    unsigned int errForestFactor    = 0;
    unsigned int errSimilarLocation = 0;
    unsigned int errTerrainDelta    = 0;

    VUtils::Random::State state(GeoManager()->GetSeed() + feature.m_hash);
    float const locationRadius = std::max(feature.m_exteriorRadius, feature.m_interiorRadius);

    float range = feature.m_centerFirst ? feature.m_minDistance : 10000;

    for (int a = 0; a < feature.m_spawnAttempts && spawnedLocations < feature.m_quantity; a++) {
        auto randomZone = GetRandomZone(state, range);
        if (feature.m_centerFirst)
            range++;

        if (m_generatedFeatures.contains(randomZone))
            errLocations++;
        else {
            auto zonePos                       = ZoneToWorldPos(randomZone);
            avledet::util::BiomeArea biomeArea = GeoManager()->GetBiomeArea(zonePos);

            if (!(std::to_underlying(feature.m_biomeArea) & std::to_underlying(biomeArea)))
                errBiomeArea++;
            else {
                for (int i = 0; i < 20; i++) {
                    auto randomPointInZone = GetRandomPointInZone(state, randomZone, locationRadius);

                    float magnitude = randomPointInZone.magnitude();
                    if ((feature.m_minDistance != 0 && magnitude < feature.m_minDistance)
                        || (feature.m_maxDistance != 0 && magnitude > feature.m_maxDistance)) {
                        errCenterDistances++;
                    } else {
                        avledet::util::Biome biome = GeoManager()->GetBiome(randomPointInZone);

                        if (!(std::to_underlying(biome) & std::to_underlying(feature.m_biome)))
                            errNoneBiomes++;
                        else {
                            randomPointInZone.y
                                    = GeoManager()->GetHeight(randomPointInZone.x, randomPointInZone.z);
                            float waterDiff = randomPointInZone.y - WATER_LEVEL;
                            if (waterDiff < feature.m_minAltitude || waterDiff > feature.m_maxAltitude)
                                errAltitude++;
                            else {
                                if (feature.m_inForest) {
                                    float forestFactor = GeoManager()->GetForestFactor(randomPointInZone);
                                    if (forestFactor < feature.m_forestTresholdMin
                                        || forestFactor > feature.m_forestTresholdMax) {
                                        errForestFactor++;
                                        continue;
                                    }
                                }

                                float delta = 0;
                                Vector3f vector;
                                GeoManager()->GetTerrainDelta(state, randomPointInZone,
                                                              feature.m_exteriorRadius, delta, vector);
                                if (delta > feature.m_maxTerrainDelta || delta < feature.m_minTerrainDelta)
                                    errTerrainDelta++;
                                else {
                                    if (feature.m_minDistanceFromSimilar <= 0
                                        || !HaveLocationInRange(feature, randomPointInZone)) {
                                        auto zone = WorldToZonePos(randomPointInZone);

                                        m_generatedFeatures[zone] = std::make_unique<Feature::Instance>(
                                                feature, randomPointInZone);

                                        spawnedLocations++;
                                        break;
                                    }
                                    errSimilarLocation++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (spawnedLocations < feature.m_quantity) {
        LOG_WARNING(AVL_LOGGER, "Failed to place all {}, placed {}/{}", feature.m_name, spawnedLocations,
                    feature.m_quantity);

        //LOG(ERROR) << "errLocations " << errLocations;
        //LOG(ERROR) << "errCenterDistances " << errCenterDistances;
        //LOG(ERROR) << "errNoneBiomes " << errNoneBiomes;
        //LOG(ERROR) << "errBiomeArea " << errBiomeArea;
        //LOG(ERROR) << "errAltitude " << errAltitude;
        //LOG(ERROR) << "errForestFactor " << errForestFactor;
        //LOG(ERROR) << "errSimilarLocation " << errSimilarLocation;
        //LOG(ERROR) << "errTerrainDelta " << errTerrainDelta;
    }
}

bool IZoneManager::HaveLocationInRange(Feature const &loc, Vector3f p)
{
    for (auto &&pair : m_generatedFeatures) {
        auto &&locationInstance = pair.second;
        auto &&location         = locationInstance->m_feature.get();

        if ((location == loc || (!loc.m_group.empty() && loc.m_group == location.m_group))
            && locationInstance->m_pos.distance_to(p) < loc.m_minDistanceFromSimilar)// TODO use sqdist
        {
            return true;
        }
    }
    return false;
}

Vector3f IZoneManager::GetRandomPointInZone(VUtils::Random::State &state, ZoneID zone, float locationRadius)
{
    auto pos  = ZoneToWorldPos(zone);
    float num = UNITS_PER_ZONE / 2.f;
    float x   = state.range(-num + locationRadius, num - locationRadius);
    float z   = state.range(-num + locationRadius, num - locationRadius);
    return pos + Vector3f(x, 0.f, z);
}

ZoneID IZoneManager::GetRandomZone(VUtils::Random::State &state, float range)
{
    int num = (std::int32_t) range / (std::int32_t) UNITS_PER_ZONE;
    ZoneID zone;
    do {
        float x = (float) state.range(-num, num);
        float y = (float) state.range(-num, num);
        zone    = ZoneID((std::int16_t) x, (std::int16_t) y);
    } while (ZoneToWorldPos(zone).magnitude() >= 10000);
    return zone;
}

// private
std::vector<IZoneManager::ClearArea> IZoneManager::TryGenerateFeature(ZoneID zoneID)
{
    auto now(std::chrono::steady_clock::now());

    std::vector<ClearArea> clearAreas;

    auto &&find = m_generatedFeatures.find(zoneID);
    if (find != m_generatedFeatures.end()) {
        auto &&locationInstance = find->second;
        auto &&location         = locationInstance->m_feature.get();

        Vector3f position = locationInstance->m_pos;
        //Vector3f vector;
        //avledet::util::Biome biome;
        //BiomeArea biomeArea;
        //Heightmap *heightmap = GetGroundData(position, vector, biome, biomeArea);

        // m_snapToWater is Mistlands only
        if (location.m_snapToWater)
            position.y = WATER_LEVEL;

        if (location.m_clearArea)
            clearAreas.push_back({position, location.m_exteriorRadius});

        Quaternion rot;

        // slopeRotation is Mistlands only
        //if (locationInstance.m_feature->m_slopeRotation) {
        //    float num;
        //    Vector3f vector2;
        //    GetTerrainDelta(position, locationInstance.m_feature->m_exteriorRadius, num, vector2);
        //    Vector3f forward(vector2.x, 0.f, vector2.z);
        //    forward.Normalize();
        //    rot = Quaternion::look_rotation(forward);
        //    assert(false);
        //    //Vector3f euler_angles = rot.euler_angles;
        //    //euler_angles.y = round(euler_angles.y / 22.5f) * 22.5f;
        //    //rot.euler_angles = euler_angles;
        //}

        if (location.m_randomRotation) {
            rot = Quaternion::euler(0, VUtils::Random::State().range(0, 16) * 22.5f, 0);
        }

        avledet::util::Hash seed = GeoManager()->GetSeed() + zoneID.x * 4271 + zoneID.y * 9187;
        GenerateFeature(location, seed, position, rot);

        // TODO fix quill Vector find
        LOG_INFO(AVL_LOGGER, "Placed '{}' in zone {} ({})", location.m_name, zoneID, position);

        // Remove all other Haldor locations, etc...
        if (location.m_unique) {
            RemoveUngeneratedFeatures(location);
        }

        // TODO determine whether this method requires a special Peer* method
        if (location.m_iconPlaced) {
            SendLocationIcons();
        }
    }

    return clearAreas;
}

// private
void IZoneManager::RemoveUngeneratedFeatures(Feature const &feature)
{
    int count = 0;
    for (auto &&itr = m_generatedFeatures.begin(); itr != m_generatedFeatures.end();) {
        auto &&instance     = itr->second;
        auto &&otherFeature = instance->m_feature.get();
        if (!IsZoneGenerated(WorldToZonePos(instance->m_pos)) && otherFeature == feature) {
            itr = m_generatedFeatures.erase(itr);
            count++;
        } else
            ++itr;
    }

    LOG_INFO(AVL_LOGGER, "Removed {} unplaced '{}'", count, feature.m_name);
}

// private
void IZoneManager::GenerateFeature(Feature const &location, avledet::util::Hash seed, Vector3f pos,
                                   Quaternion rot)
{
    VUtils::Random::State state(seed);

    // TODO is random damage really important?
    //  I don't think it drasically affects much
    //WearNTear.m_randomInitialDamage = location.m_feature.m_applyRandomDamage;
    //for (auto&& znetView2 : location.m_netViews) {

    for (auto &&piece : location.m_pieces) {
        // Dungeon hierarchy:
        //  Location
        //      Interior (InteriorTransform)
        //          DG_(dungeon)

        if (!(AVL_SETTINGS.dungeonsEnabled && piece.get_prefab().get().AllFlagsPresent(Prefab::Flag::DUNGEON))) {
            auto &&zdo = ZDOManager()->Instantiate(piece.m_prefabHash, pos + rot * piece.m_pos);
            zdo->set_rotation(rot * piece.m_rot);
        } else {
            auto &&dungeon = DungeonManager()->get_dungeon(piece.m_prefabHash);

            // TODO not really optional, it is required through a branch
            ZDO::optional zdo;

            if (dungeon.m_interior_position != Vector3f::ZERO) {

                ZoneID zone      = WorldToZonePos(pos);
                Vector3f zonePos = ZoneToWorldPos(zone);

                Vector3f piecePos
                        = zonePos + dungeon.m_interior_position// ( 0, 5000, 0 )
                          + dungeon.m_original_position;// minor position change (usually height and a horizontal axis)

                piecePos.y = dungeon.m_interior_position.y + pos.y;

                zdo = ZDOManager()->Instantiate(piece.m_prefabHash, piecePos);
                zdo->set_rotation(piece.m_rot);
            } else {
                zdo = ZDOManager()->Instantiate(piece.m_prefabHash, pos + rot * piece.m_pos);
                zdo->set_rotation(rot * piece.m_rot);
            }

            assert(zdo);

            // Only add real sky dungeon
            if (zdo->get_position().y > 4000)
                DungeonManager()->m_dungeonInstances.push_back(zdo->get_id());

            DungeonManager()->generate(dungeon, zdo);
        }
    }
    //WearNTear.m_randomInitialDamage = false;

    // https://www.reddit.com/r/valheim/comments/xns70u/comment/ipv77ca/?utm_source=share&utm_medium=web2x&context=3
    // https://www.reddit.com/r/valheim/comments/r6mv1q/comment/hmutgdl/?utm_source=share&utm_medium=web2x&context=3
    // LocationProxy are client-side generated models
    GenerateLocationProxy(location, seed, pos, rot);
}

// could be inlined...
// private
void IZoneManager::GenerateLocationProxy(Feature const &location, avledet::util::Hash seed, Vector3f pos,
                                         Quaternion rot)
{
    auto &&zdo = ZDOManager()->Instantiate(*LOCATION_PROXY_PREFAB, pos);
    zdo->set_rotation(rot);

    zdo->set(avledet::util::hashes::ZDO::ZoneManager::LOCATION, location.m_hash);
    zdo->set(avledet::util::hashes::ZDO::ZoneManager::SEED, seed);
}

// public
// TODO make this batch update every time a new location is added or whatever
std::list<std::reference_wrapper<IZoneManager::Feature::Instance>> IZoneManager::GetFeatureIcons()
{
    std::list<std::reference_wrapper<IZoneManager::Feature::Instance>> result;

    for (auto &&pair : m_generatedFeatures) {
        auto &&instance = pair.second;
        auto &&location = instance->m_feature.get();

        auto zone = WorldToZonePos(instance->m_pos);
        if (location.m_iconAlways || (location.m_iconPlaced && m_generatedZones.contains(zone))) {
            result.push_back(*instance.get());
        }
    }

    return result;
}

// private
void IZoneManager::GetTerrainDelta(VUtils::Random::State &state, Vector3f center, float radius, float &delta,
                                   Vector3f &slopeDirection)
{
    float num2 = std::numeric_limits<float>::min();
    float num3 = std::numeric_limits<float>::max();
    Vector3f b = center;
    Vector3f a = center;
    for (int i = 0; i < 10; i++) {
        Vector2f vector    = state.inside_unit_circle() * radius;
        Vector3f vector2   = center + Vector3f(vector.x, 0.f, vector.y);
        float groundHeight = GetGroundHeight(vector2);
        if (groundHeight < num3) {
            num3 = groundHeight;
            a    = vector2;
        }
        if (groundHeight > num2) {
            num2 = groundHeight;
            b    = vector2;
        }
    }
    delta          = num2 - num3;
    slopeDirection = (a - b).normal();
}

// used importantly for snapping and location/vegetation generation
// public
float IZoneManager::GetGroundHeight(Vector3f p)
{
    return GeoManager()->GetHeight(p.x, p.z);
}

// public
// if terrain is just heightmap,
// could easily create a wrapper and poll points where needed
Heightmap &IZoneManager::GetGroundData(Vector3f &p, Vector3f &normal, avledet::util::Biome &biome,
                                       avledet::util::BiomeArea &biomeArea)
{
    auto &&heightmap = HeightmapManager()->GetHeightmap(WorldToZonePos(p));

    heightmap.GetWorldHeight(p, p.y);

    biome     = heightmap.GetBiome(p);
    biomeArea = heightmap.GetBiomeArea();

    heightmap.GetWorldNormal(p, normal);

    return heightmap;
}

// public
IZoneManager::Feature::Instance *IZoneManager::find_nearest_feature(std::string_view name, Vector3f point)
{
    float closestDist = std::numeric_limits<float>::max();

    IZoneManager::Feature::Instance *closest = nullptr;

    for (auto &&pair : m_generatedFeatures) {
        auto &&instance = pair.second;
        auto &&location = instance->m_feature.get();

        float dist = instance->m_pos.sq_distance_to(point);
        if (location.m_name == name && dist < closestDist) {
            closestDist = dist;
            closest     = instance.get();
        }
    }

    return closest;
}

#else

// public
bool IZoneManager::find_nearest_feature(std::string_view name, Vector3f in, Vector3f &out)
{
    float sqMin = std::numeric_limits<float>::max();

    for (auto &&pair : m_generatedFeatures) {
        if (m_features[pair.second.first] == name) {
            float sq = in.sq_distance_to(pair.second.second);
            if (sq < sqMin) {
                out   = pair.second.second;
                sqMin = sq;
            }
        }
    }

    return sqMin < std::numeric_limits<float>::max();
}
#endif

// public
// this is world position to zone position
// formerly GetZone
ZoneID IZoneManager::WorldToZonePos(Vector3f point)
{
    auto x = std::floor((point.x + (float) UNITS_PER_ZONE / 2.f) / (float) UNITS_PER_ZONE);
    auto y = std::floor((point.z + (float) UNITS_PER_ZONE / 2.f) / (float) UNITS_PER_ZONE);
    return ZoneID((std::int16_t) x, (std::int16_t) y);
}

// public
// zone position to ~world position
// GetZonePos
Vector3f IZoneManager::ZoneToWorldPos(ZoneID id)
{
    return Vector3f(id.x * UNITS_PER_ZONE, 0, id.y * UNITS_PER_ZONE);
}

#if AVL_IS_ON(AVL_ZONE_GENERATION)
// private
bool IZoneManager::IsZoneGenerated(ZoneID zoneID)
{
    return m_generatedZones.contains(zoneID);
}
#endif