#include "DungeonManager.h"

#if VH_IS_ON(VH_DUNGEON_GENERATION)
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



void IDungeonManager::PostPrefabInit() {
    // load dungeons:
    auto opt = VUtils::Resource::ReadFile<BYTES_t>("dungeons.pkg");
    if (!opt)
        throw std::runtime_error("dungeons.pkg missing");

    DataReader pkg(opt.value());

    pkg.Read<std::string_view>(); // date/comment
    auto ver = pkg.Read<std::string_view>();
    LOG_INFO(LOGGER, "dungeons.pkg has game version {}", ver);
    if (ver != VConstants::GAME)
        LOG_WARNING(LOGGER, "dungeons.pkg uses different game version than server");

    int32_t count = pkg.Read<int32_t>();
    LOG_INFO(LOGGER, "Loading {} dungeons", count);
    for (int i = 0; i < count; i++) {
        auto dungeon = std::make_unique<Dungeon>();

        //HASH_t hash = pkg.Read<HASH_t>();

        auto name = pkg.Read<std::string_view>();

        dungeon->m_prefab = &PrefabManager()->RequirePrefabByName(name);

        //VLOG(2) << "Loading dungeon " << name;

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
        //decltype(Dungeon::m_requiredRooms)::be
        dungeon->m_requiredRooms = pkg.Read<decltype(Dungeon::m_requiredRooms)>();

        dungeon->m_spawnChance = pkg.Read<float>();
        dungeon->m_themes = (Room::Theme) pkg.Read<int32_t>();
        dungeon->m_tileWidth = pkg.Read<float>();
        
        auto roomCount = pkg.Read<int32_t>();
        for (int i2 = 0; i2 < roomCount; i2++) {
            auto room(std::make_unique<Room>());

            room->m_name = pkg.Read<std::string>();
            room->m_hash = VUtils::String::GetStableHashCode(room->m_name);
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
                
                instance.m_prefabHash = pkg.Read<HASH_t>();
                instance.m_pos = pkg.Read<Vector3f>();
                instance.m_rot = pkg.Read<Quaternion>();
                
                // ensure prefab existence
                instance.GetPrefab();

                room->m_netViews.push_back(instance);
            }

            room->m_size = pkg.Read<Vector3f>();
            room->m_theme = (Room::Theme) pkg.Read<int32_t>();
            room->m_weight = pkg.Read<float>();
            room->m_pos = pkg.Read<Vector3f>();
            room->m_rot = pkg.Read<Quaternion>();

            dungeon->m_availableRooms.push_back(std::move(room));
        }

        HASH_t hash = dungeon->m_prefab->m_hash;
        m_dungeons.insert({ hash, std::move(dungeon)});
    }
}

#if VH_IS_ON(VH_DUNGEON_REGENERATION)
ZDO* IDungeonManager::TryRegenerateDungeon(ZDO dungeonZdo) {
    static constexpr HASH_t LAST_RESET_HASH = VUtils::String::GetStableHashCodeCT("Areas LastReset");

    // https://github.com/T3kla/ValMods/blob/52da19785190c2d9b6de93d09195d942e4da8686/~DungeonReset/Scripts/Extensions.cs#LL12C86-L12C86
    auto&& lastReset = seconds(dungeonZdo.GetLong(LAST_RESET_HASH));
    auto&& unixTime = steady_clock::now().time_since_epoch();

    auto since = duration_cast<seconds>(unixTime) - lastReset;

    if (since > VH_SETTINGS.dungeonsRegenerationInterval) {
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
        auto pos = dungeonZdo.GetPosition();
        auto rot = dungeonZdo.GetRotation();

        if (!playerNear) {
            auto zdos = ZDOManager()->GetZDOs(dungeonZdo.GetZone(), [](const ZDO zdo) {
                return zdo.GetPosition().y > 4000 && zdo.GetPrefab().AllFlagsAbsent(Prefab::Flag::PLAYER | Prefab::Flag::TOMBSTONE);
            });

            for (auto&& ref : zdos) {
                auto&& zdo = ref.get();
                auto&& prefab = zdo.GetPrefab();

                assert(!(prefab.m_hash == Hashes::Object::Player || prefab.m_hash == Hashes::Object::Player_tombstone));

                ZDOManager()->DestroyZDO(zdo);
            }

            LOG_INFO(LOGGER, "Regenerated {} at {}", dungeon.m_prefab->m_name, pos);

            auto&& zdo = Generate(dungeon, pos, rot).get();
            zdo.Set(LAST_RESET_HASH, unixTime.count());
            
            return &zdo;
        }
        else {
            LOG_INFO(LOGGER, "Unable to regenerate {} at {} (peer is inside)", dungeon.m_prefab->m_name, pos);
        }
    }

    return nullptr;
}

void IDungeonManager::TryRegenerateDungeons() {
    size_t idx = m_nextIndex;
    while (idx < std::min(m_dungeonInstances.size(), m_nextIndex + VH_SETTINGS.dungeonsRegenerationMaxSteps)) 
    {
        auto&& itr = m_dungeonInstances.begin() + idx;

        ZDO* dungeonZdo = ZDOManager()->GetZDO(*itr);
        if (!dungeonZdo) {
            m_dungeonInstances.erase(itr);
            //LOG(WARNING) << "Dungeon ZDO no longer exists";
            break;
        }
        else {
            if (auto&& newDungeon = TryRegenerateDungeon(*dungeonZdo)) {
                *itr = newDungeon->GetID();
            }
            ++idx;
        }
    }

    m_nextIndex = idx;

    if (m_nextIndex >= m_dungeonInstances.size()) {
        m_nextIndex = 0;
    }
}
#endif


std::reference_wrapper<ZDO> IDungeonManager::Generate(const Dungeon& dungeon, Vector3f pos, Quaternion rot) {
    auto&& zdo = ZDOManager()->Instantiate(*dungeon.m_prefab, pos);
    zdo.SetRotation(rot);
    
    DungeonGenerator(dungeon, zdo).Generate();

    return zdo;
}

std::reference_wrapper<ZDO> IDungeonManager::Generate(const Dungeon& dungeon, Vector3f pos, Quaternion rot, HASH_t seed) {
    auto&& zdo = ZDOManager()->Instantiate(*dungeon.m_prefab, pos);
    zdo.SetRotation(rot);

    DungeonGenerator(dungeon, zdo).Generate(seed);

    return zdo;
}

void IDungeonManager::Generate(const Dungeon& dungeon, ZDO zdo) {
    DungeonGenerator(dungeon, zdo).Generate();
}
#endif