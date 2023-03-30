#include "DungeonManager.h"
#include "VUtilsResource.h"
#include "DataReader.h"
#include "PrefabManager.h"
#include "DungeonGenerator.h"
#include "ZDOManager.h"
#include "NetManager.h"
#include "Hashes.h"

auto DUNGEON_MANAGER(std::make_unique<IDungeonManager>()); // TODO stop constructing in global
IDungeonManager* DungeonManager() {
    return DUNGEON_MANAGER.get();
}



void IDungeonManager::Init() {
    // load dungeons:
    auto opt = VUtils::Resource::ReadFile<BYTES_t>("dungeons.pkg");
    if (!opt)
        throw std::runtime_error("dungeons.pkg missing");

    DataReader pkg(opt.value());

    pkg.Read<std::string>(); // date/comment
    std::string ver = pkg.Read<std::string>();
    LOG(INFO) << "dungeons.pkg has game version " << ver;
    if (ver != VConstants::GAME)
        LOG(WARNING) << "dungeons.pkg uses different game version than server";

    int32_t count = pkg.Read<int32_t>();
    LOG(INFO) << "Loading " << count << " dungeons";
    for (int i = 0; i < count; i++) {
        auto dungeon = std::make_unique<Dungeon>();

        //HASH_t hash = pkg.Read<HASH_t>();

        auto name = pkg.Read<std::string>();

        dungeon->m_prefab = &PrefabManager()->RequirePrefab(name);

        VLOG(2) << "Loading dungeon " << name;

        dungeon->m_interiorPosition = pkg.Read<Vector3f>();
        dungeon->m_originalPosition = pkg.Read<Vector3f>();

        dungeon->m_algorithm = (Dungeon::Algorithm) pkg.Read<int32_t>();
        dungeon->m_alternativeFunctionality = pkg.Read<bool>();
        dungeon->m_campRadiusMax = pkg.Read<float>();
        dungeon->m_campRadiusMin = pkg.Read<float>();
        dungeon->m_doorChance = pkg.Read<float>();
        
        auto doorCount = pkg.Read<int32_t>();
        for (int i2 = 0; i2 < doorCount; i2++) {
            Dungeon::DoorDef door;
            door.m_prefab = PrefabManager()->GetPrefab(pkg.Read<HASH_t>());
            if (!door.m_prefab) {
                throw std::runtime_error("dungeon door missing prefab");
            }

            door.m_connectionType = pkg.Read<std::string>();
            door.m_chance = pkg.Read<float>();

            dungeon->m_doorTypes.push_back(door);
        }

        dungeon->m_gridSize = pkg.Read<int32_t>();
        dungeon->m_maxRooms = pkg.Read<int32_t>();
        dungeon->m_maxTilt = pkg.Read<float>();
        dungeon->m_minAltitude = pkg.Read<float>();
        dungeon->m_minRequiredRooms = pkg.Read<int32_t>();
        dungeon->m_minRooms = pkg.Read<int32_t>();
        dungeon->m_perimeterBuffer = pkg.Read<float>();
        dungeon->m_perimeterSections = pkg.Read<int32_t>();
        
        dungeon->m_requiredRooms = pkg.Read<robin_hood::unordered_set<std::string>>();

        dungeon->m_spawnChance = pkg.Read<float>();
        dungeon->m_themes = (Room::Theme) pkg.Read<int32_t>();
        dungeon->m_tileWidth = pkg.Read<float>();
        
        auto roomCount = pkg.Read<int32_t>();
        for (int i2 = 0; i2 < roomCount; i2++) {
            auto room(std::make_unique<Room>());

            room->m_name = pkg.Read<std::string>();
            room->m_divider = pkg.Read<bool>();
            room->m_endCap = pkg.Read<bool>();
            room->m_endCapPrio = pkg.Read<int32_t>();
            room->m_entrance = pkg.Read<bool>();
            room->m_faceCenter = pkg.Read<bool>();
            room->m_minPlaceOrder = pkg.Read<int32_t>();
            room->m_perimeter = pkg.Read<bool>();

            auto connCount = pkg.Read<int32_t>();
            for (int i3 = 0; i3 < connCount; i3++) {
                auto conn(std::make_unique<RoomConnection>());

                conn->m_type = pkg.Read<std::string>();
                conn->m_entrance = pkg.Read<bool>();
                conn->m_allowDoor = pkg.Read<bool>();
                conn->m_doorOnlyIfOtherAlsoAllowsDoor = pkg.Read<bool>();
                conn->m_localPos = pkg.Read<Vector3f>();
                conn->m_localRot = pkg.Read<Quaternion>();

                room->m_roomConnections.push_back(std::move(conn));
            }

            auto viewCount = pkg.Read<int32_t>();
            for (int i3 = 0; i3 < viewCount; i3++) {
                Prefab::Instance instance;
                
                instance.m_prefab = &PrefabManager()->RequirePrefab(pkg.Read<HASH_t>());
                instance.m_pos = pkg.Read<Vector3f>();
                instance.m_rot = pkg.Read<Quaternion>();

                room->m_netViews.push_back(instance);
            }

            room->m_size = pkg.Read<Vector3f>();
            room->m_theme = (Room::Theme) pkg.Read<int32_t>();
            room->m_weight = pkg.Read<float>();
            room->m_pos = pkg.Read<Vector3f>();
            room->m_rot = pkg.Read<Quaternion>();

            dungeon->m_availableRooms.push_back(std::move(room));
        }

        m_dungeons.insert({ dungeon->m_prefab->m_hash, std::move(dungeon)});
    }
}

ZDO* IDungeonManager::TryRegenerateDungeon(ZDO& dungeonZdo) {
    auto&& netTicksNow = Valhalla()->GetWorldTicks();

    auto&& ticksDungeon = dungeonZdo.m_rev.m_ticksCreated;

    // Reset dungeons after a time
    if (ticksDungeon + SERVER_SETTINGS.dungeonResetTime < netTicksNow) {
        bool playerNear = false;

        // if a player is inside, do not reset
        for (auto&& peer : NetManager()->GetPeers()) {
            // if peer in dungeon sector, and they are high up (presumably inside the dungeon)
            if (dungeonZdo.GetZone() == ZoneManager()->WorldToZonePos(peer->m_pos)
                && peer->m_pos.y > 4000) {
                playerNear = true;
                break;
            }
        }

        auto&& dungeon = RequireDungeon(dungeonZdo.GetPrefab().m_hash);

        // Destroy all zdos high in the sky near dungeon IN ZONE
        auto pos = dungeonZdo.Position();
        auto rot = dungeonZdo.Rotation();

        if (!playerNear) {
            auto zdos = ZDOManager()->GetZDOs(dungeonZdo.GetZone(), [](const ZDO& zdo) {
                return zdo.Position().y > 4000 && zdo.GetPrefab().FlagsAbsent(Prefab::Flag::PLAYER | Prefab::Flag::TOMBSTONE);
            });

            for (auto&& ref : zdos) {
                auto&& zdo = ref.get();
                auto&& prefab = zdo.GetPrefab();

                assert(!(prefab.m_hash == Hashes::Object::Player || prefab.m_hash == Hashes::Object::Player_tombstone));

                ZDOManager()->DestroyZDO(zdo);
            }

            LOG(INFO) << "Regenerated " << dungeon.m_prefab->m_name << " at " << pos;

            return &Generate(dungeon, pos, rot);
        }
        else {
            LOG(INFO) << "Unable to regenerate " << dungeon.m_prefab->m_name << " at " << pos << " (peer is inside)";
        }
    }

    return nullptr;
}

void IDungeonManager::TryRegenerateDungeons() {
    //OPTICK_EVENT();

    //for (auto&& itr = m_dungeonInstances.begin() + m_nextIndex; 
    //    itr != m_dungeonInstances.begin() + m_nextIndex
    //    )

    size_t idx = m_nextIndex;
    while (idx < std::min(m_dungeonInstances.size(), m_nextIndex + SERVER_SETTINGS.dungeonIncrementalResetCount)) 
    {
        auto&& itr = m_dungeonInstances.begin() + idx;

        ZDO* dungeonZdo = ZDOManager()->GetZDO(*itr);
        if (!dungeonZdo) {
            m_dungeonInstances.erase(itr);
            LOG(WARNING) << "Dungeon ZDO no longer exists";
            break;
        }
        else {
            if (ZDO* newDungeon = TryRegenerateDungeon(*dungeonZdo)) {
                *itr = newDungeon->ID();
            }
            ++idx;
        }
    }

    m_nextIndex = idx;

    if (m_nextIndex >= m_dungeonInstances.size()) {
        m_nextIndex = 0;
    }

    /*
    if (m_nextIndex >= m_dungeonInstances.size()) {
        m_nextIndex = 0;
    }
    else {
        auto&& itr = m_dungeonInstances.begin() + m_nextIndex;

        ZDO* dungeonZdo = ZDOManager()->GetZDO(*itr);
        if (!dungeonZdo) {
            m_dungeonInstances.erase(itr);
            LOG(WARNING) << "Dungeon ZDO no longer exists";
        }
        else {
            if (ZDO* newDungeon = TryRegenerateDungeon(*dungeonZdo)) {
                *itr = newDungeon->ID();
            }
            m_nextIndex++;
        }
    }*/
}



ZDO& IDungeonManager::Generate(const Dungeon& dungeon, const Vector3f& pos, const Quaternion& rot) {
    auto&& zdo = ZDOManager()->Instantiate(*dungeon.m_prefab, pos, rot);
    
    DungeonGenerator(dungeon, zdo).Generate();

    return zdo;
}

ZDO& IDungeonManager::Generate(const Dungeon& dungeon, const Vector3f& pos, const Quaternion& rot, HASH_t seed) {
    auto&& zdo = ZDOManager()->Instantiate(*dungeon.m_prefab, pos, rot);

    DungeonGenerator(dungeon, zdo).Generate(seed);

    return zdo;
}

void IDungeonManager::Generate(const Dungeon& dungeon, ZDO& zdo) {
    DungeonGenerator(dungeon, zdo).Generate();
}