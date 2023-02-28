#include "DungeonManager.h"
#include "VUtilsResource.h"
#include "DataReader.h"
#include "PrefabManager.h"

auto DUNGEON_MANAGER(std::make_unique<IDungeonManager>()); // TODO stop constructing in global
IDungeonManager* DungeonManager() {
    return DUNGEON_MANAGER.get();
}

void IDungeonManager::Init() {
    // load dungeons:
    auto opt = VUtils::Resource::ReadFileBytes("dungeons.pkg");
    if (!opt)
        throw std::runtime_error("dungeons.pkg missing");

    DataReader pkg(opt.value());

    pkg.Read<std::string>(); // date
    std::string ver = pkg.Read<std::string>();
    LOG(INFO) << "dungeons.pkg has game version " << ver;
    if (ver != VConstants::GAME)
        LOG(WARNING) << "dungeons.pkg uses different game version than server";

    int32_t count = pkg.Read<int32_t>();
    LOG(INFO) << "Loading " << count << " dungeons";
    for (int i = 0; i < count; i++) {
        auto dungeon = std::make_unique<Dungeon>();

        HASH_t hash = pkg.Read<HASH_t>();

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
        
        auto roomCount = pkg.Read<int32_t>();
        for (int i2 = 0; i2 < roomCount; i2++) {
            auto room(std::make_unique<Room>());

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

                room->m_roomConnections.push_back(std::move(conn));
            }

            room->m_size = pkg.Read<Vector3>();
            room->m_theme = (Room::Theme) pkg.Read<int32_t>();
            room->m_weight = pkg.Read<float>();

            dungeon->m_availableRooms.push_back(std::move(room));
        }

        m_dungeons.insert({ hash, std::move(dungeon) });
    }
}
