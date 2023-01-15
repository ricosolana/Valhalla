#include <ranges>
#include <thread>

#include "NetManager.h"
#include "ZoneSystem.h"
#include "WorldGenerator.h"
#include "NetScene.h"
#include "HeightmapBuilder.h"

namespace ZoneSystem {

    // Dummy forward declarations
    struct Location {};
    struct RandomSpawn {};




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
        ZoneLocation(const ZoneLocation& other) {
            // clone
        }

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

    //void Awake();

    void Start();
    void SendGlobalKeys(OWNER_t peer);
    void SendLocationIcons(OWNER_t peer);
    void OnNewPeer(OWNER_t peerID);
    //void SetupLocations();
    //void ValidateVegetation();
    void Update();
    bool CreateGhostZones(const Vector3& refPoint);
    //bool CreateLocalZones(const Vector3& refPoint);
    //bool PokeLocalZone(const Vector2i& zoneID);

    bool SpawnZone(const Vector2i& zoneID);
    void PlaceZoneCtrl(Vector2i zoneID, Vector3 zoneCenterPos, std::vector<GameObject>& spawnedObjects);
    void PlaceVegetation(Vector2i zoneID, Vector3 zoneCenterPos, Heightmap hmap, std::vector<ClearArea>& clearAreas, std::vector<GameObject>& spawnedObjects);
    void PlaceLocations(Vector2i zoneID, Vector3 zoneCenterPos, Transform parent, std::vector<ClearArea>& clearAreas, std::vector<GameObject>& spawnedObjects);

    Vector3 GetRandomPointInRadius(VUtils::Random::State& state, const Vector3& center, float radius);
    bool InsideClearArea(const std::vector<ClearArea>& areas, const Vector3& point);
    ZoneLocation* GetLocation(int32_t hash);
    ZoneLocation* GetLocation(const std::string& name);
    void ClearNonPlacedLocations();
    void CheckLocationDuplicates();
    int32_t CountNrOfLocation(ZoneLocation* location);
    //void GenerateLocations(ZoneLocation location);
    //Vector2i GetRandomZone(float range);
    //Vector3 GetRandomPointInZone(const Vector2i& zone, float locationRadius);
    //Vector3 GetRandomPointInZone(float locationRadius);

    void RemoveUnplacedLocations(ZoneLocation* location);
    GameObject SpawnLocation(ZoneLocation* location, int32_t seed, Vector3 pos, Quaternion rot, SpawnMode mode, std::vector<GameObject>& spawnedGhostObjects);
    void CreateLocationProxy(ZoneLocation* location, int32_t seed, Vector3 pos, Quaternion rotation, SpawnMode mode, std::vector<GameObject>& spawnedGhostObjects);
    void RegisterLocation(ZoneLocation* location, const Vector3& pos, bool generated);
    bool HaveLocationInRange(const std::string& prefabName, const std::string& group, const Vector3& p, float radius);
    void GetTerrainDelta(const Vector3& center, float& radius, float& delta, Vector3& slopeDirection);
    void UpdateTTL(float dt);
    void SetZoneGenerated(const Vector2i& zoneID);
    bool IsZoneGenerated(const Vector2i& zoneID);
    void RPC_SetGlobalKey(OWNER_t sender, std::string name);
    void RPC_RemoveGlobalKey(OWNER_t sender, std::string name);



    // All public vars, however a LOT of them are editor visible vars
    //[HideInInspector]
    std::vector<Heightmap::Biome> m_biomeFolded;

    //[HideInInspector]
    std::vector<Heightmap::Biome> m_vegetationFolded;

    //[HideInInspector]
    std::vector<Heightmap::Biome> m_locationFolded;

    //[NonSerialized]
    bool m_drawLocations;

    //[NonSerialized]
    std::string m_drawLocationsFilter = "";

    //[global::Tooltip("Zones to load around center sector")]
    int32_t m_activeArea = 1;

    int32_t m_activeDistantArea = 1;

    //[global::Tooltip("Zone size, should match netscene sector size")]
    float m_zoneSize = 64;

    //[global::Tooltip("Time before destroying inactive zone")]
    float m_zoneTTL = 4;

    //[global::Tooltip("Time before spawning active zone")]
    float m_zoneTTS = 4;

    GameObject m_zonePrefab;

    GameObject m_zoneCtrlPrefab;

    GameObject m_locationProxyPrefab;

    float m_waterLevel = 30;

    //[Header("Versions")]
    //int32_t m_pgwVersion = 53;

    int32_t m_locationVersion = 1;

    //[Header("Generation data")]
    std::vector<std::string> m_locationScenes;

    std::vector<ZoneVegetation> m_vegetation;

    std::vector<std::unique_ptr<ZoneLocation>> m_locations;

    bool m_didZoneTest;

    //[HideInInspector]
    robin_hood::unordered_map<Vector2i, LocationInstance> m_locationInstances;




    // Private vars

    robin_hood::unordered_map<Vector3, std::string> tempIconList;

    //RaycastHit[] rayHits = new RaycastHit[200];

    robin_hood::unordered_map<int, ZoneLocation*> m_locationsByHash;

    bool m_error;

    int32_t m_terrainRayMask;

    int32_t m_blockRayMask;

    int32_t m_solidRayMask;

    int32_t m_staticSolidRayMask;

    float m_updateTimer;

    // doesnt appear to be used for anything significant
    //float m_startTime;
    //float m_lastFixedTime;

    robin_hood::unordered_map<Vector2i, ZoneData> m_zones;

    robin_hood::unordered_set<Vector2i> m_generatedZones;

    bool m_locationsGenerated;

    robin_hood::unordered_map<Vector3, std::string> m_locationIcons;

    robin_hood::unordered_set<std::string> m_globalKeys;



    NetPackage m_worldSave;
    /*
    robin_hood::unordered_set<Vector2i> m_tempGeneratedZonesSaveClone;
    robin_hood::unordered_set<std::string> m_tempGlobalKeysSaveClone;
    std::vector<LocationInstance> m_tempLocationsSaveClone;
    bool m_tempLocationsGeneratedSaveClone;
    */
    // TODO not used for anything complex; just another reuse var...
    // make a local or static...




    /*
    // private
    void Awake() {
        m_terrainRayMask = LayerMask.GetMask(new std::string[]{
            "terrain"
            });
        m_blockRayMask = LayerMask.GetMask(new std::string[]{
            "Default",
            "static_solid",
            "Default_small",
            "piece"
            });
        m_solidRayMask = LayerMask.GetMask(new std::string[]{
            "Default",
            "static_solid",
            "Default_small",
            "piece",
            "terrain"
            });
        m_staticSolidRayMask = LayerMask.GetMask(new std::string[]{
            "static_solid",
            "terrain"
            });
        for (auto&& text : m_locationScenes) {
            if (SceneManager.GetSceneByName(text).IsValid()) {
                LOG(INFO) << "Location scene " << text << " already loaded";
            }
            else {
                SceneManager.LoadScene(text, LoadSceneMode.Additive);
            }
        }
        LOG(INFO) << "Zonesystem Awake " << Time.frameCount.ToString();
    }
    */

    // private
    void Start() {
        //LOG(INFO) << "Zonesystem Start " << Time.frameCount.ToString();
        LOG(INFO) << "ZoneSystem Start ";
        //SetupLocations();
        //ValidateVegetation();
        //ZRoutedRpc instance = ZRoutedRpc.instance;
        //instance.m_onNewPeer = (Action<long>)Delegate.Combine(instance.m_onNewPeer, new Action<long>(OnNewPeer));
        NetRouteManager::Register("SetGlobalKey", RPC_SetGlobalKey);
        NetRouteManager::Register("RemoveGlobalKey", RPC_RemoveGlobalKey);
        //m_startTime = (m_lastFixedTime = Time.fixedTime);
    }

    // public

    //void GenerateLocationsIfNeeded() {
    //    if (!m_locationsGenerated) {
    //        GenerateLocations();
    //    }
    //}

    // private
    void SendGlobalKeys(OWNER_t peer) {
        //std::vector<std::string> list = new std::vector<std::string>(m_globalKeys);
        //std::vector<std::string> list = new std::vector<std::string>(m_globalKeys);
        NetRouteManager::Invoke(peer, "GlobalKeys", m_globalKeys);
    }

    // private
    void SendLocationIcons(OWNER_t peer) {
        NetPackage zpackage;
        tempIconList.clear();
        GetLocationIcons(tempIconList);
        zpackage.Write<int32_t>(tempIconList.size());
        for (auto&& keyValuePair : tempIconList) {
            zpackage.Write(keyValuePair.first);
            zpackage.Write(keyValuePair.second);
        }

        NetRouteManager::Invoke(peer, "LocationIcons", zpackage);
    }

    // private
    void OnNewPeer(OWNER_t peerID) {
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
    void PrepareSave() {
        m_worldSave.Write<int32_t>(m_generatedZones.size());
        for (auto&& vec : m_generatedZones) {
            m_worldSave.Write(vec);
        }
        m_worldSave.Write(Version::PGW);
        m_worldSave.Write(m_locationVersion);
        m_worldSave.Write<int32_t>(m_globalKeys.size());
        for (auto&& key : m_globalKeys) {
            m_worldSave.Write(key);
        }
        m_worldSave.Write(m_locationsGenerated);
        m_worldSave.Write<int32_t>(m_locationInstances.size());
        for (auto&& pair : m_locationInstances) {
            auto&& inst = pair.second;
            m_worldSave.Write(inst.m_location->m_prefabName);
            m_worldSave.Write(inst.m_position);
            m_worldSave.Write(inst.m_placed);
        }
    }

    // public
    void SaveAsync(NetPackage& writer) {
        writer.m_stream.Write(m_worldSave.m_stream.m_buf);
        m_worldSave.m_stream.Clear();
    }

    // public
    void Load(NetPackage& reader, int32_t version) {
        m_generatedZones.clear();
        int32_t num = reader.Read<int32_t>();
        for (int32_t i = 0; i < num; i++) {
            m_generatedZones.insert(reader.Read<Vector2i>());
        }
        if (version >= 13) {
            int32_t num2 = reader.Read<int32_t>();
            int32_t num3 = (version >= 21) ? reader.Read<int32_t>() : 0;
            if (num2 != Version::PGW) {
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
                if (version >= 20) {
                    m_locationsGenerated = reader.Read<bool>();
                }
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
                if (num2 != Version::PGW) {
                    m_locationInstances.clear();
                    m_locationsGenerated = false;
                }

                if (num3 != m_locationVersion) {
                    m_locationsGenerated = false;
                }
            }
        }
    }

    // private
    void Update() {
        //m_lastFixedTime = Time.fixedTime;
        //if (ZNet.GetConnectionStatus() != ZNet.ConnectionStatus.Connected) {
        //    return;
        //}

        PERIODIC_NOW(100ms, {
            UpdateTTL(.1f);

            for (auto&& znetPeer : NetManager::GetPeers()) {
                CreateGhostZones(znetPeer->m_pos);
            }
            });

        /*
        //Terminal.m_testList["Time"] = Time.fixedTime.ToString("0.00") + " / " + TimeSinceStart().ToString("0.00");
        //m_updateTimer += Time.deltaTime;
        if (m_updateTimer > 0.1f) {
            m_updateTimer = 0;
            // once again, this is not useful for the server
            // ZNet RefPos is set beyond outer space..
            //bool flag = CreateLocalZones(ZNet.instance.GetReferencePosition());
            // I feel like these ZNet refpos usages are because of the server when used as
            // a P2P server, where everyone is kinda a server? idk, but this seems like it
            UpdateTTL(0.1f);
            //if (!flag) {

                // also this is useless for server
                //CreateGhostZones(ZNet.instance.GetReferencePosition());
                for (auto&& znetPeer : NetManager::GetPeers()) {
                    CreateGhostZones(znetPeer->m_pos);
                }
            //}
        }*/
    }

    // private
    bool CreateGhostZones(const Vector3& refPoint) {
        Vector2i zone = WorldToZonePos(refPoint);
        if (!IsZoneGenerated(zone) && SpawnZone(zone)) {
            return true;
        }
        int32_t num = m_activeArea + m_activeDistantArea;
        for (int32_t i = zone.y - num; i <= zone.y + num; i++) {
            for (int32_t j = zone.x - num; j <= zone.x + num; j++) {
                Vector2i zoneID(j, i);
                GameObject gameObject2;
                if (!IsZoneGenerated(zoneID) && SpawnZone(zoneID)) {
                    return true;
                }
            }
        }
        return false;
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
    bool IsZoneLoaded(const Vector3& point) {
        Vector2i zone = WorldToZonePos(point);
        return IsZoneLoaded(zone);
    }

    // public
    bool IsZoneLoaded(const Vector2i& zoneID) {
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
    bool SpawnZone(const Vector2i& zoneID) { //GameObject& root*/) {
        Heightmap componentInChildren = m_zonePrefab.GetComponentInChildren<Heightmap>();

        // Waiting for the threadto finish
        // A thread pool might be better
        // other types arent needed
        if (!HeightmapBuilder::IsTerrainReady(zoneID)) {
            //root = null;
            return false;
        }

        Vector3 zonePos = ZoneToWorldPos(zoneID);

        GameObject root = UnityEngine.Object.Instantiate<GameObject>(m_zonePrefab, zonePos, Quaternion::IDENTITY);
        //if ((mode == SpawnMode::Ghost || mode == SpawnMode::Full) 
            //&& !IsZoneGenerated(zoneID)) {

        if (!IsZoneGenerated(zoneID)) {
            Heightmap componentInChildren2 = root.GetComponentInChildren<Heightmap>();

            //static std::vector<ClearArea> m_tempClearAreas;
            //static std::vector<GameObject> m_tempSpawnedObjects;
            //m_tempClearAreas.clear();
            //m_tempSpawnedObjects.clear();

            std::vector<ClearArea> m_tempClearAreas;
            std::vector<GameObject> m_tempSpawnedObjects;
            PlaceLocations(zoneID, zonePos, root.transform, m_tempClearAreas, m_tempSpawnedObjects);
            PlaceVegetation(zoneID, zonePos, componentInChildren2, m_tempClearAreas, m_tempSpawnedObjects);
            PlaceZoneCtrl(zoneID, zonePos, m_tempSpawnedObjects);

            //if (mode == SpawnMode::Ghost) {
            for (auto&& obj : m_tempSpawnedObjects) {
                UnityEngine.Object.Destroy(obj); // unity-specific; must modify or remove/reconsiliate...
            }
            m_tempSpawnedObjects.clear();
            UnityEngine.Object.Destroy(root);
            //root = null;
        //}
            SetZoneGenerated(zoneID);
        }
        return true;
    }

    // private
    void PlaceZoneCtrl(Vector2i zoneID, Vector3 zoneCenterPos, std::vector<GameObject>& spawnedObjects) {

        //ZNetView.StartGhostInit(); // sets a priv var, but its never actually used; redundant
        GameObject gameObject = UnityEngine.Object.Instantiate<GameObject>(m_zoneCtrlPrefab, zoneCenterPos, Quaternion::IDENTITY);
        gameObject.GetComponent<ZNetView>().GetZDO().SetPGWVersion(Version::PGW);
        spawnedObjects.push_back(gameObject); // unity only
        //ZNetView.FinishGhostInit();

    }

    // private
    Vector3 GetRandomPointInRadius(VUtils::Random::State& state, const Vector3& center, float radius) {
        //VUtils::Random::State state;
        float f = state.NextFloat() * PI * 2.f;
        float num = state.Range(0.f, radius);
        return center + Vector3(sin(f) * num, 0, cos(f) * num);
    }

    // private
    void PlaceVegetation(Vector2i zoneID, Vector3 zoneCenterPos, Heightmap hmap, std::vector<ClearArea>& clearAreas, std::vector<GameObject>& spawnedObjects) {
        //UnityEngine.Random.State state = UnityEngine.Random.state;

        int32_t seed = WorldGenerator::GetSeed();
        float num = m_zoneSize / 2.f;
        int32_t num2 = 1;
        for (auto&& zoneVegetation : m_vegetation) {
            num2++;
            if (zoneVegetation.m_enable && hmap.HaveBiome(zoneVegetation.m_biome)) {

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
                float num4 = cos(0.017453292f * zoneVegetation.m_maxTilt);
                float num5 = cos(0.017453292f * zoneVegetation.m_minTilt);
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
                            Heightmap heightmap = GetGroundData(vector2, vector3, biome, biomeArea);

                            if ((MakeBitMask(zoneVegetation.m_biome) & biome) != Heightmap::Biome::None
                                && (MakeBitMask(zoneVegetation.m_biomeArea) & biomeArea) != Heightmap::BiomeArea::None) {

                                //if ( (zoneVegetation.m_biome & biome) != Heightmap::Biome::None 
                                //    && (zoneVegetation.m_biomeArea & biomeArea) != Heightmap::BiomeArea::None) {

                                float y2;
                                Vector3 vector4;
                                if (zoneVegetation.m_snapToStaticSolid && GetStaticSolidHeight(vector2, y2, vector4)) {
                                    vector2.y = y2;
                                    vector3 = vector4;
                                }
                                float num11 = vector2.y - m_waterLevel;
                                if (num11 >= zoneVegetation.m_minAltitude && num11 <= zoneVegetation.m_maxAltitude) {
                                    if (zoneVegetation.m_minVegetation != zoneVegetation.m_maxVegetation) {
                                        float vegetationMask = heightmap.GetVegetationMask(vector2);
                                        if (vegetationMask > zoneVegetation.m_maxVegetation || vegetationMask < zoneVegetation.m_minVegetation) {
                                            continue;
                                            //goto IL_501;
                                        }
                                    }
                                    if (zoneVegetation.m_minOceanDepth != zoneVegetation.m_maxOceanDepth) {
                                        float oceanDepth = heightmap.GetOceanDepth(vector2);
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
                                            component.GetNetSync()->SetPGWVersion(Version::PGW);
                                            if (scale != gameObject.transform.localScale.x) {

                                                // this does set the Unity gameobject localscale
                                                component.SetLocalScale(Vector3(scale, scale, scale));

                                                // idk what this is doing (might be a unity gimmick)
                                                for (auto&& collider : gameObject.GetComponentsInChildren<Collider>()) {
                                                    collider.enabled = false;
                                                    collider.enabled = true;
                                                }
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
    bool InsideClearArea(const std::vector<ClearArea>& areas, const Vector3& point) {
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
    ZoneLocation* GetLocation(int32_t hash) {
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
    ZoneLocation* GetLocation(const std::string& name) {
        for (auto&& zoneLocation : m_locations) {
            if (zoneLocation->m_prefabName == name) {
                return zoneLocation.get();
            }
        }
        return nullptr;
    }

    // private
    void ClearNonPlacedLocations() {
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
    void CheckLocationDuplicates() {
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
    // seems to never be called from within logs
    /*
    void GenerateLocations() {
        if (!Application.isPlaying) {
            LOG(INFO) << "Setting up locations";
            SetupLocations();
        }
        LOG(INFO) << "Generating locations";
        DateTime now = DateTime.Now;
        m_locationsGenerated = true;
        UnityEngine.Random.State state = UnityEngine.Random.state;
        CheckLocationDuplicates();
        ClearNonPlacedLocations();
        for (auto&& zoneLocation : from a : m_locations
            orderby a.m_prioritized descending
            select a) {
            if (zoneLocation.m_enable && zoneLocation.m_quantity != 0) {
                GenerateLocations(zoneLocation);
            }
        }
        UnityEngine.Random.state = state;
        LOG(INFO) << " Done generating locations, duration:" << (DateTime.Now - now).TotalMilliseconds.ToString() << " ms";
    }*/

    // private
    int32_t CountNrOfLocation(ZoneLocation* location) {
        int32_t count = 0;
        for (auto&& loc : m_locationInstances) {
            if (loc.second.m_location.m_prefabName == location->m_prefabName) {
                count++;
            }
        }

        if (count > 0) {
            LOG(INFO) << "Old location found " << location->m_prefabName << " (" << count << ")";
        }

        return count;

        //int32_t num = 0;
        //using (robin_hood::unordered_map<Vector2i, ZoneSystem.LocationInstance>.ValueCollection.Enumerator enumerator = m_locationInstances.Values.GetEnumerator()) {
        //    while (enumerator.MoveNext()) {
        //        if (enumerator.Current.m_location.m_prefabName == location.m_prefabName) {
        //            num++;
        //        }
        //    }
        //}
        //if (num > 0) {
        //    LOG(INFO) << "Old location found " << location.m_prefabName << " x " << num.ToString();
        //}
        //return num;
    }

    // private
    /*
    void GenerateLocations(ZoneLocation location) {
        DateTime now = DateTime.Now;
        UnityEngine.Random.InitState(WorldGenerator::GetSeed() + location.m_prefabName.GetStableHashCode());
        int32_t num = 0;
        int32_t num2 = 0;
        int32_t num3 = 0;
        int32_t num4 = 0;
        int32_t num5 = 0;
        int32_t num6 = 0;
        int32_t num7 = 0;
        int32_t num8 = 0;
        float locationRadius = Mathf.Max(location.m_exteriorRadius, location.m_interiorRadius);
        int32_t num9 = location.m_prioritized ? 200000 : 100000;
        int32_t num10 = 0;
        int32_t num11 = CountNrOfLocation(location);
        float num12 = 10000;
        if (location.m_centerFirst) {
            num12 = location.m_minDistance;
        }
        if (location.m_unique && num11 > 0) {
            return;
        }
        int32_t num13 = 0;
        while (num13 < num9 && num11 < location.m_quantity) {
            Vector2i randomZone = GetRandomZone(num12);
            if (location.m_centerFirst) {
                num12 += 1;
            }
            if (m_locationInstances.ContainsKey(randomZone)) {
                num++;
            }
            else if (!IsZoneGenerated(randomZone)) {
                Vector3 zonePos = GetZonePos(randomZone);
                BiomeArea biomeArea = WorldGenerator.instance.GetBiomeArea(zonePos);
                if ((location.m_biomeArea & biomeArea) == (Heightmap.BiomeArea)0) {
                    num4++;
                }
                else {
                    for (int32_t i = 0; i < 20; i++) {
                        num10++;
                        Vector3 randomPointInZone = GetRandomPointInZone(randomZone, locationRadius);
                        float magnitude = randomPointInZone.magnitude;
                        if (location.m_minDistance != 0f && magnitude < location.m_minDistance) {
                            num2++;
                        }
                        else if (location.m_maxDistance != 0f && magnitude > location.m_maxDistance) {
                            num2++;
                        }
                        else {
                            Heightmap.Biome biome = WorldGenerator.instance.GetBiome(randomPointInZone);
                            if ((location.m_biome & biome) == Heightmap.Biome.None) {
                                num3++;
                            }
                            else {
                                randomPointInZone.y = WorldGenerator.instance.GetHeight(randomPointInZone.x, randomPointInZone.z);
                                float num14 = randomPointInZone.y - m_waterLevel;
                                if (num14 < location.m_minAltitude || num14 > location.m_maxAltitude) {
                                    num5++;
                                }
                                else {
                                    if (location.m_inForest) {
                                        float forestFactor = WorldGenerator.GetForestFactor(randomPointInZone);
                                        if (forestFactor < location.m_forestTresholdMin || forestFactor > location.m_forestTresholdMax) {
                                            num6++;
                                            goto IL_27C;
                                        }
                                    }
                                    float num15;
                                    Vector3 vector;
                                    WorldGenerator.instance.GetTerrainDelta(randomPointInZone, location.m_exteriorRadius, num15, vector);
                                    if (num15 > location.m_maxTerrainDelta || num15 < location.m_minTerrainDelta) {
                                        num8++;
                                    }
                                    else {
                                        if (location.m_minDistanceFromSimilar <= 0f || !HaveLocationInRange(location.m_prefabName, location.m_group, randomPointInZone, location.m_minDistanceFromSimilar)) {
                                            RegisterLocation(location, randomPointInZone, false);
                                            num11++;
                                            break;
                                        }
                                        num7++;
                                    }
                                }
                            }
                        }
                    IL_27C:;
                    }
                }
            }
            num13++;
        }
        if (num11 < location.m_quantity) {
            ZLog.LogWarning(std::string.Concat(new std::string[]{
                "Failed to place all ",
                location.m_prefabName,
                ", placed ",
                num11.ToString(),
                " of ",
                location.m_quantity.ToString()
                }));
            ZLog.DevLog("errorLocationInZone " + num.ToString());
            ZLog.DevLog("errorCenterDistance " + num2.ToString());
            ZLog.DevLog("errorBiome " + num3.ToString());
            ZLog.DevLog("errorBiomeArea " + num4.ToString());
            ZLog.DevLog("errorAlt " + num5.ToString());
            ZLog.DevLog("errorForest " + num6.ToString());
            ZLog.DevLog("errorSimilar " + num7.ToString());
            ZLog.DevLog("errorTerrainDelta " + num8.ToString());
        }
        DateTime.Now - now;
    }*/

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
    void PlaceLocations(Vector2i zoneID,
        Vector3 zoneCenterPos,
        Transform parent,
        std::vector<ClearArea>& clearAreas,
        //SpawnMode mode, 
        std::vector<GameObject>& spawnedObjects)
    {

        //GenerateLocationsIfNeeded();
        //DateTime now = DateTime.Now;
        auto now(steady_clock::now());
        //LocationInstance locationInstance;
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
            Heightmap heightmap;
            GetGroundData(position, vector, biome, biomeArea, heightmap);

            if (locationInstance.m_location->m_snapToWater) {
                position.y = m_waterLevel;
            }
            if (locationInstance.m_location->m_location.m_clearArea) {
                ClearArea item(position, locationInstance.m_location->m_exteriorRadius);
                clearAreas.push_back(item);
            }
            Quaternion rot(Quaternion::IDENTITY);
            if (locationInstance.m_location->m_slopeRotation) {
                float num;
                Vector3 vector2;
                GetTerrainDelta(position, locationInstance.m_location->m_exteriorRadius, num, vector2);
                Vector3 forward(vector2.x, 0, vector2.z);
                forward.Normalize();
                rot = Quaternion::LookRotation(forward);
                Vector3 eulerAngles = rot.eulerAngles;
                eulerAngles.y = round(eulerAngles.y / 22.5f) * 22.5f;
                rot.eulerAngles = eulerAngles;
            }
            else if (locationInstance.m_location.m_randomRotation) {
                //rot = Quaternion::Euler(0, (float)UnityEngine.Random.Range(0, 16) * 22.5f, 0);
                rot = Quaternion::Euler(0, (float)VUtils::Random::State().Range(0, 16) * 22.5f, 0);
            }

            int32_t seed = WorldGenerator::GetSeed() + zoneID.x * 4271 + zoneID.y * 9187;
            SpawnLocation(locationInstance.m_location, seed, position, rot, mode, spawnedObjects);
            locationInstance.m_placed = true;
            m_locationInstances[zoneID] = locationInstance;

            //TimeSpan timeSpan = DateTime.Now - now;

            LOG(INFO) << "Placed locations in zone "
                << zoneID << " duration "
                << duration_cast<milliseconds>(steady_clock::now() - now).count();

            //std::string[] array = new std::string[5];
            //array[0] = "Placed locations in zone ";
            //array[1] = zoneID.ToString();
            //array[2] = "  duration ";
            //array[3] = timeSpan.TotalMilliseconds.ToString();
            //array[4] = " ms";
            //LOG(INFO) << std::string.Concat(array);

            if (locationInstance.m_location->m_unique) {
                RemoveUnplacedLocations(locationInstance.m_location);
            }
            if (locationInstance.m_location->m_iconPlaced) {
                SendLocationIcons(NetRouteManager::EVERYBODY);
            }
        }
    }

    // private
    void RemoveUnplacedLocations(ZoneLocation* location) {
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
    GameObject SpawnProxyLocation(int32_t hash, int32_t seed, const Vector3& pos, const Quaternion& rot) {
        auto&& location = GetLocation(hash);
        if (!location) {
            LOG(ERROR) << "Missing location:" + hash;
            return null;
        }
        std::vector<GameObject> spawnedGhostObjects;
        return SpawnLocation(location, seed, pos, rot, SpawnMode::Client, spawnedGhostObjects);
    }

    // private
    GameObject SpawnLocation(ZoneLocation* location, int32_t seed, Vector3 pos, Quaternion rot, SpawnMode mode, std::vector<GameObject>& spawnedGhostObjects) {
        location.m_prefab.transform.position = Vector3::ZERO;
        location.m_prefab.transform.rotation = Quaternion::IDENTITY;
        //UnityEngine.Random.InitState(seed);

        VUtils::Random::State state(seed);

        Location component = location.m_prefab.GetComponent<Location>();
        bool flag = component && component.m_useCustomInteriorTransform && component.m_interiorTransform && component.m_generator;
        if (flag) {
            Vector2i zone = GetZone(pos);
            Vector3 zonePos = GetZonePos(zone);
            component.m_generator.transform.localPosition = Vector3.zero;
            Vector3 vector = zonePos + location.m_interiorPosition + location.m_generatorPosition - pos;
            Vector3 localPosition = (Matrix4x4.Rotate(Quaternion.Inverse(rot)) * Matrix4x4.Translate(vector)).GetColumn(3);
            localPosition.y = component.m_interiorTransform.localPosition.y;
            component.m_interiorTransform.localPosition = localPosition;
            component.m_interiorTransform.localRotation = Quaternion.Inverse(rot);
        }
        if (component
            && component.m_generator
            && component.m_useCustomInteriorTransform != component.m_generator.m_useCustomInteriorTransform) {
            //LOG(ERROR) << ""
            ZLog.LogWarning(component.name + " & " + component.m_generator.name + " don't have matching m_useCustomInteriorTransform()! If one has it the other should as well!");
        }
        if (mode == SpawnMode::Full || mode == SpawnMode::Ghost) {
            for (auto&& znetView : location.m_netViews) {
                znetView.gameObject.SetActive(true);
            }
            //UnityEngine.Random.InitState(seed);

            state = VUtils::Random::State(seed);

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

                    if (mode == SpawnMode::Ghost) {
                        spawnedGhostObjects.Add(gameObject);
                        //ZNetView.FinishGhostInit();
                    }
                }
            }
            WearNTear.m_randomInitialDamage = false;
            CreateLocationProxy(location, seed, pos, rot, mode, spawnedGhostObjects);
            SnapToGround.SnappAll();
            return null;
        }
        UnityEngine.Random.InitState(seed);
        for (auto&& randomSpawn2 : location.m_randomSpawns) {
            randomSpawn2.Randomize();
        }
        for (auto&& znetView3 : location.m_netViews) {
            znetView3.gameObject.SetActive(false);
        }
        GameObject gameObject2 = UnityEngine.Object.Instantiate<GameObject>(location.m_prefab, pos, rot);
        gameObject2.SetActive(true);
        SnapToGround.SnappAll();
        return gameObject2;
    }

    // private
    void CreateLocationProxy(ZoneLocation* location, int32_t seed, Vector3 pos, Quaternion rotation, SpawnMode mode, std::vector<GameObject>& spawnedGhostObjects) {
        //if (mode == SpawnMode::Ghost) {
        //    ZNetView.StartGhostInit();
        //}

        GameObject gameObject = UnityEngine.Object.Instantiate<GameObject>(m_locationProxyPrefab, pos, rotation);
        LocationProxy component = gameObject.GetComponent<LocationProxy>();
        bool spawnNow = mode == ZoneSystem.SpawnMode.Full;
        component.SetLocation(location.m_prefab.name, seed, spawnNow, Version::PGW);
        if (mode == ZoneSystem.SpawnMode.Ghost) {
            spawnedGhostObjects.push_back(gameObject);
            //ZNetView.FinishGhostInit();
        }
    }

    // private
    void RegisterLocation(ZoneLocation* location, const Vector3& pos, bool generated) {
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
    bool HaveLocationInRange(const std::string& prefabName, const std::string& group, const Vector3& p, float radius) {

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
    bool GetLocationIcon(const std::string& name, Vector3& pos) {
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
    void GetLocationIcons(robin_hood::unordered_map<Vector3, std::string> icons) {
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
    void GetTerrainDelta(const Vector3& center, float& radius, float& delta, Vector3& slopeDirection) {
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
    bool IsBlocked(const Vector3& p) {
        p.y += 2000;
        return Physics.Raycast(p, Vector3::DOWN, 10000, m_blockRayMask);
    }

    // public
    float GetAverageGroundHeight(const Vector3& p, float radius) {
        Vector3 origin = p;
        origin.y = 6000;
        RaycastHit raycastHit;
        if (Physics.Raycast(origin, Vector3::DOWN, raycastHit, 10000, m_terrainRayMask)) {
            return raycastHit.point.y;
        }
        return p.y;
    }

    // public
    float GetGroundHeight(const Vector3& p) {
        Vector3 origin = p;
        origin.y = 6000;
        RaycastHit raycastHit;
        if (Physics.Raycast(origin, Vector3::DOWN, raycastHit, 10000, m_terrainRayMask)) {
            return raycastHit.point.y;
        }
        return p.y;
    }

    // public
    bool GetGroundHeight(const Vector3& p, float& height) {
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
    float GetSolidHeight(const Vector3& p) {
        Vector3 origin = p;
        origin.y += 1000;
        RaycastHit raycastHit;
        if (Physics.Raycast(origin, Vector3::DOWN, raycastHit, 2000, m_solidRayMask)) {
            return raycastHit.point.y;
        }
        return p.y;
    }

    // public
    bool GetSolidHeight(const Vector3& p, float& height, int32_t heightMargin = 1000) {
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
    bool GetSolidHeight(const Vector3& p, float& radius, float height, Transform ignore) {
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
    bool GetSolidHeight(const Vector3& p, float& height, const Vector3& normal) {
        GameObject gameObject;
        return GetSolidHeight(p, height, normal, gameObject);
    }

    // public
    bool GetSolidHeight(const Vector3& p, float& height, const Vector3& normal, GameObject go) {
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
    bool GetStaticSolidHeight(const Vector3& p, float& height, const Vector3& normal) {
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
    bool FindFloor(const Vector3& p, float& height) {
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
    Heightmap* GetGroundData(Vector3& p, Vector3& normal, Heightmap::Biome& biome, Heightmap::BiomeArea& biomeArea) {
        biome = Heightmap::Biome::None;
        biomeArea = Heightmap::BiomeArea::Everything;
        //hmap = null;

        // test collision from point, casting downwards through terrain

        // If final result is completely linked to Heightmap, could just use heightmap
        // should be simple enough,

        // global heightmaps can be queried at the world--->zone then 
        // get the relative point inside zone

        Heightmap::GetAllHeightmaps();

        // 'Terrain' gameobject is always hit 
        //  Terrain is component though
        // Terrain also exists as a Unity prefab builtin



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
    void UpdateTTL(float dt) {
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
    bool FindClosestLocation(const std::string& name, const Vector3& point, LocationInstance& closest) {
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
    Vector2i WorldToZonePos(const Vector3& point) {
        int32_t x = floor((point.x + m_zoneSize / 2) / m_zoneSize);
        int32_t y = floor((point.z + m_zoneSize / 2) / m_zoneSize);
        return Vector2i(x, y);
    }

    // public
    // zone position to ~world position
    // GetZonePos
    Vector3 ZoneToWorldPos(const Vector2i& id) {
        return Vector3(id.x * m_zoneSize, 0, id.y * m_zoneSize);
    }

    // private
    void SetZoneGenerated(const Vector2i& zoneID) {
        m_generatedZones.insert(zoneID);
    }

    // private
    bool IsZoneGenerated(const Vector2i& zoneID) {
        return m_generatedZones.contains(zoneID);
    }

    // public
    bool SkipSaving() {
        return m_error || m_didZoneTest;
    }

    // public
    //float TimeSinceStart() {
    //    return m_lastFixedTime - m_startTime;
    //}

    // public
    void ResetGlobalKeys() {
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

    // private
    void RPC_SetGlobalKey(OWNER_t sender, std::string name) {
        if (m_globalKeys.contains(name)) {
            return;
        }
        m_globalKeys.insert(name);
        SendGlobalKeys(NetRouteManager::EVERYBODY);
    }

    // public
    // client terminal only
    /*
    void RemoveGlobalKey(const std::string& name) {
        NetRouteManager::Invoke("RemoveGlobalKey", name);
    }*/

    // private
    void RPC_RemoveGlobalKey(OWNER_t sender, std::string name) {
        if (!m_globalKeys.contains(name)) {
            return;
        }
        m_globalKeys.erase(name);
        SendGlobalKeys(NetRouteManager::EVERYBODY);
    }

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


}