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



struct RandomSpawn {};

// private
//struct ZoneData {
//    GameObject m_root;
//
//    float m_ttl;
//};







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





// private
void IZoneManager::Init() {
    LOG(INFO) << "Initializing ZoneManager";

    {
        // load ZoneLocations:
        auto opt = VUtils::Resource::ReadFileBytes("zoneLocations.pkg");
        if (!opt)
            throw std::runtime_error("zoneLocations.pkg missing");

        NetPackage pkg(opt.value());

        auto count = pkg.Read<int32_t>();
        LOG(INFO) << "Loading " << count << " ZoneLocations";
        while (count--) {
            // TODO read zoneLocations from file
            auto loc = std::make_unique<ZoneLocation>();
            auto prefabName = pkg.Read<std::string>();

            loc->m_prefab = PrefabManager()->GetPrefab(prefabName);
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
            loc->m_prioritized = pkg.Read<bool>();
            loc->m_quantity = pkg.Read<int32_t>();
            loc->m_randomRotation = pkg.Read<bool>();
            loc->m_slopeRotation = pkg.Read<bool>();
            loc->m_snapToWater = pkg.Read<bool>();
            loc->m_unique = pkg.Read<bool>();

            auto count = pkg.Read<int32_t>();
            while (count--) {
                ZoneLocation::Piece piece;
                piece.m_prefab = PrefabManager()->GetPrefab(pkg.Read<HASH_t>());
                assert(piece.m_prefab && "missing location prefab");
                piece.m_pos = pkg.Read<Vector3>();
                piece.m_rot = pkg.Read<Quaternion>();
                loc->m_pieces.push_back(piece);
            }

            m_locationsByHash[VUtils::String::GetStableHashCode(prefabName)] = std::move(loc);

            //m_locations.push_back(std::move(loc));
        }
    }

    {
        // load ZoneVegetation:
        auto opt = VUtils::Resource::ReadFileBytes("vegetation.pkg");
        if (!opt)
            throw std::runtime_error("vegetation.pkg missing");

        NetPackage pkg(opt.value());

        auto count = pkg.Read<int32_t>();
        LOG(INFO) << "Loading " << count << " ZoneVegetations";
        while (count--) {
            auto veg = std::make_unique<ZoneVegetation>();

            auto prefabName = pkg.Read<std::string>();

            veg->m_prefab = PrefabManager()->GetPrefab(prefabName);
            assert(veg->m_prefab && "missing vegetation prefab");
            veg->m_biome = (Biome) pkg.Read<int32_t>();
            veg->m_biomeArea = (BiomeArea) pkg.Read<int32_t>();
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

    RouteManager()->Register("SetGlobalKey", [this](Peer* peer, std::string name) {
        // TODO constraint check
        if (m_globalKeys.contains(name)) {
            return;
        }
        m_globalKeys.insert(name);
        SendGlobalKeys(IRouteManager::EVERYBODY);
    });

    RouteManager()->Register("RemoveGlobalKey", [this](Peer* peer, std::string name) {
        // TODO constraint check
        if (!m_globalKeys.contains(name)) {
            return;
        }
        m_globalKeys.erase(name);
        SendGlobalKeys(IRouteManager::EVERYBODY);
    });
}

// private
void IZoneManager::SendGlobalKeys(OWNER_t peer) {
    RouteManager()->Invoke(peer, "GlobalKeys", m_globalKeys);
}

// private
void IZoneManager::SendLocationIcons(OWNER_t peer) {
    NetPackage zpackage;
    tempIconList.clear();
    GetLocationIcons(tempIconList);
    zpackage.Write<int32_t>(tempIconList.size());
    for (auto&& keyValuePair : tempIconList) {
        zpackage.Write(keyValuePair.first);
        zpackage.Write(keyValuePair.second);
    }

    RouteManager()->Invoke(peer, "LocationIcons", zpackage);
}

// private
void IZoneManager::OnNewPeer(OWNER_t peerID) {
    LOG(INFO) << "Server: New peer connected,sending global keys";
    SendGlobalKeys(peerID);
    SendLocationIcons(peerID);
}

// public
void IZoneManager::Save(NetPackage& pkg) {
    pkg.Write((int32_t)m_generatedZones.size());
    for (auto&& vec : m_generatedZones) {
        pkg.Write(vec);
    }
    pkg.Write(VConstants::PGW);
    pkg.Write(VConstants::LOCATION);
    pkg.Write(m_globalKeys);

    pkg.Write(true); // m_worldSave.Write(m_locationsGenerated);
    pkg.Write((int32_t)m_locationInstances.size());
    for (auto&& pair : m_locationInstances) {
        auto&& inst = pair.second;
        pkg.Write(inst.m_location->m_prefab->m_name);
        pkg.Write(inst.m_position);
        pkg.Write(inst.m_placed);
    }
}

// public
void IZoneManager::Load(NetPackage& reader, int32_t version) {
    auto countZones = reader.Read<int32_t>();
    while (countZones--) {
        m_generatedZones.insert(reader.Read<Vector2i>());
    }

    if (version >= 13) {
        int32_t pgwVersion = reader.Read<int32_t>(); // 99
        int32_t locationVersion = (version >= 21) ? reader.Read<int32_t>() : 0; // 26
        if (pgwVersion != VConstants::PGW) {
            throw std::runtime_error("incompatible PGW version");
        }

        if (version >= 14) {
            int32_t countKeys = reader.Read<int32_t>();
            while (countKeys--) {
                m_globalKeys.insert(reader.Read<std::string>());
            }
        }

        if (version >= 18) {
            // kinda dumb, ?
            if (version >= 20) //m_locationsGenerated = reader.Read<bool>();
                reader.Read<bool>();
            
            const auto countLocations = reader.Read<int32_t>();
            for (int32_t i = 0; i < countLocations; i++) {
                auto text = reader.Read<std::string>();
                auto pos = reader.Read<Vector3>();
                bool placed = version >= 19 ? reader.Read<bool>() : false;

                auto&& location = GetLocation(text);
                if (location) {
                    m_locationInstances[WorldToZonePos(pos)] = {location, pos, placed};
                }
                else {
                    LOG(ERROR) << "Failed to find location " << text;
                }
            }

            LOG(INFO) << "Loaded " << countLocations << " ZoneLocation instances";
            //if (pgw != VConstants::PGW) {
                //m_locationInstances.clear();
            //}

            if (locationVersion != VConstants::LOCATION) {
                //m_locationsGenerated = false;
            }
        }
    }
}

// private
void IZoneManager::Update() {
    PERIODIC_NOW(100ms, {
        //UpdateTTL(.1f);
        auto&& peers = NetManager()->GetPeers();
        for (auto&& pair : peers) {
            auto&& peer = pair.second;
            CreateGhostZones(peer->m_pos);
        }
    });
}

// private
void IZoneManager::CreateGhostZones(const Vector3& refPoint) {
    Vector2i zone = WorldToZonePos(refPoint);

    auto num = NEAR_ACTIVE_AREA + DISTANT_ACTIVE_AREA;
    for (int32_t z = zone.y - num; z <= zone.y + num; z++) {
        for (int32_t x = zone.x - num; x <= zone.x + num; x++) {
            SpawnZone({x, z});
        }
    }
}

void IZoneManager::SpawnZone(const ZoneID& zoneID) {
    //Heightmap componentInChildren = m_zonePrefab.GetComponentInChildren<Heightmap>();

    // _root object is ZonePrefab, Components:
    //  WaterVolume
    //  Heightmap

    // Wait for builder thread
    //if (!(IsZoneGenerated(zoneID) && HeightmapBuilder::IsTerrainReady(zoneID))) {
    if (!IsZoneGenerated(zoneID) && HeightmapBuilder::IsTerrainReady(zoneID)) {
        auto componentInChildren2 = HeightmapManager()->GetOrCreateHeightmap(zoneID);

        std::vector<ClearArea> m_tempClearAreas;
        PlaceLocations(zoneID, m_tempClearAreas);
        PlaceVegetation(zoneID, componentInChildren2, m_tempClearAreas);
        PlaceZoneCtrl(zoneID);

        m_generatedZones.insert(zoneID);
    }
}

// private
void IZoneManager::PlaceZoneCtrl(const ZoneID& zoneID) {
    auto pos = ZoneToWorldPos(zoneID);
    PrefabManager()->Instantiate(Hashes::Object::_ZoneCtrl, pos);
}

// private
Vector3 IZoneManager::GetRandomPointInRadius(VUtils::Random::State& state, const Vector3& center, float radius) {
    float f = state.NextFloat() * PI * 2.f;
    float num = state.Range(0.f, radius);
    return center + Vector3(sin(f) * num, 0.f, cos(f) * num);
}

// private
void IZoneManager::PlaceVegetation(const ZoneID&zoneID, Heightmap *hmap, std::vector<ClearArea>& clearAreas) {
    const Vector3 zoneCenterPos = ZoneToWorldPos(zoneID);

    int32_t seed = GeoManager()->GetSeed();
    float num = ZONE_SIZE / 2.f;
    int32_t num2 = 1;
    for (auto&& zoneVegetation : m_vegetation) {
        num2++;
        if (hmap->HaveBiome(zoneVegetation->m_biome)) {

            VUtils::Random::State state(
                seed + zoneID.x * 4271 + zoneID.y * 9187 +
                VUtils::String::GetStableHashCode(zoneVegetation->m_prefab->m_name)
            );

            int32_t num3 = 1;
            if (zoneVegetation->m_max < 1) {
                if (state.NextFloat() > zoneVegetation->m_max) {
                    continue;
                }
            }
            else {
                num3 = state.Range((int32_t)zoneVegetation->m_min, (int32_t)zoneVegetation->m_max + 1);
            }

            // flag should always be true, all vegetation seem to always have a NetView
            //bool flag = zoneVegetation.m_prefab.GetComponent<ZNetView>() != null;
            float num4 = cos((PI / 180.f) * zoneVegetation->m_maxTilt);
            float num5 = cos((PI / 180.f) * zoneVegetation->m_minTilt);
            float num6 = num - zoneVegetation->m_groupRadius;
            int32_t num7 = zoneVegetation->m_forcePlacement ? (num3 * 50) : num3;
            int32_t num8 = 0;
            for (int32_t i = 0; i < num7; i++) {

                Vector3 vector(state.Range(zoneCenterPos.x - num6, zoneCenterPos.x + num6),
                    0.f, state.Range(zoneCenterPos.z - num6, zoneCenterPos.z + num6));

                int32_t num9 = state.Range(zoneVegetation->m_groupSizeMin, zoneVegetation->m_groupSizeMax + 1);
                bool flag2 = false;
                for (int32_t j = 0; j < num9; j++) {

                    Vector3 vector2 = (j == 0) ? vector
                        : GetRandomPointInRadius(state, vector, zoneVegetation->m_groupRadius);

                    // random rotations
                    float rot_y = (float)state.Range(0, 360);
                    float scale = state.Range(zoneVegetation->m_scaleMin, zoneVegetation->m_scaleMax);
                    float rot_x = state.Range(-zoneVegetation->m_randTilt, zoneVegetation->m_randTilt);
                    float rot_z = state.Range(-zoneVegetation->m_randTilt, zoneVegetation->m_randTilt);

                    if (!zoneVegetation->m_blockCheck
                        || !IsBlocked(vector2)) {

                        Vector3 vector3;
                        Biome biome;
                        BiomeArea biomeArea;
                        Heightmap *otherHeightmap = GetGroundData(vector2, vector3, biome, biomeArea);

                        if ((std::to_underlying(zoneVegetation->m_biome) & std::to_underlying(biome))
                            && (std::to_underlying(zoneVegetation->m_biomeArea) & std::to_underlying(biomeArea))) {

                            float y2;
                            Vector3 vector4;
                            if (zoneVegetation->m_snapToStaticSolid && GetStaticSolidHeight(vector2, y2, vector4)) {
                                vector2.y = y2;
                                vector3 = vector4;
                            }
                            float num11 = vector2.y - WATER_LEVEL;
                            if (num11 >= zoneVegetation->m_minAltitude && num11 <= zoneVegetation->m_maxAltitude) {
                                if (zoneVegetation->m_minVegetation != zoneVegetation->m_maxVegetation) {
                                    float vegetationMask = otherHeightmap->GetVegetationMask(vector2);
                                    if (vegetationMask > zoneVegetation->m_maxVegetation || vegetationMask < zoneVegetation->m_minVegetation) {
                                        continue;
                                        //goto IL_501;
                                    }
                                }
                                if (zoneVegetation->m_minOceanDepth != zoneVegetation->m_maxOceanDepth) {
                                    float oceanDepth = otherHeightmap->GetOceanDepth(vector2);
                                    if (oceanDepth < zoneVegetation->m_minOceanDepth || oceanDepth > zoneVegetation->m_maxOceanDepth) {
                                        continue;
                                        //goto IL_501;
                                    }
                                }
                                if (vector3.y >= num4 && vector3.y <= num5) {
                                    if (zoneVegetation->m_terrainDeltaRadius > 0) {
                                        float num12;
                                        Vector3 vector5;
                                        GetTerrainDelta(vector2, zoneVegetation->m_terrainDeltaRadius, num12, vector5);
                                        if (num12 > zoneVegetation->m_maxTerrainDelta || num12 < zoneVegetation->m_minTerrainDelta) {
                                            continue;
                                            //goto IL_501;
                                        }
                                    }
                                    if (zoneVegetation->m_inForest) {
                                        float forestFactor = GeoManager()->GetForestFactor(vector2);
                                        if (forestFactor < zoneVegetation->m_forestTresholdMin || forestFactor > zoneVegetation->m_forestTresholdMax) {
                                            continue;
                                            //goto IL_501;
                                        }
                                    }


                                    if (!InsideClearArea(clearAreas, vector2)) {
                                        if (zoneVegetation->m_snapToWater) {
                                            vector2.y = WATER_LEVEL;
                                        }
                                        vector2.y += zoneVegetation->m_groundOffset;
                                        Quaternion rotation = Quaternion::IDENTITY;



                                        if (zoneVegetation->m_chanceToUseGroundTilt > 0
                                            && state.NextFloat() <= zoneVegetation->m_chanceToUseGroundTilt) {
                                            //Quaternion rotation2 = Quaternion::Euler(0.f, rot_y, 0.f);
                                            //rotation = Quaternion.LookRotation(
                                            //    vector3.Cross(rotation2 * Vector3::FORWARD),
                                            //    vector3);
                                        }
                                        else {
                                            //rotation = Quaternion::Euler(rot_x, rot_y, rot_z);
                                        }



                                        // TODO modify this later once Euler implemented
                                        rotation = Quaternion::Euler(rot_x, rot_y, rot_z);

                                        auto zdo = PrefabManager()->Instantiate(zoneVegetation->m_prefab, vector2, rotation);

                                        if (!zdo)
                                            throw std::runtime_error("zdo failed to instantiate; vegetation.pkg might be corrupt");

                                        if (scale != zoneVegetation->m_prefab->m_localScale.x) {
                                            // this does set the Unity gameobject localscale
                                            zdo->Set("scale", Vector3(scale, scale, scale));

                                            //zdo->SetLocalScale(Vector3(scale, scale, scale));
                                        }

                                        flag2 = true;
                                    }
                                }
                            }
                        }
                    }
                    //IL_501:;
                        // serves as a continue
                }
                if (flag2) {
                    num8++;
                }
                if (num8 >= num3) {
                    break;
                }
            }
        }
    }
}

// private
bool IZoneManager::InsideClearArea(const std::vector<ClearArea>& areas, const Vector3& point) {
    for (auto&& clearArea : areas) {
        if (point.x > clearArea.m_center.x - clearArea.m_radius
            && point.x < clearArea.m_center.x + clearArea.m_radius
            && point.z > clearArea.m_center.z - clearArea.m_radius
            && point.z < clearArea.m_center.z + clearArea.m_radius) {
            return true;
        }
    }
    return false;
}

// private
IZoneManager::ZoneLocation* IZoneManager::GetLocation(int32_t hash) {
    auto&& find = m_locationsByHash.find(hash);
    if (find != m_locationsByHash.end())
        return find->second.get();

    return nullptr;
}

// private
IZoneManager::ZoneLocation* IZoneManager::GetLocation(const std::string& name) {
    return GetLocation(VUtils::String::GetStableHashCode(name));
}

// public
// call from within ZNet.init or earlier...
void IZoneManager::GenerateLocations() {
    auto now(steady_clock::now());

    // Generate prioritized locations first
    for (auto&& pair : m_locationsByHash) {
        auto&& loc = pair.second;
        if (loc->m_prioritized)
            GenerateLocations(loc.get());
    }

    // Then generate unprioritized locations
    for (auto&& pair : m_locationsByHash) {
        auto&& loc = pair.second;
        if (!loc->m_prioritized)
            GenerateLocations(loc.get());
    }

    LOG(INFO) << "Location generation took " << duration_cast<milliseconds>(steady_clock::now() - now).count() << "ms";
}

// private
void IZoneManager::GenerateLocations(const ZoneLocation* location) {
    int spawnedLocations = 0;

    // CountNrOfLocation: inlined
    for (auto&& inst : m_locationInstances) {
        if (inst.second.m_location == location) // better to compare locations itself rather than name
            spawnedLocations++;
    }

    // Haldor is unique
    // Yet he should be spawned throughout the map
    // This logic is contradictory to his own behaviour
    // Why is there just 1 attempt to spawn haldor?
    //  Actually this function is called once per ZoneLocation, so it actually makes sense
    //      so redundant...
    //if (location->m_unique && spawnedLocations)
        //return;

    unsigned int errLocations = 0;
    unsigned int errCenterDistances = 0;
    unsigned int errNoneBiomes = 0;
    unsigned int errBiomeArea = 0;
    unsigned int errAltitude = 0;
    unsigned int errForestFactor = 0;
    unsigned int errSimilarLocation = 0;
    unsigned int errTerrainDelta = 0;

    VUtils::Random::State state(GeoManager()->GetSeed() + VUtils::String::GetStableHashCode(location->m_prefab->m_name));
    const float locationRadius = std::max(location->m_exteriorRadius, location->m_interiorRadius);

    float range = location->m_centerFirst ? location->m_minDistance : 10000;

    const int spawnAttempts = location->m_prioritized ? 200000 : 100000;
    for (int a = 0; a < spawnAttempts && spawnedLocations < location->m_quantity; a++) {
        Vector2i randomZone = GetRandomZone(state, range);
        if (location->m_centerFirst)
            range++;

        if (m_locationInstances.contains(randomZone))
            errLocations++;
        else {            
            Vector3 zonePos = ZoneToWorldPos(randomZone);
            BiomeArea biomeArea = GeoManager()->GetBiomeArea(zonePos);

            if (std::to_underlying(location->m_biomeArea) & std::to_underlying(biomeArea))
                errBiomeArea++;
            else {
                for (int i = 0; i < 20; i++) {

                    // GetRandomPointInZone(): inlined
                    float num = ZONE_SIZE / 2.f;
                    float x = state.Range(-num + locationRadius, num - locationRadius);
                    float z = state.Range(-num + locationRadius, num - locationRadius);
                    Vector3 randomPointInZone = zonePos + Vector3(x, 0.f, z);



                    float magnitude = randomPointInZone.Magnitude();
                    if (magnitude > 0 && (magnitude < location->m_minDistance || magnitude > location->m_maxDistance))
                        errCenterDistances++;
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
                                    if (location->m_minDistanceFromSimilar <= 0) {

                                        // HaveLocationInRange: inlined
                                        bool locInRange = false;
                                        for (auto&& inst : m_locationInstances) {
                                            auto&& loc = inst.second.m_location;

                                            // Try to find the same ZoneLocation or any ZoneLocation with the same group
                                            if ((loc->m_prefab == location->m_prefab
                                                || (!location->m_group.empty() && location->m_group == loc->m_group))
                                                && inst.second.m_position.Distance(randomPointInZone) < location->m_minDistanceFromSimilar)
                                            {
                                                locInRange = true;
                                                break;
                                            }
                                        }

                                        if (!locInRange) {
                                            m_locationInstances.insert({ 
                                                WorldToZonePos(randomPointInZone), { location, randomPointInZone, false }});
                                            spawnedLocations++;
                                            break;
                                        }
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
        LOG(ERROR) << "Failed to place all " << location->m_prefab->m_name << ", placed " << spawnedLocations << "/" << location->m_quantity;

        LOG(ERROR) << "errLocations " << errLocations;
        LOG(ERROR) << "errCenterDistances " << errCenterDistances;
        LOG(ERROR) << "errNoneBiomes " << errNoneBiomes;
        LOG(ERROR) << "errBiomeArea " << errBiomeArea;
        LOG(ERROR) << "errAltitude " << errAltitude;
        LOG(ERROR) << "errForestFactor " << errForestFactor;
        LOG(ERROR) << "errSimilarLocation " << errSimilarLocation;
        LOG(ERROR) << "errTerrainDelta " << errTerrainDelta;
    }
}

Vector2i IZoneManager::GetRandomZone(VUtils::Random::State& state, float range) {
    int num = (int32_t)range / (int32_t)ZONE_SIZE;
    Vector2i vector2i;
    do {
        vector2i = Vector2i(state.Range(-num, num), state.Range(-num, num));
    } while (ZoneToWorldPos(vector2i).Magnitude() >= 10000);
    return vector2i;
}

/*
    // private
    Vector2i GetRandomZone(float range) {
        int32_t num = (int32_t)range / (int32_t)m_zoneSize;
        Vector2i vector2i;
        do {
            vector2i = new Vector2i(UnityEngine.Random.Range(-num, num), UnityEngine.Random.Range(-num, num));
        } while (GetZonePos(vector2i).magnitude >= 10000);
        return vector2i;
    }
    // private
    Vector3 GetRandomPointInZone(const Vector2i& zone, float locationRadius) {
        Vector3 zonePos = GetZonePos(zone);
        float num = m_zoneSize / 2f;
        float x = UnityEngine.Random.Range(-num + locationRadius, num - locationRadius);
        float z = UnityEngine.Random.Range(-num + locationRadius, num - locationRadius);
        return zonePos + new Vector3(x, 0f, z);
    }
    // private
    Vector3 GetRandomPointInZone(float locationRadius) {
        Vector3 point = new Vector3(UnityEngine.Random.Range(-10000f, 10000f), 0f, UnityEngine.Random.Range(-10000f, 10000f));
        Vector2i zone = GetZone(point);
        Vector3 zonePos = GetZonePos(zone);
        float num = m_zoneSize / 2f;
        return new Vector3(UnityEngine.Random.Range(zonePos.x - num + locationRadius, zonePos.x + num - locationRadius), 0f, UnityEngine.Random.Range(zonePos.z - num + locationRadius, zonePos.z + num - locationRadius));
    }*/

    // private
void IZoneManager::PlaceLocations(const Vector2i &zoneID,
    std::vector<ClearArea>& clearAreas)
{
    auto now(steady_clock::now());

    auto&& find = m_locationInstances.find(zoneID);
    if (find != m_locationInstances.end()) {
        auto&& locationInstance = find->second;
        if (locationInstance.m_placed) {
            return;
        }

        Vector3 position = locationInstance.m_position;
        Vector3 vector;
        Biome biome = Biome::None;
        BiomeArea biomeArea;
        Heightmap *heightmap = GetGroundData(position, vector, biome, biomeArea);

        if (locationInstance.m_location->m_snapToWater)
            position.y = WATER_LEVEL;

        if (locationInstance.m_location->m_clearArea)
            clearAreas.push_back({position, locationInstance.m_location->m_exteriorRadius });

        assert(false);

        Quaternion rot(Quaternion::IDENTITY);
        if (locationInstance.m_location->m_slopeRotation) {
            float num;
            Vector3 vector2;
            GetTerrainDelta(position, locationInstance.m_location->m_exteriorRadius, num, vector2);
            Vector3 forward(vector2.x, 0.f, vector2.z);
            forward.Normalize();
            rot = Quaternion::LookRotation(forward);
            assert(false);
            //Vector3 eulerAngles = rot.eulerAngles;
            //eulerAngles.y = round(eulerAngles.y / 22.5f) * 22.5f;
            //rot.eulerAngles = eulerAngles;
        }
        //else if (locationInstance.m_location.m_randomRotation) {
        //    //rot = Quaternion::Euler(0, (float)UnityEngine.Random.Range(0, 16) * 22.5f, 0);
        //    rot = Quaternion::Euler(0, (float)VUtils::Random::State().Range(0, 16) * 22.5f, 0);
        //}

        int32_t seed = GeoManager()->GetSeed() + zoneID.x * 4271 + zoneID.y * 9187;
        SpawnLocation(locationInstance.m_location, seed, position, rot);
        locationInstance.m_placed = true;
        m_locationInstances[zoneID] = locationInstance;

        //TimeSpan timeSpan = DateTime.Now - now;

        LOG(INFO) << "Placed locations in zone "
            << zoneID.x << " " << zoneID.y << " duration "
            << duration_cast<milliseconds>(steady_clock::now() - now).count();

        if (locationInstance.m_location->m_unique) {
            RemoveUnplacedLocations(locationInstance.m_location);
        }
        if (locationInstance.m_location->m_iconPlaced) {
            SendLocationIcons(IRouteManager::EVERYBODY);
        }
    }
}

// private
void IZoneManager::RemoveUnplacedLocations(const ZoneLocation* location) {
    std::vector<Vector2i> list;
    for (auto&& keyValuePair : m_locationInstances) {
        if (keyValuePair.second.m_location == location && !keyValuePair.second.m_placed) {
            list.push_back(keyValuePair.first);
        }
    }

    for (auto&& key : list) {
        m_locationInstances.erase(key);
    }

    LOG(INFO) << "Removed " << list.size() << " unplaced locations of type " << location->m_prefab->m_name;
}

// private
void IZoneManager::SpawnLocation(const ZoneLocation* location, int32_t seed, const Vector3& pos, const Quaternion& rot) {

    //location->m_prefab.transform.position = Vector3::ZERO;
    //location->m_prefab.transform.rotation = Quaternion::IDENTITY;

    //Location component = location.m_prefab.GetComponent<Location>();
    //bool flag = component.m_useCustomInteriorTransform && component.m_interiorTransform && component.m_generator;
    //bool flag = location->m_useCustomInteriorTransform && location->m_generatorPosition;
    bool flag = false;
    if (flag) {
        LOG(ERROR) << "Trieed pre-initializing ZoneLocation Dungeon: " << location->m_prefab->m_name;
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
    for (auto&& znetView2 : location->m_pieces) {
        //if (znetView2.gameObject.activeSelf) {
            Vector3 position = znetView2.m_pos;
            Vector3 position2 = pos + rot * position;
            Quaternion rotation = znetView2.m_rot;
            Quaternion rotation2 = rot * rotation;

            PrefabManager()->Instantiate(znetView2.m_prefab, position2, rotation2);

            //GameObject gameObject = UnityEngine.Object.Instantiate<GameObject>(znetView2.gameObject, position2, rotation2);
            //gameObject.GetComponent<ZNetView>().GetZDO().SetPGWVersion(m_pgwVersion);
            //DungeonGenerator component2 = gameObject.GetComponent<DungeonGenerator>();
            //if (component2) {
            //    if (flag) {
            //        component2.m_originalPosition = location.m_generatorPosition;
            //    }
            //    component2.Generate(mode);
            //}
        //}
    }
    //WearNTear.m_randomInitialDamage = false;
    CreateLocationProxy(location, seed, pos, rot);
    //SnapToGround.SnappAll();
}

// could be inlined...
// private
void IZoneManager::CreateLocationProxy(const ZoneLocation* location, int32_t seed, const Vector3& pos, const Quaternion& rot) {
    auto zdo = PrefabManager()->Instantiate(LOCATION_PROXY_PREFAB, pos, rot);
    
    zdo->Set("location", location->m_prefab->m_hash);
    zdo->Set("seed", seed);
}

// private
bool IZoneManager::HaveLocationInRange(const std::string& prefabName, const std::string& group, const Vector3& p, float radius) {
    for (auto&& inst : m_locationInstances) {
        if ((inst.second.m_location->m_prefab->m_name == prefabName
            || (!group.empty() && group == inst.second.m_location->m_group))
            && inst.second.m_position.Distance(p) < radius) {
            return true;
        }
    }
    return false;
}

// public
void IZoneManager::GetLocationIcons(robin_hood::unordered_map<Vector3, std::string> &icons) {
    for (auto&& pair : m_locationInstances) {
        auto&& loc = pair.second;
        if (loc.m_location->m_iconAlways
            || (loc.m_location->m_iconPlaced && loc.m_placed))
        {
            icons[pair.second.m_position] = loc.m_location->m_prefab->m_name;
        }
    }
}

// private
void IZoneManager::GetTerrainDelta(const Vector3& center, float radius, float& delta, Vector3& slopeDirection) {
    float num2 = std::numeric_limits<float>::min();
    float num3 = std::numeric_limits<float>::max();
    Vector3 b = center;
    Vector3 a = center;
    VUtils::Random::State state;
    for (int32_t i = 0; i < 10; i++) {
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
    Vector3 height = p;
    Vector3 n;
    Biome biome;
    BiomeArea biomeArea;
    GetGroundData(height, n, biome, biomeArea);

    return height.y;

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
Heightmap* IZoneManager::GetGroundData(Vector3& p, Vector3& normal, Biome& biome, BiomeArea& biomeArea) {
    //biome = Heightmap::Biome::None;
    //biomeArea = Heightmap::BiomeArea::Everything;
    //hmap = null;

    // test collision from point, casting downwards through terrain

    // If final result is completely linked to Heightmap, could just use heightmap
    // should be simple enough,

    // global heightmaps can be queried at the world--->zone then 
    // get the relative point inside zone



    auto heightmap = HeightmapManager()->GetOrCreateHeightmap(WorldToZonePos(p));

    p.y = HeightmapManager()->GetHeight(p);
    biome = heightmap->GetBiome(p);
    biomeArea = heightmap->GetBiomeArea();

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
        if (loc.m_location->m_prefab->m_name == name && num2 < num) {
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
Vector3 IZoneManager::ZoneToWorldPos(const Vector2i& id) {
    return Vector3(id.x * ZONE_SIZE, 0, id.y * ZONE_SIZE);
}

// inlined because 1 use only
// private
//void SetZoneGenerated(const Vector2i& zoneID) {
//    m_generatedZones.insert(zoneID);
//}

// private
bool IZoneManager::IsZoneGenerated(const Vector2i& zoneID) {
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


