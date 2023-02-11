#include <thread>

#include "NetManager.h"
#include "ZoneManager.h"
#include "GeoManager.h"
#include "HeightmapBuilder.h"
#include "PrefabManager.h"
#include "RouteManager.h"
#include "VUtilsResource.h"
#include "Hashes.h"

auto ZONE_MANAGER(std::make_unique<IZoneManager>()); // TODO stop constructing in global
IZoneManager* ZoneManager() {
    return ZONE_MANAGER.get();
}



// private
void IZoneManager::Init() {
    LOG(INFO) << "Initializing ZoneManager";

    {
        // load ZoneLocations:
        auto opt = VUtils::Resource::ReadFileBytes("zoneLocations.pkg");
        if (!opt)
            throw std::runtime_error("zoneLocations.pkg missing");

        DataReader pkg(opt.value());

        pkg.Read<std::string>(); // date
        std::string ver = pkg.Read<std::string>();
        LOG(INFO) << "zoneLocations.pkg has game version " << ver;
        if (ver != VConstants::GAME)
            LOG(WARNING) << "zoneLocations.pkg uses different game version than server";

        int32_t count = pkg.Read<int32_t>();
        LOG(INFO) << "Loading " << count << " ZoneLocations";
        for (int i=0; i < count; i++) {
            // TODO read zoneLocations from file
            auto loc = std::make_unique<ZoneLocation>();
            loc->m_name = pkg.Read<std::string>();
            loc->m_hash = VUtils::String::GetStableHashCode(loc->m_name);

            //loc->m_prefab = PrefabManager()->GetPrefab(prefabName);
            //assert(loc->m_prefab && "missing zonelocation prefab");

            // ZoneLocations do not have prefab (only proxy by hash)
            //  Only their netviews do
            //if (!loc->m_prefab) {
            //    LOG(ERROR) << "vegetation missing prefab '" << loc->m_name << "'";
            //    throw std::runtime_error("vegetation missing prefab");
            //}

            loc->m_biome = (Biome)pkg.Read<int32_t>();
            loc->m_biomeArea = (BiomeArea)pkg.Read<int32_t>();
            loc->m_applyRandomDamage = pkg.Read<bool>();
            loc->m_centerFirst = pkg.Read<bool>();
            loc->m_clearArea = pkg.Read<bool>();
            loc->m_useCustomInteriorTransform = pkg.Read<bool>();
            loc->m_exteriorRadius = pkg.Read<float>();
            loc->m_interiorRadius = pkg.Read<float>();
            loc->m_forestTresholdMin = pkg.Read<float>();
            loc->m_forestTresholdMax = pkg.Read<float>();
            loc->m_interiorPosition = pkg.Read<Vector3>();
            loc->m_generatorPosition = pkg.Read<Vector3>();
            loc->m_group = pkg.Read<std::string>();
            loc->m_iconAlways = pkg.Read<bool>();
            loc->m_iconPlaced = pkg.Read<bool>();
            loc->m_inForest = pkg.Read<bool>();
            loc->m_minAltitude = pkg.Read<float>();
            loc->m_maxAltitude = pkg.Read<float>();
            loc->m_minDistance = pkg.Read<float>();
            loc->m_maxDistance = pkg.Read<float>();
            loc->m_minTerrainDelta = pkg.Read<float>();
            loc->m_maxTerrainDelta = pkg.Read<float>();
            loc->m_minDistanceFromSimilar = pkg.Read<float>();
            //loc->m_prioritized = pkg.Read<bool>();
            loc->m_spawnAttempts = pkg.Read<int32_t>();
            loc->m_quantity = pkg.Read<int32_t>();
            loc->m_randomRotation = pkg.Read<bool>();
            loc->m_slopeRotation = pkg.Read<bool>();
            loc->m_snapToWater = pkg.Read<bool>();
            loc->m_unique = pkg.Read<bool>();

            auto views = pkg.Read<int32_t>();
            for (int j=0; j < views; j++) {
                ZoneLocation::Piece piece;

                auto hash = pkg.Read<HASH_t>();

                piece.m_prefab = PrefabManager()->GetPrefab(hash);
                if (!piece.m_prefab) {
                    LOG(ERROR) << "ZoneLocation znetview missing prefab '" << loc->m_name << "' " << hash;
                    throw std::runtime_error("znetview missing prefab");
                }

                piece.m_pos = pkg.Read<Vector3>();
                piece.m_rot = pkg.Read<Quaternion>();
                loc->m_pieces.push_back(piece);
            }

            //m_locationsByHash[VUtils::String::GetStableHashCode(prefabName)] = std::move(loc);

            m_locationsByHash[loc->m_hash] = loc.get();
            m_locations.push_back(std::move(loc));

            //m_locationsByHash[loc->m_hash] = std::move(loc);

            //m_locations.push_back(std::move(loc));
        }
    }

    {
        // load ZoneVegetation:
        auto opt = VUtils::Resource::ReadFileBytes("vegetation.pkg");
        if (!opt)
            throw std::runtime_error("vegetation.pkg missing");

        DataReader pkg(opt.value());

        pkg.Read<std::string>(); // date
        std::string ver = pkg.Read<std::string>();
        LOG(INFO) << "zoneVegetation.pkg has game version " << ver;
        if (ver != VConstants::GAME)
            LOG(WARNING) << "zoneVegetation.pkg uses different game version than server";

        auto count = pkg.Read<int32_t>();
        LOG(INFO) << "Loading " << count << " ZoneVegetations";
        //while (count--) {
        for (int i=0; i < count; i++) {
            auto veg = std::make_unique<ZoneVegetation>();

            auto prefabName = pkg.Read<std::string>();

            veg->m_prefab = PrefabManager()->GetPrefab(prefabName);
            //assert(veg->m_prefab && "missing vegetation prefab");

            if (!veg->m_prefab) {
                LOG(ERROR) << "vegetation missing prefab '" << prefabName << "'";
                throw std::runtime_error("vegetation missing prefab");
            }

            veg->m_biome = (Biome) pkg.Read<int32_t>();
            veg->m_biomeArea = (BiomeArea) pkg.Read<int32_t>();
            veg->m_radius = pkg.Read<float>();
            veg->m_min = pkg.Read<float>();
            veg->m_max = pkg.Read<float>();
            veg->m_minTilt = pkg.Read<float>();
            veg->m_maxTilt = pkg.Read<float>();
            veg->m_groupRadius = pkg.Read<float>();
            veg->m_forcePlacement = pkg.Read<bool>();
            veg->m_groupSizeMin = pkg.Read<int32_t>();
            veg->m_groupSizeMax = pkg.Read<int32_t>();
            veg->m_scaleMin = pkg.Read<float>();
            veg->m_scaleMax = pkg.Read<float>();
            veg->m_randTilt = pkg.Read<float>();
            veg->m_blockCheck = pkg.Read<bool>();
            veg->m_minAltitude = pkg.Read<float>();
            veg->m_maxAltitude = pkg.Read<float>();
            veg->m_minOceanDepth = pkg.Read<float>();
            veg->m_maxOceanDepth = pkg.Read<float>();
            veg->m_terrainDeltaRadius = pkg.Read<float>();
            veg->m_minTerrainDelta = pkg.Read<float>();
            veg->m_maxTerrainDelta = pkg.Read<float>();
            veg->m_inForest = pkg.Read<bool>();
            veg->m_forestTresholdMin = pkg.Read<float>();
            veg->m_forestTresholdMax = pkg.Read<float>();
            veg->m_snapToWater = pkg.Read<bool>();
            veg->m_snapToStaticSolid = pkg.Read<bool>();
            veg->m_groundOffset = pkg.Read<float>();
            veg->m_chanceToUseGroundTilt = pkg.Read<float>();
            veg->m_minVegetation = pkg.Read<float>();
            veg->m_maxVegetation = pkg.Read<float>();

            m_vegetation.push_back(std::move(veg));
        }
    }

    ZONE_CTRL_PREFAB = PrefabManager()->GetPrefab(Hashes::Object::_ZoneCtrl);
    LOCATION_PROXY_PREFAB = PrefabManager()->GetPrefab(Hashes::Object::LocationProxy);

    if (!ZONE_CTRL_PREFAB || !LOCATION_PROXY_PREFAB)
        throw std::runtime_error("Some crucial ZoneManager prefabs failed to load");

    RouteManager()->Register(Hashes::Routed::SetGlobalKey, [this](Peer* peer, std::string name) {
        // TODO constraint check
        if (m_globalKeys.insert(name).second)
            SendGlobalKeys(IRouteManager::EVERYBODY); // Notify clients
    });

    RouteManager()->Register(Hashes::Routed::RemoveGlobalKey, [this](Peer* peer, std::string name) {
        // TODO constraint check
        if (m_globalKeys.erase(name))
            SendGlobalKeys(IRouteManager::EVERYBODY); // Notify clients
    });

    RouteManager()->Register(Hashes::Routed::DiscoverLocation, [this](Peer* peer, std::string locationName, Vector3 point, std::string pinName, int pinType, bool showMap) {
        LocationInstance inst;
        if (FindClosestLocation(locationName, point, inst)) {
            LOG(INFO) << "Found location: '" << locationName << "'";
            RouteManager()->Invoke(peer->m_uuid, Hashes::Routed::DiscoverLocationCallback, pinName, pinType, inst.m_position, showMap);
        }
        else {
            LOG(INFO) << "Failed to find location: '" << locationName << "'";
        }
    });
}

// private
void IZoneManager::OnNewPeer(Peer* peer) {
    SendGlobalKeys(peer->m_uuid);
    SendLocationIcons(peer->m_uuid);
}

bool IZoneManager::ZonesOverlap(const ZoneID& zone, const Vector3& refPoint) {
    return ZonesOverlap(zone,
        WorldToZonePos(refPoint));
}

bool IZoneManager::ZonesOverlap(const ZoneID& zone, const ZoneID& refCenterZone) {
    int num = NEAR_ACTIVE_AREA - 1;
    return zone.x >= refCenterZone.x - num
        && zone.x <= refCenterZone.x + num
        && zone.y <= refCenterZone.y + num
        && zone.y >= refCenterZone.y - num;
}

bool IZoneManager::IsInPeerActiveArea(const ZoneID& zone, OWNER_t uid) {
    auto&& peer = NetManager()->GetPeer(uid);
    if (peer) return IZoneManager::ZonesOverlap(zone, peer->m_pos);
    return false;
}

// private
void IZoneManager::SendGlobalKeys(OWNER_t peer) {
    LOG(INFO) << "Sending global keys to " << peer;
    RouteManager()->Invoke(peer, "GlobalKeys", m_globalKeys);
}

// private
void IZoneManager::SendLocationIcons(OWNER_t peer) {
    LOG(INFO) << "Sending location icons to " << peer;

    BYTES_t bytes;
    DataWriter writer(bytes);

    robin_hood::unordered_map<Vector3, std::string> icons;
    GetLocationIcons(icons);

    writer.Write<int32_t>(icons.size());
    for (auto&& keyValuePair : icons) {
        writer.Write(keyValuePair.first);
        writer.Write(keyValuePair.second);
    }

    RouteManager()->Invoke(peer, Hashes::Routed::LocationIcons, bytes);
}

// public
void IZoneManager::Save(DataWriter& pkg) {
    pkg.Write(m_generatedZones);
    pkg.Write(VConstants::PGW);
    pkg.Write(VConstants::LOCATION);
    pkg.Write(m_globalKeys);
    pkg.Write(true); // m_worldSave.Write(m_locationsGenerated);
    pkg.Write<int32_t>(m_locationInstances.size());
    for (auto&& pair : m_locationInstances) {
        auto&& inst = pair.second;
        pkg.Write(inst.m_location->m_name);
        pkg.Write(inst.m_position);
        pkg.Write(m_generatedZones.contains(WorldToZonePos(inst.m_position)));
    }
}

// public
void IZoneManager::Load(DataReader& reader, int32_t version) {
    m_generatedZones = reader.Read<robin_hood::unordered_set<Vector2i>>();

    //const auto countZones = reader.Read<int32_t>();
    //for (int i=0; i < countZones; i++) {
    //    m_generatedZones.insert(reader.Read<Vector2i>());
    //}

    if (version >= 13) {
        const auto pgwVersion = reader.Read<int32_t>(); // 99
        const auto locationVersion = (version >= 21) ? reader.Read<int32_t>() : 0; // 26
        if (pgwVersion != VConstants::PGW)
            LOG(WARNING) << "Loading unsupported pgw version";

        if (version >= 14) {
            m_globalKeys = reader.Read<robin_hood::unordered_set<std::string>>();
            //const auto countKeys = reader.Read<int32_t>();
            //for (int i=0; i < countKeys; i++) {
            //    m_globalKeys.insert(reader.Read<std::string>());
            //}

            if (version >= 18) {
                if (version >= 20) reader.Read<bool>(); // m_locationsGenerated

                const auto countLocations = reader.Read<int32_t>();
                for (int i = 0; i < countLocations; i++) {
                    auto text = reader.Read<std::string>();
                    auto pos = reader.Read<Vector3>();
                    if (version >= 19) reader.Read<bool>(); // m_placed

                    auto&& location = GetLocation(text);
                    if (location) {
                        m_locationInstances[WorldToZonePos(pos)] = { location, pos };
                    }
                    else {
                        LOG(ERROR) << "Failed to find location " << text;
                    }
                }

                LOG(INFO) << "Loaded " << countLocations << " ZoneLocation instances";
                if (pgwVersion != VConstants::PGW) {
                  m_locationInstances.clear();
                }

                // this would completely regenerate locations across modified patches
                if (locationVersion != VConstants::LOCATION) {
                    //m_locationsGenerated = false;
                }
            }
        }


    }
}

// private
void IZoneManager::Update() {
    PERIODIC_NOW(100ms, {
        auto&& peers = NetManager()->GetPeers();
        for (auto&& pair : peers) {
            auto&& peer = pair.second;
            CreateGhostZones(peer->m_pos);
        }
    });
}

// Rename?
void IZoneManager::CreateGhostZones(const Vector3& refPoint) {
    auto zone = WorldToZonePos(refPoint);

    // Prioritize center zone
    if (!SpawnZone(zone)) {

        // If spawning fails, spawn other neighboring zones
        auto num = NEAR_ACTIVE_AREA + DISTANT_ACTIVE_AREA;
        for (int z = zone.y - num; z <= zone.y + num; z++) {
            for (int x = zone.x - num; x <= zone.x + num; x++) {

                // Skip center zone
                if (x == zone.x && z == zone.y)
                    continue;

                SpawnZone({ x, z });
            }
        }
    }
}

bool IZoneManager::SpawnZone(const ZoneID& zone) {
    //Heightmap componentInChildren = m_zonePrefab.GetComponentInChildren<Heightmap>();

    // _root object is ZonePrefab, Components:
    //  WaterVolume
    //  Heightmap
    // 
    //  *note: ZonePrefab does NOT contain ZDO, nor ZNetView, unline _ZoneCtrl (which does)

    // Wait for builder thread
    if ((zone.x > -WORLD_SIZE_IN_ZONES/2 && zone.y > -WORLD_SIZE_IN_ZONES/2
        && zone.x < WORLD_SIZE_IN_ZONES/2 && zone.y < WORLD_SIZE_IN_ZONES/2)
        && !IsZoneGenerated(zone)) {
        if (auto heightmap = HeightmapManager()->PollHeightmap(zone)) {
            static std::vector<ClearArea> m_tempClearAreas;

            if (SERVER_SETTINGS.spawningLocations)
                m_tempClearAreas = PlaceLocations(zone);

            if (SERVER_SETTINGS.spawningVegetation)
                PlaceVegetation(zone, *heightmap, m_tempClearAreas);

            if (SERVER_SETTINGS.spawningCreatures)
                PlaceZoneCtrl(zone);

            m_generatedZones.insert(zone);
            return true;
        }
    }
    return false;
}

// private
void IZoneManager::PlaceZoneCtrl(const ZoneID& zone) {
    // ZoneCtrl is basically a player controlled natural mob spawner
    //  - SpawnSystem
    auto pos = ZoneToWorldPos(zone);
    PrefabManager()->Instantiate(ZONE_CTRL_PREFAB, pos);
}

// private
Vector3 IZoneManager::GetRandomPointInRadius(VUtils::Random::State& state, const Vector3& center, float radius) {
    float f = state.NextFloat() * PI * 2.f;
    float num = state.Range(0.f, radius);
    return center + Vector3(sin(f) * num, 0.f, cos(f) * num);
}

// private
void IZoneManager::PlaceVegetation(const ZoneID& zoneID, Heightmap& heightmap, const std::vector<ClearArea>& clearAreas) {
    const Vector3 center = ZoneToWorldPos(zoneID);

    const auto seed = GeoManager()->GetSeed();

    //Biome biomes = GeoManager()->GetBiomes(center.x, center.z);

    std::vector<ClearArea> placedAreas;

    for (const auto& zoneVegetation : m_vegetation) {
        // This ultimately serves as a large precheck, assuming heightmap were being used (which it no longer seems good)
        if (!heightmap.HaveBiome(zoneVegetation->m_biome))
            continue;

        // TODO make unique per vegetation instance
        // this state will be the same for all same vegetation within a given zone, in a given world
        VUtils::Random::State state(
            seed + zoneID.x * 4271 + zoneID.y * 9187 + zoneVegetation->m_prefab->m_hash
        );

        int32_t num3 = 1;
        // max is used for both chance, and quantity in conjunction with min 
        if (zoneVegetation->m_max < 1) {
            if (state.NextFloat() > zoneVegetation->m_max) {
                continue;
            }
        }
        else {
            num3 = state.Range((int32_t) zoneVegetation->m_min, (int32_t) zoneVegetation->m_max + 1);
        }

        // flag should always be true, all vegetation seem to always have a NetView
        //bool flag = zoneVegetation.m_prefab.GetComponent<ZNetView>() != null;
        float maxTilt = std::cosf(zoneVegetation->m_maxTilt * PI / 180.f);
        float minTilt = std::cosf(zoneVegetation->m_minTilt * PI / 180.f);
        float num6 = ZONE_SIZE * .5f - zoneVegetation->m_groupRadius;
        const int spawnAttempts = zoneVegetation->m_forcePlacement ? (num3 * 50) : num3;
        int32_t numSpawned = 0;
        for (int i = 0; i < spawnAttempts; i++) {
            float vx = state.Range(center.x - num6, center.x + num6);
            float vz = state.Range(center.z - num6, center.z + num6);

            Vector3 basePos(vx, 0., vz);

            const auto groupCount = state.Range(zoneVegetation->m_groupSizeMin, zoneVegetation->m_groupSizeMax + 1);
            bool generated = false;
            for (int32_t j = 0; j < groupCount; j++) {

                Vector3 pos = (j == 0) ? basePos
                    : GetRandomPointInRadius(state, basePos, zoneVegetation->m_groupRadius);

                // random rotations
                float rot_y = state.Range(0, 360);
                float scale = state.Range(zoneVegetation->m_scaleMin, zoneVegetation->m_scaleMax);
                float rot_x = state.Range(-zoneVegetation->m_randTilt, zoneVegetation->m_randTilt);
                float rot_z = state.Range(-zoneVegetation->m_randTilt, zoneVegetation->m_randTilt);

                // Use a method similar to clear area with rectangular regions
                //if (!zoneVegetation->m_blockCheck
                    //|| !IsBlocked(vector2)) // no unity   \_(^.^)_/
                {

                    //auto biome = heightmap->GetBiome(pos);
                    //auto biomeArea = heightmap->GetBiomeArea();
                    //if (!heightmap->GetWorldHeight(pos, pos.y))
                    //    throw std::runtime_error("Heightmap failed to compute height within bounds");

                    Vector3 normal;
                    Biome biome;
                    BiomeArea biomeArea;
                    Heightmap &otherHeightmap = GetGroundData(pos, normal, biome, biomeArea);

                    if (!((std::to_underlying(zoneVegetation->m_biome) & std::to_underlying(biome))
                        && (std::to_underlying(zoneVegetation->m_biomeArea) & std::to_underlying(biomeArea))))
                        continue;

                    // Mistlands only
                    //  A huge amount of mistlands are angled rock spires (not part of terrain),
                    //  so vegetation spanws on top of these,
                    // I do not have a way to implement this. The client, however does, which updating
                    // objects with static physics every so often while nearby
                    // 
                    //float y2 = 0;
                    //Vector3 vector4;
                    //if (zoneVegetation->m_snapToStaticSolid && GetStaticSolidHeight(vector2, y2, vector4)) {
                    //    vector2.y = y2;
                    //    vector3 = vector4;
                    //}

                    float waterDiff = pos.y - WATER_LEVEL;
                    if (waterDiff < zoneVegetation->m_minAltitude || waterDiff > zoneVegetation->m_maxAltitude)
                        continue;

                    // Mistlands only
                    // TODO might be affecting mist (probably is? just a hunch)
                    if (zoneVegetation->m_minVegetation != zoneVegetation->m_maxVegetation) {
                        float vegetationMask = otherHeightmap.GetVegetationMask(pos);
                        if (vegetationMask > zoneVegetation->m_maxVegetation || vegetationMask < zoneVegetation->m_minVegetation) {
                            continue;
                        }
                    }

                    if (zoneVegetation->m_minOceanDepth != zoneVegetation->m_maxOceanDepth) {
                        float oceanDepth = otherHeightmap.GetOceanDepth(pos);
                        if (oceanDepth < zoneVegetation->m_minOceanDepth || oceanDepth > zoneVegetation->m_maxOceanDepth) {
                            continue;
                        }
                    }

                    if (normal.y >= maxTilt && normal.y <= minTilt) 
                    {

                        if (zoneVegetation->m_terrainDeltaRadius > 0) {
                            float num12;
                            Vector3 vector5;
                            GetTerrainDelta(state, pos, zoneVegetation->m_terrainDeltaRadius, num12, vector5);
                            if (num12 > zoneVegetation->m_maxTerrainDelta || num12 < zoneVegetation->m_minTerrainDelta) {
                                continue;
                            }
                        }

                        if (zoneVegetation->m_inForest) {
                            float forestFactor = GeoManager()->GetForestFactor(pos);
                            if (forestFactor < zoneVegetation->m_forestTresholdMin || forestFactor > zoneVegetation->m_forestTresholdMax) {
                                continue;
                            }
                        }

                        if (!InsideClearArea(clearAreas, pos) 
                            && (zoneVegetation->m_radius == 0 || !OverlapsClearArea(placedAreas, pos, zoneVegetation->m_radius))) // custom
                        {

                            if (zoneVegetation->m_snapToWater)
                                pos.y = WATER_LEVEL;

                            pos.y += zoneVegetation->m_groundOffset;
                            Quaternion rotation = Quaternion::IDENTITY;



                            if (zoneVegetation->m_chanceToUseGroundTilt > 0
                                && state.NextFloat() <= zoneVegetation->m_chanceToUseGroundTilt) {
                                //Quaternion rotation2 = Quaternion::Euler(0.f, rot_y, 0.f);
                                //rotation = Quaternion.LookRotation(
                                //    vector3.Cross(rotation2 * Vector3::FORWARD),
                                //    vector3);
                            }
                            else {
                                rotation = Quaternion::Euler(rot_x, rot_y, rot_z);
                            }



                            // TODO modify this later once Euler implemented
                            ///rotation = Quaternion::Euler(rot_x, rot_y, rot_z);

                            // hardcoded for now
                            rotation = Quaternion(0, 1, 0, 0);

                            auto zdo = PrefabManager()->Instantiate(zoneVegetation->m_prefab, pos, rotation);

                            // basically any solid objects cannot be overlapped
                            //  the exception to this rule is mist, swamp_beacon, silvervein... basically non-physical vegetation
                            if (zoneVegetation->m_radius > 0)
                                placedAreas.push_back({ pos, zoneVegetation->m_radius });

                            if (scale != zoneVegetation->m_prefab->m_localScale.x) {
                                // this does set the Unity gameobject localscale
                                zdo->Set("scale", Vector3(scale, scale, scale));
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
bool IZoneManager::InsideClearArea(const std::vector<ClearArea>& areas, const Vector3& point) {
    for (auto&& clearArea : areas) {
        if (point.x > clearArea.m_center.x - clearArea.m_semiWidth
            && point.x < clearArea.m_center.x + clearArea.m_semiWidth
            && point.z > clearArea.m_center.z - clearArea.m_semiWidth
            && point.z < clearArea.m_center.z + clearArea.m_semiWidth) {
            return true;
        }
    }
    return false;
}

bool IZoneManager::OverlapsClearArea(const std::vector<ClearArea>& areas, const Vector3& point, float radius) {
    for (auto&& area : areas) {

        float d = VUtils::Math::SqDistance(point.x, point.z, area.m_center.x, area.m_center.z);
        float rd = area.m_semiWidth + radius;

        if (d < rd * rd)
            return true;
    }
    return false;
}

// private
const IZoneManager::ZoneLocation* IZoneManager::GetLocation(HASH_t hash) {
    auto&& find = m_locationsByHash.find(hash);
    if (find != m_locationsByHash.end())
        return find->second;

    return nullptr;
}

// private
const IZoneManager::ZoneLocation* IZoneManager::GetLocation(const std::string& name) {
    return GetLocation(VUtils::String::GetStableHashCode(name));
}

// public
// call from within ZNet.init or earlier...
void IZoneManager::GenerateLocations() {
    // Will be empty if world failed to load
    if (!m_locationInstances.empty())
        return;

    // Crucially important Location
    // So check that it exists period
    auto&& spawnLoc = m_locationsByHash.find(VUtils::String::GetStableHashCode("StartTemple"));
    if (spawnLoc == m_locationsByHash.end())
        throw std::runtime_error("unable to find StartTemple");

    if (!SERVER_SETTINGS.spawningLocations) {
        LOG(WARNING) << "Location generation is disabled";
        GenerateLocations(spawnLoc->second);
    }
    else {
        auto now(steady_clock::now());

        // Already presorted by priority
        for (auto&& loc : m_locations) {
            GenerateLocations(loc.get());
        }

        LOG(INFO) << "Location generation took " << duration_cast<seconds>(steady_clock::now() - now).count() << "s";
    }
}

// private
void IZoneManager::GenerateLocations(const ZoneLocation* location) {
    int spawnedLocations = 0;

    // CountNrOfLocation: inlined
    for (auto&& inst : m_locationInstances) {
        if (inst.second.m_location == location) // better to compare locations itself rather than name
            spawnedLocations++;
    }

    unsigned int errLocations = 0;
    unsigned int errCenterDistances = 0;
    unsigned int errNoneBiomes = 0;
    unsigned int errBiomeArea = 0;
    unsigned int errAltitude = 0;
    unsigned int errForestFactor = 0;
    unsigned int errSimilarLocation = 0;
    unsigned int errTerrainDelta = 0;

    VUtils::Random::State state(GeoManager()->GetSeed() + location->m_hash);
    const float locationRadius = std::max(location->m_exteriorRadius, location->m_interiorRadius);

    float range = location->m_centerFirst ? location->m_minDistance : 10000;

    for (int a = 0; a < location->m_spawnAttempts && spawnedLocations < location->m_quantity; a++) {
        Vector2i randomZone = GetRandomZone(state, range);
        if (location->m_centerFirst)
            range++;

        if (m_locationInstances.contains(randomZone))
            errLocations++;
        else {
            Vector3 zonePos = ZoneToWorldPos(randomZone);
            BiomeArea biomeArea = GeoManager()->GetBiomeArea(zonePos);

            if (!(std::to_underlying(location->m_biomeArea) & std::to_underlying(biomeArea)))
                errBiomeArea++;
            else {
                for (int i = 0; i < 20; i++) {
                    auto randomPointInZone = GetRandomPointInZone(state, randomZone, locationRadius);

                    float magnitude = randomPointInZone.Magnitude();
                    if ((location->m_minDistance != 0 && magnitude < location->m_minDistance)
                        || (location->m_maxDistance != 0 && magnitude > location->m_maxDistance)) {
                        errCenterDistances++;
                    } 
                    else {
                        Biome biome = GeoManager()->GetBiome(randomPointInZone);

                        if (!(std::to_underlying(biome) & std::to_underlying(location->m_biome)))
                            errNoneBiomes++;
                        else {
                            randomPointInZone.y = GeoManager()->GetHeight(randomPointInZone.x, randomPointInZone.z);
                            float waterDiff = randomPointInZone.y - WATER_LEVEL;
                            if (waterDiff < location->m_minAltitude || waterDiff > location->m_maxAltitude)
                                errAltitude++;
                            else {
                                if (location->m_inForest) {
                                    float forestFactor = GeoManager()->GetForestFactor(randomPointInZone);
                                    if (forestFactor < location->m_forestTresholdMin || forestFactor > location->m_forestTresholdMax) {
                                        errForestFactor++;
                                        continue;
                                    }
                                }

                                float delta = 0;
                                Vector3 vector;
                                GeoManager()->GetTerrainDelta(state, randomPointInZone, location->m_exteriorRadius, delta, vector);
                                if (delta > location->m_maxTerrainDelta
                                    || delta < location->m_minTerrainDelta)
                                    errTerrainDelta++;
                                else {
                                    if (location->m_minDistanceFromSimilar <= 0
                                        || !HaveLocationInRange(location, randomPointInZone)) {
                                        auto zone = WorldToZonePos(randomPointInZone);

                                        m_locationInstances[zone] = { location, randomPointInZone };
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

    if (spawnedLocations < location->m_quantity) {
        LOG(WARNING) << "Failed to place all " << location->m_name << ", placed " << spawnedLocations << "/" << location->m_quantity;

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

bool IZoneManager::HaveLocationInRange(const ZoneLocation* loc, const Vector3 &p) {
    for (auto&& pair : m_locationInstances) {
        auto&& locationInstance = pair.second;
        if ((locationInstance.m_location == loc 
            || (!loc->m_group.empty() && loc->m_group == locationInstance.m_location->m_group)) 
            && locationInstance.m_position.Distance(p) < loc->m_minDistanceFromSimilar) // TODO use sqdist
        {
            return true;
        }
    }
    return false;
}

Vector3 IZoneManager::GetRandomPointInZone(VUtils::Random::State& state, const ZoneID& zone, float locationRadius) {
    auto pos = ZoneToWorldPos(zone);
    float num = ZONE_SIZE / 2.f;
    float x = state.Range(-num + locationRadius, num - locationRadius);
    float z = state.Range(-num + locationRadius, num - locationRadius);
    return pos + Vector3(x, 0.f, z);
}

Vector2i IZoneManager::GetRandomZone(VUtils::Random::State& state, float range) {
    int num = (int32_t)range / (int32_t)ZONE_SIZE;
    Vector2i vector2i;
    do {
        float x = state.Range(-num, num);
        float y = state.Range(-num, num);
        vector2i = Vector2i(x, y);
    } while (ZoneToWorldPos(vector2i).Magnitude() >= 10000);
    return vector2i;
}

// private
std::vector<IZoneManager::ClearArea> IZoneManager::PlaceLocations(const ZoneID &zoneID)
{
    auto now(steady_clock::now());

    std::vector<ClearArea> clearAreas;

    auto&& find = m_locationInstances.find(zoneID);
    if (find != m_locationInstances.end()) {
        auto&& locationInstance = find->second;

        //assert(!locationInstance.m_placed);

        // Should ALWAYS be false
        //if (locationInstance.m_placed) {
        //    return;
        //}

        Vector3 position = locationInstance.m_position;
        //Vector3 vector;
        //Biome biome;
        //BiomeArea biomeArea;
        //Heightmap *heightmap = GetGroundData(position, vector, biome, biomeArea);

        // m_snapToWater is Mistlands only
        if (locationInstance.m_location->m_snapToWater)
            position.y = WATER_LEVEL;

        if (locationInstance.m_location->m_clearArea)
            clearAreas.push_back({position, locationInstance.m_location->m_exteriorRadius });

        Quaternion rot(Quaternion::IDENTITY);

        // slopeRotation is Mistlands only
        //if (locationInstance.m_location->m_slopeRotation) {
        //    float num;
        //    Vector3 vector2;
        //    GetTerrainDelta(position, locationInstance.m_location->m_exteriorRadius, num, vector2);
        //    Vector3 forward(vector2.x, 0.f, vector2.z);
        //    forward.Normalize();
        //    rot = Quaternion::LookRotation(forward);
        //    assert(false);
        //    //Vector3 eulerAngles = rot.eulerAngles;
        //    //eulerAngles.y = round(eulerAngles.y / 22.5f) * 22.5f;
        //    //rot.eulerAngles = eulerAngles;
        //}
        //else if (locationInstance.m_location.m_randomRotation) {
        //    //rot = Quaternion::Euler(0, (float)UnityEngine.Random.Range(0, 16) * 22.5f, 0);
        //    rot = Quaternion::Euler(0, (float)VUtils::Random::State().Range(0, 16) * 22.5f, 0);
        //}

        HASH_t seed = GeoManager()->GetSeed() + zoneID.x * 4271 + zoneID.y * 9187;
        SpawnLocation(locationInstance.m_location, seed, position, rot);
        //locationInstance.m_placed = true;

        LOG(INFO) << "Placed '" << locationInstance.m_location->m_name << "' in zone (" << zoneID.x << ", " << zoneID.y << ") at height " << position.y;

        if (locationInstance.m_location->m_unique) {
            RemoveUnplacedLocations(locationInstance.m_location);
        }

        if (locationInstance.m_location->m_iconPlaced) {
            SendLocationIcons(IRouteManager::EVERYBODY);
        }
    }

    return clearAreas;
}

// private
void IZoneManager::RemoveUnplacedLocations(const ZoneLocation* location) {
    int count = 0;
    for (auto&& itr = m_locationInstances.begin(); itr != m_locationInstances.end();) {
        if (itr->second.m_location == location) {
            itr = m_locationInstances.erase(itr);
            count++;
        }
        else ++itr;
    }

    LOG(INFO) << "Removed " << count << " unplaced '" << location->m_name << "'";
}

// private
void IZoneManager::SpawnLocation(const ZoneLocation* location, HASH_t seed, const Vector3& pos, const Quaternion& rot) {

    //location->m_prefab.transform.position = Vector3::ZERO;
    //location->m_prefab.transform.rotation = Quaternion::IDENTITY;

    //Location component = location.m_prefab.GetComponent<Location>();
    //bool flag = component.m_useCustomInteriorTransform && component.m_interiorTransform && component.m_generator;
    //bool flag = location->m_useCustomInteriorTransform && location->m_generatorPosition;
    bool flag = false;
    if (flag) {
        LOG(ERROR) << "Tried pre-initializing ZoneLocation Dungeon: " << location->m_name;
        //Vector2i zone = WorldToZonePos(pos);
        //Vector3 zonePos = ZoneToWorldPos(zone);
        //component.m_generator.transform.localPosition = Vector3::ZERO;
        //Vector3 vector = zonePos + location.m_interiorPosition + location.m_generatorPosition - pos;
        //Vector3 localPosition = (Matrix4x4.Rotate(Quaternion.Inverse(rot)) * Matrix4x4.Translate(vector)).GetColumn(3);
        //localPosition.y = component.m_interiorTransform.localPosition.y;
        //component.m_interiorTransform.localPosition = localPosition;
        //component.m_interiorTransform.localRotation = Quaternion.Inverse(rot);
    }

    // Simple Unity precondition check
    //  (not needed if I use perfect data)
    //if (component
    //    && component.m_generator
    //    && component.m_useCustomInteriorTransform != component.m_generator.m_useCustomInteriorTransform) {
    //    LOG(ERROR) << component.name << " & " + component.m_generator.name << " don't have matching m_useCustomInteriorTransform()! If one has it the other should as well!";
    //}

    //for (auto&& znetView : location.m_netViews) {
    //    znetView.gameObject.SetActive(true);
    //}

    VUtils::Random::State state(seed);

    //for (auto&& randomSpawn : location.m_randomSpawns) {
        //randomSpawn.Randomize();
    //}

    //WearNTear.m_randomInitialDamage = location.m_location.m_applyRandomDamage;
    //for (auto&& znetView2 : location.m_netViews) {
    for (auto&& piece : location->m_pieces) {
        PrefabManager()->Instantiate(piece.m_prefab, pos + rot * piece.m_pos, rot * piece.m_rot);
        //PrefabManager()->Instantiate(piece.m_prefab, pos + piece.m_pos, piece.m_rot);

        // Dungeon generation is too complex
        //DungeonGenerator component2 = gameObject.GetComponent<DungeonGenerator>();
        //if (component2) {
        //    if (flag) {
        //        component2.m_originalPosition = location.m_generatorPosition;
        //    }
        //    component2.Generate(mode);
        //}
    }
    //WearNTear.m_randomInitialDamage = false;

    // https://www.reddit.com/r/valheim/comments/xns70u/comment/ipv77ca/?utm_source=share&utm_medium=web2x&context=3
    // https://www.reddit.com/r/valheim/comments/r6mv1q/comment/hmutgdl/?utm_source=share&utm_medium=web2x&context=3
    // LocationProxy are client-side generated models that are static
    CreateLocationProxy(location, seed, pos, rot);
    //SnapToGround.SnappAll();
}

// could be inlined...
// private
void IZoneManager::CreateLocationProxy(const ZoneLocation* location, HASH_t seed, const Vector3& pos, const Quaternion& rot) {
    auto zdo = PrefabManager()->Instantiate(LOCATION_PROXY_PREFAB, pos, rot);
    
    zdo->Set("location", location->m_hash);
    zdo->Set("seed", seed);
}

// public
// TODO make this batch update every time a new location is added or whatever
void IZoneManager::GetLocationIcons(robin_hood::unordered_map<Vector3, std::string> &icons) {
    for (auto&& pair : m_locationInstances) {
        auto&& loc = pair.second;
        auto zone = WorldToZonePos(loc.m_position);
        if (loc.m_location->m_iconAlways
            || (loc.m_location->m_iconPlaced && m_generatedZones.contains(zone)))
        {
            icons[pair.second.m_position] = loc.m_location->m_name;
        }
    }
}

// private
void IZoneManager::GetTerrainDelta(VUtils::Random::State& state, const Vector3& center, float radius, float& delta, Vector3& slopeDirection) {
    float num2 = std::numeric_limits<float>::min();
    float num3 = std::numeric_limits<float>::max();
    Vector3 b = center;
    Vector3 a = center;
    for (int i = 0; i < 10; i++) {
        Vector2 vector = state.InsideUnitCircle() * radius;
        Vector3 vector2 = center + Vector3(vector.x, 0.f, vector.y);
        float groundHeight = GetGroundHeight(vector2);
        if (groundHeight < num3) {
            num3 = groundHeight;
            a = vector2;
        }
        if (groundHeight > num2) {
            num2 = groundHeight;
            b = vector2;
        }
    }
    delta = num2 - num3;
    slopeDirection = (a - b).Normalize();
}

// public
// Used by ZoneVegetation placement for blocked check
bool IZoneManager::IsBlocked(const Vector3& p) {
    throw std::runtime_error("not implemented");
    //return Physics.Raycast(p + Vector3(0, 2000, 0), Vector3::DOWN, 10000, m_blockRayMask);
}

// used importantly for snapping and location/vegetation generation
// public
float IZoneManager::GetGroundHeight(const Vector3& p) {
    return GeoManager()->GetHeight(p.x, p.z);

    //Vector3 height = p;
    //Vector3 n;
    //Biome biome;
    //BiomeArea biomeArea;
    //GetGroundData(height, n, biome, biomeArea);
    //
    //return height.y;

    //Vector3 origin = p;
    //origin.y = 6000;
    //RaycastHit raycastHit;
    //if (Physics.Raycast(origin, Vector3::DOWN, raycastHit, 10000, m_terrainRayMask)) {
    //    return raycastHit.point.y;
    //}
    //return p.y;
}

// public
// this overload seems a lot more client-based in usage
bool IZoneManager::GetGroundHeight(const Vector3& p, float& height) {
    throw std::runtime_error("not implemented");
    //p.y = 6000;
    //RaycastHit raycastHit;
    //if (Physics.Raycast(p, Vector3::DOWN, raycastHit, 10000, m_terrainRayMask)) {
    //    height = raycastHit.point.y;
    //    return true;
    //}
    //height = 0;
    //return false;
}

// ?? client only ??
// public
//float IZoneManager::GetSolidHeight(const Vector3& p) {
//    Vector3 origin = p;
//    origin.y += 1000;
//    RaycastHit raycastHit;
//    if (Physics.Raycast(origin, Vector3::DOWN, raycastHit, 2000, m_solidRayMask)) {
//        return raycastHit.point.y;
//    }
//    return p.y;
//}
//
//// public
//bool IZoneManager::GetSolidHeight(const Vector3& p, float& height, int32_t heightMargin = 1000) {
//    p.y += (float)heightMargin;
//    RaycastHit raycastHit;
//    if (Physics.Raycast(p, Vector3::DOWN, raycastHit, 2000, m_solidRayMask)
//        && !raycastHit.collider.attachedRigidbody) {
//        height = raycastHit.point.y;
//        return true;
//    }
//    height = 0;
//    return false;
//}
//
//// public
//bool IZoneManager::GetSolidHeight(const Vector3& p, float& radius, float height, Transform ignore) {
//    height = p.y - 1000;
//    p.y += 1000;
//    int32_t num;
//    if (radius <= 0) {
//        num = Physics.RaycastNonAlloc(p, Vector3::DOWN, rayHits, 2000, m_solidRayMask);
//    }
//    else {
//        num = Physics.SphereCastNonAlloc(p, radius, Vector3::DOWN, rayHits, 2000, m_solidRayMask);
//    }
//    bool result = false;
//    for (int32_t i = 0; i < num; i++) {
//        RaycastHit raycastHit = rayHits[i];
//        Collider collider = raycastHit.collider;
//        if (!(collider.attachedRigidbody != null)
//            && (!(ignore != null) || !Utils.IsParent(collider.transform, ignore))) {
//            if (raycastHit.point.y > height) {
//                height = raycastHit.point.y;
//            }
//            result = true;
//        }
//    }
//    return result;
//}

// only used by ZoneVegetation 
//  Only if snapToStaticSolid, 
//  only for Yggshoot and Magecap
// public
bool IZoneManager::GetStaticSolidHeight(const Vector3& p, float& height, const Vector3& normal) {
    throw std::runtime_error("not implemented");
    //p.y += 1000;
    //RaycastHit raycastHit;
    //if (Physics.Raycast(p, Vector3::DOWN, raycastHit, 2000, m_staticSolidRayMask)
    //    && !raycastHit.collider.attachedRigidbody) {
    //    height = raycastHit.point.y;
    //    normal = raycastHit.normal;
    //    return true;
    //}
    //height = 0;
    //normal = Vector3::ZERO;
    //return false;
}

// only used by BroodSpawner
//      by client...
// public
//bool IZoneManager::FindFloor(const Vector3& p, float& height) {
//    RaycastHit raycastHit;
//    if (Physics.Raycast(p + Vector3::UP * 1, Vector3::DOWN, raycastHit, 1000, m_solidRayMask)) {
//        height = raycastHit.point.y;
//        return true;
//    }
//    height = 0;
//    return false;
//}

// public
// if terrain is just heightmap,
// could easily create a wrapper and poll points where needed
Heightmap& IZoneManager::GetGroundData(Vector3& p, Vector3& normal, Biome& biome, BiomeArea& biomeArea) {
    //biome = Heightmap::Biome::None;
    //biomeArea = Heightmap::BiomeArea::Everything;
    //hmap = null;

    // test collision from point, casting downwards through terrain

    // If final result is completely linked to Heightmap, could just use heightmap
    // should be simple enough,

    // global heightmaps can be queried at the world--->zone then 
    // get the relative point inside zone



    //auto heightmap = HeightmapManager()->GetOrCreateHeightmap(WorldToZonePos(p));

    //auto heightmap = HeightmapManager()->GetHeightmap(p);
    //assert(heightmap && "Only call this method when Heightmap is definetly generated");

    auto &&heightmap = HeightmapManager()->GetHeightmap(WorldToZonePos(p));

    //assert(heightmap->GetWorldHeight(p, p.y) && "Should not see this error");

    //p.y =  HeightmapManager()->GetHeight(p);

    //heightmap->GetWorldHeight(p, p.y);

    //p.y = GeoManager()->GetHeight(p.x, p.z);
    heightmap.GetWorldHeight(p, p.y);
    //biome = GeoManager()->GetBiome(p.x, p.z);
    //biomeArea = GeoManager()->GetBiomeArea(p);

    biome = heightmap.GetBiome(p);
    biomeArea = heightmap.GetBiomeArea();

    heightmap.GetWorldNormal(p, normal);

    return heightmap;

    //if (heightmap) {
    //    // normal is difficult to determine
    //    biome = heightmap->
    //    p.y = HeightmapManager()->GetHeight(p);
    //    return heightmap;
    //}
    //
    //return nullptr;

    //RaycastHit raycastHit;
    //if (Physics.Raycast(p + Vector3::UP * 5000, Vector3::DOWN, raycastHit, 10000, m_terrainRayMask)) {
    //    p.y = raycastHit.point.y;
    //    normal = raycastHit.normal;
    //    Heightmap component = raycastHit.collider.GetComponent<Heightmap>();
    //    if (component) {
    //        biome = component.GetBiome(raycastHit.point);
    //        biomeArea = component.GetBiomeArea();
    //        //hmap = component;
    //        return component;
    //    }
    //}
    //else
    //    normal = Vector3::UP;
    //
    //return nullptr;
}

// public
bool IZoneManager::FindClosestLocation(const std::string& name, const Vector3& point, LocationInstance& closest) {
    float num = std::numeric_limits<float>::max();
    bool result = false;

    for (auto&& pair : m_locationInstances) {
        auto&& loc = pair.second;
        float num2 = loc.m_position.Distance(point);
        if (loc.m_location->m_name == name && num2 < num) {
            num = num2;
            closest = loc;
            result = true;
        }
    }

    return result;
}

// public
// this is world position to zone position
// formerly GetZone
Vector2i IZoneManager::WorldToZonePos(const Vector3& point) {
    int32_t x = floor((point.x + (float)ZONE_SIZE / 2.f) / (float)ZONE_SIZE);
    int32_t y = floor((point.z + (float)ZONE_SIZE / 2.f) / (float)ZONE_SIZE);
    return Vector2i(x, y);
}

// public
// zone position to ~world position
// GetZonePos
Vector3 IZoneManager::ZoneToWorldPos(const ZoneID& id) {
    return Vector3(id.x * ZONE_SIZE, 0, id.y * ZONE_SIZE);
}

// inlined because 1 use only
// private
//void SetZoneGenerated(const Vector2i& zoneID) {
//    m_generatedZones.insert(zoneID);
//}

// private
bool IZoneManager::IsZoneGenerated(const ZoneID& zoneID) {
    return m_generatedZones.contains(zoneID);
}

// public
//float TimeSinceStart() {
//    return m_lastFixedTime - m_startTime;
//}

// public
void IZoneManager::ResetGlobalKeys() {
    m_globalKeys.clear();
    SendGlobalKeys(IRouteManager::EVERYBODY);
}

// client terminal
/*
// public
void SetGlobalKey(const std::string& name) {
    ZRoutedRpc.instance.InvokeRoutedRPC("SetGlobalKey", new object[]{
        name
        });
}*/

// public
// seems to be client only, could be wrong
/*
bool GetGlobalKey(const std::string& name) {
    return m_globalKeys.Contains(name);
}*/

// public
// client terminal only
/*
void RemoveGlobalKey(const std::string& name) {
    NetRouteManager::Invoke("RemoveGlobalKey", name);
}*/

// public
// client terminal only
/*
std::vector<std::string> GetGlobalKeys() {
    return new std::vector<std::string>(m_globalKeys);
}*/

// client terminal only
// public
/*
robin_hood::unordered_map<Vector2i, ZoneSystem.LocationInstance>.ValueCollection GetLocationList() {
    return m_locationInstances.Values;
}*/


