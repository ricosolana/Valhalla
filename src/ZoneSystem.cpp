#include <ranges>
#include <thread>

#include "NetManager.h"
#include "ZoneSystem.h"
#include "WorldGenerator.h"
#include "HeightmapBuilder.h"
#include "PrefabHashes.h"

// I feel like zdos are what allows for objects to be created
// simple ZDO pool containing the most recent objects, which are frequently revised


// Dummy forward declarations


struct RandomSpawn {};

struct LocationProxy {
    //LocationProxy()
};


// private
struct ZoneData {
    GameObject m_root;

    float m_ttl;
};

struct ClearArea {
    // public
    ClearArea(const Vector3& p, float r) {
        m_center = p;
        m_radius = r;
    }

    Vector3 m_center;

    float m_radius;
};

struct Location {
    ClearArea m_clearArea;
};

// Is used externally, unknown whether this can be made private
//[Serializable]
struct ZoneVegetation {
    // public
    ZoneVegetation(const ZoneVegetation& other) {
        // clone members
    }

    std::string m_name = "veg";

    GameObject m_prefab;

    bool m_enable = true;

    float m_min = 0;

    float m_max = 10;

    bool m_forcePlacement = false;

    float m_scaleMin = 1;

    float m_scaleMax = 1;

    float m_randTilt = 0;

    float m_chanceToUseGroundTilt = 0;

    //[BitMask(typeof(Heightmap.Biome))]
    Heightmap::Biome m_biome = Heightmap::Biome::None;

    //[BitMask(typeof(Heightmap.BiomeArea))]
    Heightmap::BiomeArea m_biomeArea = Heightmap::BiomeArea::Everything;

    bool m_blockCheck = true;

    bool m_snapToStaticSolid = false;

    float m_minAltitude = -1000;

    float m_maxAltitude = 1000;

    float m_minVegetation = 0;

    float m_maxVegetation = 0;

    float m_minOceanDepth = 0;

    float m_maxOceanDepth = 0;

    float m_minTilt = 0;

    float m_maxTilt = 90;

    float m_terrainDeltaRadius = 0;

    float m_maxTerrainDelta = 2;

    float m_minTerrainDelta = 0;

    bool m_snapToWater = false;

    float m_groundOffset = 0;

    int32_t m_groupSizeMin = 1;

    int32_t m_groupSizeMax = 1;

    float m_groupRadius = 0;

    //[Header("Forest fractal 0-1 inside forest")]
    bool m_inForest = false;

    float m_forestTresholdMin = 0;

    float m_forestTresholdMax = 1;

    //[HideInInspector]
    bool m_foldout = false;
};

// is used externally
//[Serializable]
struct ZoneLocation {
    //ZoneLocation(const ZoneLocation& other) {
    //    // clone
    //}

    bool m_enable = true;

    std::string m_prefabName;

    //[BitMask(typeof(Heightmap.Biome))]
    Heightmap::Biome m_biome;

    //[BitMask(typeof(Heightmap.BiomeArea))]
    Heightmap::BiomeArea m_biomeArea = Heightmap::BiomeArea::Everything;

    int32_t m_quantity;

    bool m_prioritized;

    bool m_centerFirst;

    bool m_unique;

    std::string m_group = "";

    float m_minDistanceFromSimilar;

    bool m_iconAlways;

    bool m_iconPlaced;

    bool m_randomRotation = true;

    bool m_slopeRotation;

    bool m_snapToWater;

    float m_minTerrainDelta;

    float m_maxTerrainDelta = 2;

    [Header("Forest fractal 0-1 inside forest")]
    bool m_inForest;

    float m_forestTresholdMin;

    float m_forestTresholdMax = 1;

    [Space(10f)]
    float m_minDistance;

    float m_maxDistance;

    float m_minAltitude = -1000;

    float m_maxAltitude = 1000;

    //[NonSerialized]
    GameObject m_prefab;

    //[NonSerialized]
    int32_t m_hash;

    //[NonSerialized]
    Location m_location;

    //[NonSerialized]
    float m_interiorRadius = 10;

    //[NonSerialized]
    float m_exteriorRadius = 10;

    //[NonSerialized]
    Vector3 m_interiorPosition;

    //[NonSerialized]
    Vector3 m_generatorPosition;

    //[NonSerialized]
    std::vector<ZNetView> m_netViews;

    //[NonSerialized]
    std::vector<RandomSpawn> m_randomSpawns;

    //[HideInInspector]
    bool m_foldout;
};

// can be private
struct LocationInstance {
    ZoneLocation* m_location;

    Vector3 m_position;

    bool m_placed;
};

enum SpawnMode {
    Full,
    Client,
    Ghost
};



bool IZoneManager::InActiveArea(const Vector2i& zone, const Vector3& refPoint) {
    return InActiveArea(zone,
        WorldToZonePos(refPoint));
}

bool IZoneManager::InActiveArea(const Vector2i& zone, const Vector2i& refCenterZone) {
    int num = ACTIVE_AREA - 1;
    return zone.x >= refCenterZone.x - num
        && zone.x <= refCenterZone.x + num
        && zone.y <= refCenterZone.y + num
        && zone.y >= refCenterZone.y - num;
}





// private
void IZoneManager::Init() {
    //LOG(INFO) << "Zonesystem Start " << Time.frameCount.ToString();
    LOG(INFO) << "ZoneSystem Start ";
    //SetupLocations();
    //ValidateVegetation();
    //ZRoutedRpc instance = ZRoutedRpc.instance;
    //instance.m_onNewPeer = (Action<long>)Delegate.Combine(instance.m_onNewPeer, new Action<long>(OnNewPeer));
    RouteManager()->Register("SetGlobalKey", [this](OWNER_t sender, std::string name) {
        if (m_globalKeys.contains(name)) {
            return;
        }
        m_globalKeys.insert(name);
        SendGlobalKeys(IRouteManager::EVERYBODY);
    });
    RouteManager()->Register("RemoveGlobalKey", [this](OWNER_t sender, std::string name) {
        if (!m_globalKeys.contains(name)) {
            return;
        }
        m_globalKeys.erase(name);
        SendGlobalKeys(IRouteManager::EVERYBODY);
    });
    //m_startTime = (m_lastFixedTime = Time.fixedTime);
}

// public

//void GenerateLocationsIfNeeded() {
//    if (!m_locationsGenerated) {
//        GenerateLocations();
//    }
//}

// private
void IZoneManager::SendGlobalKeys(OWNER_t peer) {
    //std::vector<std::string> list = new std::vector<std::string>(m_globalKeys);
    //std::vector<std::string> list = new std::vector<std::string>(m_globalKeys);
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

// private
//  Added 3 locations, 0 vegetations, 0 environments, 0 biome env-setups from locations_cp1
//  Added 1 locations, 0 vegetations, 1 environments, 0 biome env-setups from locations_mountaincaves
// almost nothing is loaded with this fn, idk why
// so it is practically redundant (it only extra locations from other scenes)

/*
void SetupLocations() {
    GameObject[] array = Resources.FindObjectsOfTypeAll<GameObject>();
    std::vector<Location> list;
    for (auto&& gameObject : array) {
        if (gameObject.name == "_Locations") {
            Location[] componentsInChildren = gameObject.GetComponentsInChildren<Location>(true);
            list.AddRange(componentsInChildren);
        }
    }
    std::vector<LocationList> allLocationLists = LocationList.GetAllLocationLists();
    allLocationLists.Sort((LocationList a, LocationList b) = > a.m_sortOrder.CompareTo(b.m_sortOrder));
    for (auto&& locationList : allauto && s) {
        m_locations.AddRange(locationList.m_locations);
        m_vegetation.AddRange(locationList.m_vegetation);
        for (auto&& env : locationList.m_environments) {
            EnvMan.instance.AppendEnvironment(env);
        }
        for (auto&& biomeEnv : locationList.m_biomeEnvironments) {
            EnvMan.instance.AppendBiomeSetup(biomeEnv);
        }
        ClutterSystem.instance.m_clutter.AddRange(locationList.m_clutter);
        LOG(INFO) << std::string.Format("Added {0} locations, {1} vegetations, {2} environments, {3} biome env-setups, {4} clutter  from ", new object[]{
            locationList.m_locations.Count,
            locationList.m_vegetation.Count,
            locationList.m_environments.Count,
            locationList.m_biomeEnvironments.Count,
            locationList.m_clutter.Count
            }) + locationList.gameObject.scene.name);
            RandEventSystem.instance.m_events.AddRange(locationList.m_events);
    }
    using (std::vector<Location>.Enumerator enumerator4 = list.GetEnumerator()) {
        while (enumerator4.MoveNext()) {
            if (enumerator4.Current.transform.gameObject.activeInHierarchy) {
                m_error = true;
            }
        }
    }
    for (auto&& zoneLocation : m_locations) {
        Transform transform = null;
        for (auto&& location : list) {
            if (location.gameObject.name == zoneLocation.m_prefabName) {
                transform = location.transform;
                break;
            }
        }
        if (!(transform == null) || zoneLocation.m_enable) {
            zoneLocation.m_prefab = transform.gameObject;
            zoneLocation.m_hash = zoneLocation.m_prefab.name.GetStableHashCode();
            Location componentInChildren = zoneLocation.m_prefab.GetComponentInChildren<Location>();
            zoneLocation.m_location = componentInChildren;
            zoneLocation.m_interiorRadius = (componentInChildren.m_hasInterior ? componentInChildren.m_interiorRadius : 0f);
            zoneLocation.m_exteriorRadius = componentInChildren.m_exteriorRadius;
            if (componentInChildren.m_interiorTransform && componentInChildren.m_generator) {
                zoneLocation.m_interiorPosition = componentInChildren.m_interiorTransform.localPosition;
                zoneLocation.m_generatorPosition = componentInChildren.m_generator.transform.localPosition;
            }
            if (Application.isPlaying) {
                ZoneSystem.PrepareNetViews(zoneLocation.m_prefab, zoneLocation.m_netViews);
                ZoneSystem.PrepareRandomSpawns(zoneLocation.m_prefab, zoneLocation.m_randomSpawns);
                if (!m_locationsByHash.ContainsKey(zoneLocation.m_hash)) {
                    m_locationsByHash.Add(zoneLocation.m_hash, zoneLocation);
                }
            }
        }
    }
}*/

/*
// public
void PrepareNetViews(GameObject root, std::vector<ZNetView> &views) {
    views.clear();
    for (auto&& znetView : root.GetComponentsInChildren<auto&&>(true)) {
        if (Utils.IsEnabledInheirarcy(znetView.gameObject, root)) {
            views.push_back(znetView);
        }
    }
}
// public
void PrepareRandomSpawns(GameObject root, std::vector<RandomSpawn> &randomSpawns) {
    randomSpawns.Clear();
    for (auto&& randomSpawn : root.GetComponentsInChildren<auto&&>(true)) {
        if (Utils.IsEnabledInheirarcy(randomSpawn.gameObject, root)) {
            randomSpawns.Add(randomSpawn);
            randomSpawn.Prepare();
        }
    }
}*/

// private
//void ValidateVegetation() {
//    for (auto&& zoneVegetation : m_vegetation) {
//        if (zoneVegetation.m_enable
//            && zoneVegetation.m_prefab
//            && zoneVegetation.m_prefab.GetComponent<ZNetView>() == null) {
//            LOG(ERROR) << "Vegetation is missing ZNetView (" << zoneVegetation.m_name << ")";
//            //ZLog.LogError(std::string.Concat(new std::string[]{
//            //    "Vegetation ",
//            //    zoneVegetation.m_prefab.name,
//            //    " [ ",
//            //    zoneVegetation.m_name,
//            //    "] is missing ZNetView"
//            //    }));
//        }
//    }
//}

// public
void IZoneManager::PrepareSave() {
    m_worldSave.Write<int32_t>(m_generatedZones.size());
    for (auto&& vec : m_generatedZones) {
        m_worldSave.Write(vec);
    }
    m_worldSave.Write(VConstants::PGW);
    m_worldSave.Write(m_locationVersion);
    m_worldSave.Write<int32_t>(m_globalKeys.size());
    for (auto&& key : m_globalKeys) {
        m_worldSave.Write(key);
    }
    m_worldSave.Write(true); // m_worldSave.Write(m_locationsGenerated);
    m_worldSave.Write<int32_t>(m_locationInstances.size());
    for (auto&& pair : m_locationInstances) {
        auto&& inst = pair.second;
        m_worldSave.Write(inst.m_location->m_prefabName);
        m_worldSave.Write(inst.m_position);
        m_worldSave.Write(inst.m_placed);
    }
}

// public
void IZoneManager::SaveAsync(NetPackage& writer) {
    writer.m_stream.Write(m_worldSave.m_stream.m_buf);
    m_worldSave.m_stream.Clear();
}

// public
void IZoneManager::Load(NetPackage& reader, int32_t version) {
    m_generatedZones.clear();
    int32_t num = reader.Read<int32_t>();
    for (int32_t i = 0; i < num; i++) {
        m_generatedZones.insert(reader.Read<Vector2i>());
    }
    if (version >= 13) {
        int32_t num2 = reader.Read<int32_t>();
        int32_t num3 = (version >= 21) ? reader.Read<int32_t>() : 0;
        if (num2 != VConstants::PGW) {
            m_generatedZones.clear();
        }
        if (version >= 14) {
            m_globalKeys.clear();
            int32_t num4 = reader.Read<int32_t>();
            for (int32_t j = 0; j < num4; j++) {
                std::string item2 = reader.Read<std::string>();
                m_globalKeys.insert(item2);
            }
        }
        if (version >= 18) {
            // kinda dumb, ?
            if (version >= 20) //m_locationsGenerated = reader.Read<bool>();
                reader.Read<bool>();
                    
            m_locationInstances.clear();
            int32_t num5 = reader.Read<int32_t>();
            for (int32_t k = 0; k < num5; k++) {
                auto text = reader.Read<std::string>();
                auto zero = reader.Read<Vector3>();

                bool generated = false;
                if (version >= 19)
                    generated = reader.Read<bool>();

                auto&& location = GetLocation(text);
                if (location) {
                    RegisterLocation(location, zero, generated);
                }
                else {
                    LOG(ERROR) << "Failed to find location " << text;
                    //ZLog.DevLog("Failed to find location " + text);
                }
            }
            LOG(INFO) << "Loaded " << num5 << " locations";
            if (num2 != VConstants::PGW) {
                m_locationInstances.clear();
                //m_locationsGenerated = false;
            }

            if (num3 != m_locationVersion) {
                //m_locationsGenerated = false;
            }
        }
    }
}

// private
void IZoneManager::Update() {
    PERIODIC_NOW(100ms, {
        UpdateTTL(.1f);

        for (auto&& znetPeer : NetManager::GetPeers()) {
            CreateGhostZones(znetPeer->m_pos);
        }
    });
}

// private
void IZoneManager::CreateGhostZones(const Vector3& refPoint) {
    Vector2i zone = WorldToZonePos(refPoint);

    int32_t num = m_activeArea + m_activeDistantArea;
    for (int32_t z = zone.y - num; z <= zone.y + num; z++) {
        for (int32_t x = zone.x - num; x <= zone.x + num; x++) {
            SpawnZone({x, z});
        }
    }

    //Vector2i zone = WorldToZonePos(refPoint);
    //if (!IsZoneGenerated(zone) && SpawnZone(zone)) {
    //    return true;
    //}
    //int32_t num = m_activeArea + m_activeDistantArea;
    //for (int32_t i = zone.y - num; i <= zone.y + num; i++) {
    //    for (int32_t j = zone.x - num; j <= zone.x + num; j++) {
    //        Vector2i zoneID(j, i);
    //        GameObject gameObject2;
    //        if (!IsZoneGenerated(zoneID) && SpawnZone(zoneID)) {
    //            return true;
    //        }
    //    }
    //}
    //return false;
}

// private
// not useful for server
/*
bool CreateLocalZones(const Vector3& refPoint) {
    Vector2i zone = GetZone(refPoint);
    if (PokeLocalZone(zone)) {
        return true;
    }
    for (int32_t i = zone.y - m_activeArea; i <= zone.y + m_activeArea; i++) {
        for (int32_t j = zone.x - m_activeArea; j <= zone.x + m_activeArea; j++) {
            Vector2i vector2i(j, i);
            if (!(vector2i == zone) && PokeLocalZone(vector2i)) {
                return true;
            }
        }
    }
    return false;
}
// private
bool PokeLocalZone(const Vector2i& zoneID) {
    ZoneData zoneData;
    if (m_zones.TryGetValue(zoneID, zoneData)) {
        zoneData.m_ttl = 0;
        return false;
    }
    auto&& mode = (!IsZoneGenerated(zoneID)) ? SpawnMode::Full : SpawnMode::Client;
    GameObject root;
    if (SpawnZone(zoneID, mode, root)) {
        ZoneData zoneData2;
        zoneData2.m_root = root;
        m_zones.insert({ zoneID, zoneData2 });
        return true;
    }
    return false;
}*/

// public
bool IZoneManager::IsZoneLoaded(const Vector3& point) {
    Vector2i zone = WorldToZonePos(point);
    return IsZoneLoaded(zone);
}

// public
bool IZoneManager::IsZoneLoaded(const Vector2i& zoneID) {
    return m_zones.contains(zoneID);
}

// public
// seemingly used by client only
/*
bool IsActiveAreaLoaded() {
    Vector2i zone = GetZone(ZNet.instance.GetReferencePosition());
    for (int32_t i = zone.y - m_activeArea; i <= zone.y + m_activeArea; i++) {
        for (int32_t j = zone.x - m_activeArea; j <= zone.x + m_activeArea; j++) {
            if (const !m_zones.ContainsKey(new & Vector2i(j, i))) {
                return false;
            }
        }
    }
    return true;
}*/

// private
// Only ever used in ghost mode
void IZoneManager::SpawnZone(const Vector2i& zoneID) {
    //Heightmap componentInChildren = m_zonePrefab.GetComponentInChildren<Heightmap>();

    // Wait for builder thread
    if (!(IsZoneGenerated(zoneID) && HeightmapBuilder::IsTerrainReady(zoneID))) {
        auto componentInChildren2 = HeightmapManager::CreateHeightmap(zoneID);

        std::vector<ClearArea> m_tempClearAreas;
        std::vector<GameObject> m_tempSpawnedObjects;
        PlaceLocations(zoneID, m_tempClearAreas, m_tempSpawnedObjects);
        PlaceVegetation(zoneID, componentInChildren2, m_tempClearAreas, m_tempSpawnedObjects);
        PlaceZoneCtrl(zoneID, m_tempSpawnedObjects);

        //if (mode == SpawnMode::Ghost) {
        for (auto&& obj : m_tempSpawnedObjects) {
            UnityEngine.Object.Destroy(obj); // unity-specific; must modify or remove/reconsiliate...
        }
        m_tempSpawnedObjects.clear();
        //UnityEngine.Object.Destroy(root);
        //root = null;

        m_generatedZones.insert(zoneID);
    }
}

// private
void IZoneManager::PlaceZoneCtrl(const Vector2i& zoneID, std::vector<GameObject>& spawnedObjects) {

    // ZoneCtrl contains
    //  SpawnSystem

    auto pos = ZoneToWorldPos(zoneID);
    // Create a zdo at that location
    // Idea: use the serialized zpackage prefab table to lookup zoneCtrl then instantiate
    Hashes::Objects::_ZoneCtrl


    //ZNetView.StartGhostInit(); // sets a priv var, but its never actually used; redundant
    GameObject gameObject = UnityEngine.Object.Instantiate<GameObject>(m_zoneCtrlPrefab, zoneCenterPos, Quaternion::IDENTITY);
    gameObject.GetComponent<ZNetView>().GetZDO().SetPGWVersion(VConstants::PGW);
    spawnedObjects.push_back(gameObject); // unity only
    //ZNetView.FinishGhostInit();

}

// private
Vector3 IZoneManager::GetRandomPointInRadius(VUtils::Random::State& state, const Vector3& center, float radius) {
    //VUtils::Random::State state;
    float f = state.NextFloat() * PI * 2.f;
    float num = state.Range(0.f, radius);
    return center + Vector3(sin(f) * num, 0, cos(f) * num);
}

// private
void IZoneManager::PlaceVegetation(const Vector2i &zoneID, Heightmap *hmap, std::vector<ClearArea>& clearAreas, std::vector<GameObject>& spawnedObjects) {
    //UnityEngine.Random.State state = UnityEngine.Random.state;

    const Vector3 zoneCenterPos = ZoneToWorldPos(zoneID);

    int32_t seed = WorldGenerator::GetSeed();
    float num = m_zoneSize / 2.f;
    int32_t num2 = 1;
    for (auto&& zoneVegetation : m_vegetation) {
        num2++;
        if (zoneVegetation.m_enable && hmap->HaveBiome(zoneVegetation.m_biome)) {

            VUtils::Random::State state(
                seed + zoneID.x * 4271 + zoneID.y * 9187 +
                VUtils::String::GetStableHashCode(zoneVegetation.m_prefab.name)
            );

            //UnityEngine.Random.InitState(seed + zoneID.x * 4271 + zoneID.y * 9187 + zoneVegetation.m_prefab.name.GetStableHashCode());
            int32_t num3 = 1;
            if (zoneVegetation.m_max < 1) {
                if (state.NextFloat() > zoneVegetation.m_max) {
                    continue;
                }
            }
            else {
                num3 = state.Range((int32_t)zoneVegetation.m_min, (int32_t)zoneVegetation.m_max + 1);
            }

            // flag should always be true, all vegetation seem to always have a NetView
            //bool flag = zoneVegetation.m_prefab.GetComponent<ZNetView>() != null;
            float num4 = cos((PI / 180.f) * zoneVegetation.m_maxTilt);
            float num5 = cos((PI / 180.f) * zoneVegetation.m_minTilt);
            float num6 = num - zoneVegetation.m_groupRadius;
            int32_t num7 = zoneVegetation.m_forcePlacement ? (num3 * 50) : num3;
            int32_t num8 = 0;
            for (int32_t i = 0; i < num7; i++) {

                Vector3 vector(state.Range(zoneCenterPos.x - num6, zoneCenterPos.x + num6),
                    0, state.Range(zoneCenterPos.z - num6, zoneCenterPos.z + num6));

                int32_t num9 = state.Range(zoneVegetation.m_groupSizeMin, zoneVegetation.m_groupSizeMax + 1);
                bool flag2 = false;
                for (int32_t j = 0; j < num9; j++) {

                    Vector3 vector2 = (j == 0) ? vector
                        : GetRandomPointInRadius(state, vector, zoneVegetation.m_groupRadius);

                    // random rotations
                    float rot_y = (float)state.Range(0, 360);
                    float scale = state.Range(zoneVegetation.m_scaleMin, zoneVegetation.m_scaleMax);
                    float rot_x = state.Range(-zoneVegetation.m_randTilt, zoneVegetation.m_randTilt);
                    float rot_z = state.Range(-zoneVegetation.m_randTilt, zoneVegetation.m_randTilt);

                    if (!zoneVegetation.m_blockCheck
                        || !IsBlocked(vector2)) {

                        Vector3 vector3;
                        Heightmap::Biome biome;
                        Heightmap::BiomeArea biomeArea;
                        Heightmap *otherHeightmap = GetGroundData(vector2, vector3, biome, biomeArea);

                        if ((std::to_underlying(zoneVegetation.m_biome) & std::to_underlying(biome))
                            && (std::to_underlying(zoneVegetation.m_biomeArea) & std::to_underlying(biomeArea))) {

                            float y2;
                            Vector3 vector4;
                            if (zoneVegetation.m_snapToStaticSolid && GetStaticSolidHeight(vector2, y2, vector4)) {
                                vector2.y = y2;
                                vector3 = vector4;
                            }
                            float num11 = vector2.y - m_waterLevel;
                            if (num11 >= zoneVegetation.m_minAltitude && num11 <= zoneVegetation.m_maxAltitude) {
                                if (zoneVegetation.m_minVegetation != zoneVegetation.m_maxVegetation) {
                                    float vegetationMask = otherHeightmap->GetVegetationMask(vector2);
                                    if (vegetationMask > zoneVegetation.m_maxVegetation || vegetationMask < zoneVegetation.m_minVegetation) {
                                        continue;
                                        //goto IL_501;
                                    }
                                }
                                if (zoneVegetation.m_minOceanDepth != zoneVegetation.m_maxOceanDepth) {
                                    float oceanDepth = otherHeightmap->GetOceanDepth(vector2);
                                    if (oceanDepth < zoneVegetation.m_minOceanDepth || oceanDepth > zoneVegetation.m_maxOceanDepth) {
                                        continue;
                                        //goto IL_501;
                                    }
                                }
                                if (vector3.y >= num4 && vector3.y <= num5) {
                                    if (zoneVegetation.m_terrainDeltaRadius > 0) {
                                        float num12;
                                        Vector3 vector5;
                                        GetTerrainDelta(vector2, zoneVegetation.m_terrainDeltaRadius, num12, vector5);
                                        if (num12 > zoneVegetation.m_maxTerrainDelta || num12 < zoneVegetation.m_minTerrainDelta) {
                                            continue;
                                            //goto IL_501;
                                        }
                                    }
                                    if (zoneVegetation.m_inForest) {
                                        float forestFactor = WorldGenerator::GetForestFactor(vector2);
                                        if (forestFactor < zoneVegetation.m_forestTresholdMin || forestFactor > zoneVegetation.m_forestTresholdMax) {
                                            continue;
                                            //goto IL_501;
                                        }
                                    }


                                    if (!InsideClearArea(clearAreas, vector2)) {
                                        if (zoneVegetation.m_snapToWater) {
                                            vector2.y = m_waterLevel;
                                        }
                                        vector2.y += zoneVegetation.m_groundOffset;
                                        Quaternion rotation = Quaternion::IDENTITY;



                                        /*
                                        * A large majority of the bottom portion is for
                                        * setting a custom non-1 scale of the object
                                        * it could be omitted for now to reduce complexity
                                        */

                                        // just omit this portion
                                        // or any scale-specific parts

                                        // Unity Quaternion Euler is not implemented in c#
                                        // LooKRotation is also internal

                                        if (zoneVegetation.m_chanceToUseGroundTilt > 0
                                            && state.NextFloat() <= zoneVegetation.m_chanceToUseGroundTilt) {
                                            //Quaternion rotation2 = Quaternion::Euler(0.f, rot_y, 0.f);
                                            //rotation = Quaternion.LookRotation(
                                            //    vector3.Cross(rotation2 * Vector3::FORWARD),
                                            //    vector3);
                                        }
                                        else {
                                            //rotation = Quaternion::Euler(rot_x, rot_y, rot_z);
                                        }



                                        // temp for simplicity
                                        rotation = Quaternion::Euler(rot_x, rot_y, rot_z);



                                        // has always been true in my minimal testing
                                        //if (flag) {
                                            // Always GHOST mode from ZoneSystem::Update CreateGhostZones call for all connected peers

                                            //if (mode == SpawnMode::Full || mode == SpawnMode::Ghost) {
                                                //if (mode == SpawnMode::Ghost) {
                                                //    ZNetView.StartGhostInit();
                                                //}



                                        GameObject gameObject = UnityEngine.Object.Instantiate<GameObject>(zoneVegetation.m_prefab, vector2, rotation);
                                        ZNetView component = gameObject.GetComponent<ZNetView>();
                                        component.GetNetSync()->m_pgwVersion = VConstants::PGW;
                                        if (scale != gameObject.transform.localScale.x) {

                                            // this does set the Unity gameobject localscale
                                            component.SetLocalScale(Vector3(scale, scale, scale));

                                            // idk what this is doing (might be a unity gimmick)
                                            //for (auto&& collider : gameObject.GetComponentsInChildren<Collider>()) {
                                            //    collider.enabled = false;
                                            //    collider.enabled = true;
                                            //}
                                        }

                                        //if (mode == SpawnMode::Ghost) {
                                        spawnedObjects.push_back(gameObject);
                                        //ZNetView.FinishGhostInit(); // redundant
                                    //}
                                //}
                            //}
                            //else {
                            //    GameObject gameObject2 = UnityEngine.Object.Instantiate<GameObject>(zoneVegetation.m_prefab, vector2, rotation);
                            //    gameObject2.transform.localScale = new Vector3(num10, num10, num10);
                            //    gameObject2.transform.SetParent(parent, true);
                            //}
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
    //UnityEngine.Random.state = state;
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
ZoneLocation* IZoneManager::GetLocation(int32_t hash) {
    auto&& find = m_locationsByHash.find(hash);
    if (find != m_locationsByHash.end())
        return find->second;

    //ZoneSystem.ZoneLocation result;
    //if (m_locationsByHash.TryGetValue(hash, result)) {
    //    return result;
    //}
    //return null;
}

// private
ZoneLocation* IZoneManager::GetLocation(const std::string& name) {
    for (auto&& zoneLocation : m_locations) {
        if (zoneLocation->m_prefabName == name) {
            return zoneLocation.get();
        }
    }
    return nullptr;
}

// private
void IZoneManager::ClearNonPlacedLocations() {
    for (auto&& itr = m_locationInstances.begin(); itr != m_locationInstances.end();) {
        if (!itr->second.m_placed)
            itr = m_locationInstances.erase(itr);
        else
            ++itr;
    }

    //robin_hood::unordered_map<Vector2i, LocationInstance> dictionary;
    //for (auto&& keyValuePair : m_locationInstances) {
    //    if (keyValuePair.second.m_placed) {
    //        dictionary.insert({ keyValuePair.first, keyValuePair.second });
    //    }
    //}
    //m_locationInstances = dictionary;
}

// private
void IZoneManager::CheckLocationDuplicates() {
    LOG(INFO) << "Checking for location duplicates";
    for (int32_t i = 0; i < m_locations.size(); i++) {
        auto&& zoneLocation = m_locations[i];
        if (zoneLocation->m_enable) {
            for (int32_t j = i + 1; j < m_locations.size(); j++) {
                auto&& zoneLocation2 = m_locations[j];
                if (zoneLocation2->m_enable
                    && zoneLocation->m_prefabName == zoneLocation2->m_prefabName) {
                    LOG(ERROR) << "Two locations points to the same location prefab " << zoneLocation->m_prefabName;
                }
            }
        }
    }
}

// public
// call from within ZNet.init or earlier...
void IZoneManager::GenerateLocations() {
    auto now(steady_clock::now());

    //m_locationsGenerated = true;

    CheckLocationDuplicates();
    ClearNonPlacedLocations();

    // Generate prioritized locations first
    for (auto&& loc : m_locations) {
        if (loc->m_prioritized)
            GenerateLocations(loc.get());
    }

    // Then generate unprioritized locations
    for (auto&& loc : m_locations) {
        if (!loc->m_prioritized)
            GenerateLocations(loc.get());
    }

    LOG(INFO) << "Location generation took " << duration_cast<milliseconds>(steady_clock::now() - now).count() << "ms";
}

// private
//int32_t CountNrOfLocation(ZoneLocation* location) {
//    int32_t count = 0;
//    for (auto&& loc : m_locationInstances) {
//        if (loc.second.m_location.m_prefabName == location->m_prefabName) {
//            count++;
//        }
//    }
//
//    if (count > 0) {
//        LOG(INFO) << "Old location found " << location->m_prefabName << " (" << count << ")";
//    }
//
//    return count;
//}

// private
void IZoneManager::GenerateLocations(ZoneLocation* location) {
    VUtils::Random::State state(WorldGenerator::GetSeed() + VUtils::String::GetStableHashCode(location->m_prefabName));
    const float locationRadius = std::max(location->m_exteriorRadius, location->m_interiorRadius);
    unsigned int spawnedLocations = 0;

    unsigned int errLocations = 0;
    unsigned int errCenterDistances = 0;
    unsigned int errNoneBiomes = 0;
    unsigned int errBiomeArea = 0;
    unsigned int errAltitude = 0;
    unsigned int errForestFactor = 0;
    unsigned int errSimilarLocation = 0;
    unsigned int errTerrainDelta = 0;

    for (auto&& inst : m_locationInstances) {
        if (inst.second.m_location->m_prefabName == location->m_prefabName)
            spawnedLocations++;
    }
    if (spawnedLocations)
        LOG(INFO) << "Old location found " << location->m_prefabName << " x " << spawnedLocations;

    float range = location->m_centerFirst ? location->m_minDistance : 10000;

    if (location->m_unique && spawnedLocations)
        return;

    const unsigned int spawnAttempts = location->m_prioritized ? 200000 : 100000;
    for (unsigned int a = 0; a < spawnAttempts && spawnedLocations < location->m_quantity; a++) {
        Vector2i randomZone = GetRandomZone(state, range);
        if (location->m_centerFirst)
            range++;

        if (m_locationInstances.contains(randomZone))
            errLocations++;
        else { // pointless condition
            //assert(m_generatedZones.contains(randomZone) && "Tried regenerating zone");

            Vector3 zonePos = ZoneToWorldPos(randomZone);
            Heightmap::BiomeArea biomeArea = WorldGenerator::GetBiomeArea(zonePos);
            //if ((BitMask<location->m_biomeArea> & biomeArea) == (Heightmap::BiomeArea)0)
            if (std::to_underlying(location->m_biomeArea) & std::to_underlying(biomeArea))
                errBiomeArea++;
            else {
                for (int i = 0; i < 20; i++) {

                    // generate point in zone
                    float num = ZONE_SIZE / 2.f;
                    float x = state.Range(-num + locationRadius, num - locationRadius);
                    float z = state.Range(-num + locationRadius, num - locationRadius);
                    Vector3 randomPointInZone = zonePos + Vector3(x, 0, z);



                    float magnitude = randomPointInZone.Magnitude(); assert(magnitude >= 0);
                    if (magnitude < location->m_minDistance || magnitude > location->m_maxDistance)
                        errCenterDistances++;
                    else {
                        Heightmap::Biome biome = WorldGenerator::GetBiome(randomPointInZone);
                        //if ((location->m_biome & biome) == Heightmap::Biome::None)
                        if (!(std::to_underlying(biome) & std::to_underlying(location->m_biome)))
                            errNoneBiomes++;
                        else {
                            randomPointInZone.y = WorldGenerator::GetHeight(randomPointInZone.x, randomPointInZone.z);
                            float waterDiff = randomPointInZone.y - WATER_LEVEL;
                            if (waterDiff < location->m_minAltitude || waterDiff > location->m_maxAltitude)
                                errAltitude++;
                            else {
                                if (location->m_inForest) {
                                    float forestFactor = WorldGenerator::GetForestFactor(randomPointInZone);
                                    if (forestFactor < location->m_forestTresholdMin || forestFactor > location->m_forestTresholdMax) {
                                        errForestFactor++;
                                        continue;
                                    }
                                }

                                float delta = 0;
                                Vector3 vector;
                                WorldGenerator::GetTerrainDelta(state, randomPointInZone, location->m_exteriorRadius, delta, vector);
                                if (delta > location->m_maxTerrainDelta
                                    || delta < location->m_minTerrainDelta)
                                    errTerrainDelta++;
                                else {
                                    if (location->m_minDistanceFromSimilar <= 0) {

                                        // HaveLocationInRange: inlined
                                        bool locInRange = false;
                                        for (auto&& inst : m_locationInstances) {
                                            auto&& loc = inst.second.m_location;
                                            if ((loc->m_prefabName == location->m_prefabName
                                                || (!location->m_group.empty() && location->m_group == loc->m_group))
                                                && inst.second.m_position.Distance(randomPointInZone) < location->m_minDistanceFromSimilar)
                                            {
                                                locInRange = true;
                                                break;
                                            }
                                        }

                                        if (!locInRange) {
                                            RegisterLocation(location, randomPointInZone, false);
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
        LOG(ERROR) << "Failed to place all " << location->m_prefabName << ", placed " << spawnedLocations << "/" << location->m_quantity;

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
    std::vector<ClearArea>& clearAreas,
    //SpawnMode mode, 
    std::vector<GameObject>& spawnedObjects)
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
        Heightmap::Biome biome = Heightmap::Biome::None;
        Heightmap::BiomeArea biomeArea;
        Heightmap *heightmap = GetGroundData(position, vector, biome, biomeArea);

        if (locationInstance.m_location->m_snapToWater) {
            position.y = m_waterLevel;
        }
        if (locationInstance.m_location->m_location.m_clearArea.m_radius) {
            ClearArea item(position, locationInstance.m_location->m_exteriorRadius);
            clearAreas.push_back(item);
        }

        assert(false);

        Quaternion rot(Quaternion::IDENTITY);
        if (locationInstance.m_location->m_slopeRotation) {
            float num;
            Vector3 vector2;
            GetTerrainDelta(position, locationInstance.m_location->m_exteriorRadius, num, vector2);
            Vector3 forward(vector2.x, 0, vector2.z);
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

        int32_t seed = WorldGenerator::GetSeed() + zoneID.x * 4271 + zoneID.y * 9187;
        SpawnLocation(locationInstance.m_location, seed, position, rot, spawnedObjects);
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
            SendLocationIcons(IManagerRoute::EVERYBODY);
        }
    }
}

// private
void IZoneManager::RemoveUnplacedLocations(ZoneLocation* location) {
    std::vector<Vector2i> list;
    for (auto&& keyValuePair : m_locationInstances) {
        if (keyValuePair.second.m_location == location && !keyValuePair.second.m_placed) {
            list.push_back(keyValuePair.first);
        }
    }
    for (auto&& key : list) {
        m_locationInstances.erase(key);
    }
    //ZLog.DevLog("Removed " + list.Count.ToString() + " unplaced locations of type " + location.m_prefabName);
    LOG(INFO) << "Removed " << list.size() << " unplaced locations of type " << location->m_prefabName;
}

// client only
/*
// public
bool TestSpawnLocation(const std::string& name, const Vector3& pos, bool disableSave = true) {
    ZoneLocation location = GetLocation(name);
    if (location == null) {
        LOG(INFO) << "Missing location:" << name;
        global::Console.instance.Print("Missing location:" + name);
        return false;
    }
    if (location.m_prefab == null) {
        LOG(INFO) << "Missing prefab in location:" << name;
        global::Console.instance.Print("Missing location:" + name);
        return false;
    }
    float num = std::max(location.m_exteriorRadius, location.m_interiorRadius);
    Vector2i zone = GetZone(pos);
    Vector3 zonePos = GetZonePos(zone);
    pos.x = VUtils::Math::Clamp(pos.x, zonePos.x - m_zoneSize / 2.f + num, zonePos.x + m_zoneSize / 2.f - num);
    pos.z = VUtils::Math::Clamp(pos.z, zonePos.z - m_zoneSize / 2.f + num, zonePos.z + m_zoneSize / 2.f - num);
    std::string[] array = new std::string[6];
    array[0] = "radius ";
    array[1] = num.ToString();
    array[2] = "  ";
    int32_t num2 = 3;
    Vector3 vector = zonePos;
    array[num2] = vector.ToString();
    array[4] = " ";
    int32_t num3 = 5;
    vector = pos;
    array[num3] = vector.ToString();
    LOG(INFO) << std::string.Concat(array);
    MessageHud.instance.ShowMessage(MessageHud.MessageType.Center, "Location spawned, " + (disableSave ? "world saving DISABLED until restart" : "CAUTION! world saving is ENABLED, use normal location command to disable it!"), 0, null);

    m_didZoneTest = disableSave;
    float y = (float)UnityEngine.Random.Range(0, 16) * 22.5f;
    std::vector<GameObject> spawnedGhostObjects;
    SpawnLocation(location, UnityEngine.Random.Range(0, 99999), pos, Quaternion.Euler(0f, y, 0f), ZoneSystem.SpawnMode.Full, spawnedGhostObjects);
    return true;
}*/

// public
// the client uses this
//GameObject SpawnProxyLocation(int32_t hash, int32_t seed, const Vector3& pos, const Quaternion& rot) {
//    auto&& location = GetLocation(hash);
//    if (!location) {
//        LOG(ERROR) << "Missing location:" + hash;
//        return null;
//    }
//    std::vector<GameObject> spawnedGhostObjects;
//    return SpawnLocation(location, seed, pos, rot, SpawnMode::Client, spawnedGhostObjects);
//}

// private
GameObject IZoneManager::SpawnLocation(ZoneLocation* location, int32_t seed, Vector3 pos, Quaternion rot, std::vector<GameObject>& spawnedGhostObjects) {

    throw std::runtime_error("not implemented");

    /*
    location->m_prefab.transform.position = Vector3::ZERO;
    location->m_prefab.transform.rotation = Quaternion::IDENTITY;
    //UnityEngine.Random.InitState(seed);

        

    Location component = location.m_prefab.GetComponent<Location>();
    bool flag = component && component.m_useCustomInteriorTransform && component.m_interiorTransform && component.m_generator;
    if (flag) {
        Vector2i zone = WorldToZonePos(pos);
        Vector3 zonePos = ZoneToWorldPos(zone);
        component.m_generator.transform.localPosition = Vector3::ZERO;
        Vector3 vector = zonePos + location.m_interiorPosition + location.m_generatorPosition - pos;
        Vector3 localPosition = (Matrix4x4.Rotate(Quaternion.Inverse(rot)) * Matrix4x4.Translate(vector)).GetColumn(3);
        localPosition.y = component.m_interiorTransform.localPosition.y;
        component.m_interiorTransform.localPosition = localPosition;
        component.m_interiorTransform.localRotation = Quaternion.Inverse(rot);
    }

    if (component
        && component.m_generator
        && component.m_useCustomInteriorTransform != component.m_generator.m_useCustomInteriorTransform) {
        LOG(ERROR) << component.name << " & " + component.m_generator.name << " don't have matching m_useCustomInteriorTransform()! If one has it the other should as well!";
    }

    //if (mode == SpawnMode::Full || mode == SpawnMode::Ghost) {
        for (auto&& znetView : location.m_netViews) {
            znetView.gameObject.SetActive(true);
        }
        //UnityEngine.Random.InitState(seed);

        //state = VUtils::Random::State(seed);

        VUtils::Random::State state(seed);

        for (auto&& randomSpawn : location.m_randomSpawns) {
            randomSpawn.Randomize();
        }

        WearNTear.m_randomInitialDamage = location.m_location.m_applyRandomDamage;
        for (auto&& znetView2 : location.m_netViews) {
            if (znetView2.gameObject.activeSelf) {
                Vector3 position = znetView2.gameObject.transform.position;
                Vector3 position2 = pos + rot * position;
                Quaternion rotation = znetView2.gameObject.transform.rotation;
                Quaternion rotation2 = rot * rotation;

                //if (mode == SpawnMode::Ghost) {
                //    ZNetView.StartGhostInit(); // redundant; does practically nothing
                //}

                GameObject gameObject = UnityEngine.Object.Instantiate<GameObject>(znetView2.gameObject, position2, rotation2);
                gameObject.GetComponent<ZNetView>().GetZDO().SetPGWVersion(m_pgwVersion);
                DungeonGenerator component2 = gameObject.GetComponent<DungeonGenerator>();
                if (component2) {
                    if (flag) {
                        component2.m_originalPosition = location.m_generatorPosition;
                    }
                    component2.Generate(mode);
                }

                //if (mode == SpawnMode::Ghost) {
                    spawnedGhostObjects.Add(gameObject);
                    //ZNetView.FinishGhostInit();
                //}
            }
        }
        WearNTear.m_randomInitialDamage = false;
        CreateLocationProxy(location, seed, pos, rot, mode, spawnedGhostObjects);
        SnapToGround.SnappAll();
        return null;
    //}*/
        
    //UnityEngine.Random.InitState(seed);
    //for (auto&& randomSpawn2 : location.m_randomSpawns) {
    //    randomSpawn2.Randomize();
    //}
    //for (auto&& znetView3 : location.m_netViews) {
    //    znetView3.gameObject.SetActive(false);
    //}
    //GameObject gameObject2 = UnityEngine.Object.Instantiate<GameObject>(location.m_prefab, pos, rot);
    //gameObject2.SetActive(true);
    //SnapToGround.SnappAll();
    //return gameObject2;
}

// could be inlined...
// private
void IZoneManager::CreateLocationProxy(ZoneLocation* location, int32_t seed, Vector3 pos, Quaternion rotation, SpawnMode mode, std::vector<GameObject>& spawnedGhostObjects) {
    //if (mode == SpawnMode::Ghost) {
    //    ZNetView.StartGhostInit();
    //}
        
    GameObject gameObject = UnityEngine.Object.Instantiate<GameObject>(m_locationProxyPrefab, pos, rotation);
    LocationProxy component = gameObject.GetComponent<LocationProxy>();
    bool spawnNow = mode == ZoneSystem.SpawnMode.Full;
    component.SetLocation(location.m_prefab.name, seed, spawnNow, Version::PGW);
    //if (mode == ZoneSystem.SpawnMode.Ghost) {
        spawnedGhostObjects.push_back(gameObject);
        //ZNetView.FinishGhostInit();
    //}
}

// private
void IZoneManager::RegisterLocation(ZoneLocation* location, const Vector3& pos, bool generated) {
    LocationInstance value = default(ZoneSystem.LocationInstance);
    value.m_location = location;
    value.m_position = pos;
    value.m_placed = generated;
    Vector2i zone = GetZone(pos);
    if (m_locationInstances.contains(zone)) {
        //std::string str = "Location already exist in zone ";
        //Vector2i vector2i = zone;
        //ZLog.LogWarning(str + vector2i.ToString());
        LOG(ERROR) << "Location already exist in zone " << zone.x << " " << zone.y;
        return;
    }
    m_locationInstances.insert({ zone, value }); // TODO this can be optimized (test return val)
}

// private
bool IZoneManager::HaveLocationInRange(const std::string& prefabName, const std::string& group, const Vector3& p, float radius) {

    for (auto&& inst : m_locationInstances) {
        if ((inst.second.m_location->m_prefabName == prefabName
            || (!group.empty() && group == inst.second.m_location->m_group))
            && inst.second.m_position.Distance(p) < radius) {
            return true;
        }
    }
    return false;

    //for (auto&& locationInstance : m_locationInstances.Values) {
    //    if ((locationInstance.m_location.m_prefabName == prefabName || (group.Length > 0 && group == locationInstance.m_location.m_group)) && Vector3.Distance(locationInstance.m_position, p) < radius) {
    //        return true;
    //    }
    //}
    //return false;
}

// public
bool IZoneManager::GetLocationIcon(const std::string& name, Vector3& pos) {
    // iterate map looking
    for (auto&& pair : m_locationInstances) {
        auto&& loc = pair.second;
        if ((loc.m_location->m_iconAlways
            || (loc.m_location->m_iconPlaced && loc.m_placed))
            && loc.m_location->m_prefabName == name) {
            pos = loc.m_position;
            return true;
        }
    }

    pos = Vector3::ZERO;
    return false;

    /*
    using (robin_hood::unordered_map<Vector2i, ZoneSystem.LocationInstance>.Enumerator enumerator = m_locationInstances.GetEnumerator()) {
        while (enumerator.MoveNext()) {
            KeyValuePair<Vector2i, ZoneSystem.LocationInstance> keyValuePair = enumerator.Current;
            if ((keyValuePair.Value.m_location.m_iconAlways
                || (keyValuePair.Value.m_location.m_iconPlaced && keyValuePair.Value.m_placed))
                && keyValuePair.Value.m_location.m_prefabName == name) {
                pos = keyValuePair.Value.m_position;
                return true;
            }
        }
    }

    pos = Vector3.zero;
    return false;*/
}

// public
void IZoneManager::GetLocationIcons(robin_hood::unordered_map<Vector3, std::string> icons) {
    for (auto&& pair : m_locationInstances) {
        auto&& loc = pair.second;
        if (loc.m_location->m_iconAlways
            || (loc.m_location->m_iconPlaced && loc.m_placed))
        {
            icons[pair.second.m_position] = loc.m_location->m_prefabName;
        }
    }

    /*
    using (robin_hood::unordered_map<Vector2i, ZoneSystem.LocationInstance>.ValueCollection.Enumerator enumerator = m_locationInstances.Values.GetEnumerator()) {
        while (enumerator.MoveNext()) {
            ZoneSystem.LocationInstance locationInstance = enumerator.Current;
            if (locationInstance.m_location.m_iconAlways
                || (locationInstance.m_location.m_iconPlaced && locationInstance.m_placed)) {
                icons[locationInstance.m_position] = locationInstance.m_location.m_prefabName;
            }
        }
        return;
    }*/
}

// private
void IZoneManager::GetTerrainDelta(const Vector3& center, float& radius, float& delta, Vector3& slopeDirection) {
    float num2 = -999999;
    float num3 = 999999;
    Vector3 b = center;
    Vector3 a = center;
    VUtils::Random::State state;
    for (int32_t i = 0; i < 10; i++) {
        Vector2 vector = state.GetRandomUnitCircle() * radius;
        Vector3 vector2 = center + Vector3(vector.x, 0, vector.y);
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
bool IZoneManager::IsBlocked(const Vector3& p) {
    p.y += 2000;
    return Physics.Raycast(p, Vector3::DOWN, 10000, m_blockRayMask);
}

// public
float IZoneManager::GetAverageGroundHeight(const Vector3& p, float radius) {
    Vector3 origin = p;
    origin.y = 6000;
    RaycastHit raycastHit;
    if (Physics.Raycast(origin, Vector3::DOWN, raycastHit, 10000, m_terrainRayMask)) {
        return raycastHit.point.y;
    }
    return p.y;
}

// public
float IZoneManager::GetGroundHeight(const Vector3& p) {
    Vector3 origin = p;
    origin.y = 6000;
    RaycastHit raycastHit;
    if (Physics.Raycast(origin, Vector3::DOWN, raycastHit, 10000, m_terrainRayMask)) {
        return raycastHit.point.y;
    }
    return p.y;
}

// public
bool IZoneManager::GetGroundHeight(const Vector3& p, float& height) {
    p.y = 6000;
    RaycastHit raycastHit;
    if (Physics.Raycast(p, Vector3::DOWN, raycastHit, 10000, m_terrainRayMask)) {
        height = raycastHit.point.y;
        return true;
    }
    height = 0f;
    return false;
}

// public
float IZoneManager::GetSolidHeight(const Vector3& p) {
    Vector3 origin = p;
    origin.y += 1000;
    RaycastHit raycastHit;
    if (Physics.Raycast(origin, Vector3::DOWN, raycastHit, 2000, m_solidRayMask)) {
        return raycastHit.point.y;
    }
    return p.y;
}

// public
bool IZoneManager::GetSolidHeight(const Vector3& p, float& height, int32_t heightMargin = 1000) {
    p.y += (float)heightMargin;
    RaycastHit raycastHit;
    if (Physics.Raycast(p, Vector3::DOWN, raycastHit, 2000, m_solidRayMask)
        && !raycastHit.collider.attachedRigidbody) {
        height = raycastHit.point.y;
        return true;
    }
    height = 0;
    return false;
}

// public
bool IZoneManager::GetSolidHeight(const Vector3& p, float& radius, float height, Transform ignore) {
    height = p.y - 1000;
    p.y += 1000;
    int32_t num;
    if (radius <= 0) {
        num = Physics.RaycastNonAlloc(p, Vector3::DOWN, rayHits, 2000, m_solidRayMask);
    }
    else {
        num = Physics.SphereCastNonAlloc(p, radius, Vector3::DOWN, rayHits, 2000, m_solidRayMask);
    }
    bool result = false;
    for (int32_t i = 0; i < num; i++) {
        RaycastHit raycastHit = rayHits[i];
        Collider collider = raycastHit.collider;
        if (!(collider.attachedRigidbody != null)
            && (!(ignore != null) || !Utils.IsParent(collider.transform, ignore))) {
            if (raycastHit.point.y > height) {
                height = raycastHit.point.y;
            }
            result = true;
        }
    }
    return result;
}

// public
bool IZoneManager::GetSolidHeight(const Vector3& p, float& height, const Vector3& normal) {
    GameObject gameObject;
    return GetSolidHeight(p, height, normal, gameObject);
}

// public
bool IZoneManager::GetSolidHeight(const Vector3& p, float& height, const Vector3& normal, GameObject go) {
    p.y += 1000;
    RaycastHit raycastHit;
    if (Physics.Raycast(p, Vector3::DOWN, raycastHit, 2000, m_solidRayMask)
        && !raycastHit.collider.attachedRigidbody) {
        height = raycastHit.point.y;
        normal = raycastHit.normal;
        go = raycastHit.collider.gameObject;
        return true;
    }
    height = 0;
    normal = Vector3::ZERO;
    go = null;
    return false;
}

// public
bool IZoneManager::GetStaticSolidHeight(const Vector3& p, float& height, const Vector3& normal) {
    p.y += 1000;
    RaycastHit raycastHit;
    if (Physics.Raycast(p, Vector3::DOWN, raycastHit, 2000, m_staticSolidRayMask)
        && !raycastHit.collider.attachedRigidbody) {
        height = raycastHit.point.y;
        normal = raycastHit.normal;
        return true;
    }
    height = 0;
    normal = Vector3::ZERO;
    return false;
}

// public
bool IZoneManager::FindFloor(const Vector3& p, float& height) {
    RaycastHit raycastHit;
    if (Physics.Raycast(p + Vector3::UP * 1, Vector3::DOWN, raycastHit, 1000, m_solidRayMask)) {
        height = raycastHit.point.y;
        return true;
    }
    height = 0;
    return false;
}

// public
// if terrain is just heightmap,
// could easily create a wrapper and poll points where needed
Heightmap* IZoneManager::GetGroundData(Vector3& p, Vector3& normal, Heightmap::Biome& biome, Heightmap::BiomeArea& biomeArea) {
    biome = Heightmap::Biome::None;
    biomeArea = Heightmap::BiomeArea::Everything;
    //hmap = null;

    // test collision from point, casting downwards through terrain

    // If final result is completely linked to Heightmap, could just use heightmap
    // should be simple enough,

    // global heightmaps can be queried at the world--->zone then 
    // get the relative point inside zone

    auto heightmap = HeightmapManager::FindHeightmap(p);
    if (heightmap) {
        HeightmapManager::GetHeight(p, p.y);
        return heightmap;
    }



    RaycastHit raycastHit;
    if (Physics.Raycast(p + Vector3::UP * 5000, Vector3::DOWN, raycastHit, 10000, m_terrainRayMask)) {
        p.y = raycastHit.point.y;
        normal = raycastHit.normal;
        Heightmap component = raycastHit.collider.GetComponent<Heightmap>();
        if (component) {
            biome = component.GetBiome(raycastHit.point);
            biomeArea = component.GetBiomeArea();
            //hmap = component;
            return component;
        }
    }
    else
        normal = Vector3::UP;

    return nullptr;
}

// private
void IZoneManager::UpdateTTL(float dt) {
    for (auto&& keyValuePair : m_zones) {
        keyValuePair.second.m_ttl += dt;
    }

    for (auto&& keyValuePair2 : m_zones) {
        if (keyValuePair2.second.m_ttl > m_zoneTTL && !NetScene::HaveInstanceInSector(keyValuePair2.first)) {
            UnityEngine.Object.Destroy(keyValuePair2.second.m_root);
            m_zones.erase(keyValuePair2.first);
            break;
        }
    }
}

// public
bool IZoneManager::FindClosestLocation(const std::string& name, const Vector3& point, LocationInstance& closest) {
    float num = 999999;
    closest = default(ZoneSystem.LocationInstance);
    bool result = false;

    for (auto&& pair : m_locationInstances) {
        auto&& loc = pair.second;
        float num2 = loc.m_position.Distance(point);
        if (loc.m_location->m_prefabName == name && num2 < num) {
            num = num2;
            closest = loc;
            result = true;
        }
    }

    //for (auto&& locationInstance : m_locationInstances.Values) {
    //    float num2 = locationInstance.m_position, point);
    //    if (locationInstance.m_location.m_prefabName == name && num2 < num) {
    //        num = num2;
    //        closest = locationInstance;
    //        result = true;
    //    }
    //}

    return result;
}

// public
// this is world position to zone position
// formerly GetZone
Vector2i IZoneManager::WorldToZonePos(const Vector3& point) {
    int32_t x = floor((point.x + m_zoneSize / 2) / m_zoneSize);
    int32_t y = floor((point.z + m_zoneSize / 2) / m_zoneSize);
    return Vector2i(x, y);
}

// public
// zone position to ~world position
// GetZonePos
Vector3 IZoneManager::ZoneToWorldPos(const Vector2i& id) {
    return Vector3(id.x * m_zoneSize, 0, id.y * m_zoneSize);
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
bool IZoneManager::SkipSaving() {
    return m_error || m_didZoneTest;
}

// public
//float TimeSinceStart() {
//    return m_lastFixedTime - m_startTime;
//}

// public
void IZoneManager::ResetGlobalKeys() {
    m_globalKeys.clear();
    SendGlobalKeys(NetRouteManager::EVERYBODY);
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


