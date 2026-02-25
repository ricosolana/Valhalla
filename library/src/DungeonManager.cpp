#include "DungeonManager.h"
#include "Dungeon.h"
#include "Quaternion.h"
#include "RandomSpawn.h"
#include "Types.h"
#include "Vector.h"
#include <range/v3/view/map.hpp>
#include <stdexcept>
#include <type_traits>
#include <vector>

#if AVL_IS_ON(AVL_DUNGEON_GENERATION)
    #include "DataStream.h"
    #include "DungeonGenerator.h"
    #include "Hashes.h"
    #include "NetManager.h"
    #include "PrefabManager.h"
    #include "VUtilsResource.h"
    #include "ZDOManager.h"

auto DUNGEON_MANAGER = std::make_unique<IDungeonManager>();// TODO stop constructing in global

IDungeonManager *DungeonManager()
{
    return DUNGEON_MANAGER.get();
}

void IDungeonManager::post_prefab_init()
{
    // load dungeons:
    auto opt = VUtils::Resource::ReadFile<avledet::util::Bytes>("dungeons.pkg");
    if (!opt)
        throw std::runtime_error("dungeons.pkg missing");

    DataReader pkg(opt.value());

    auto comment = pkg.read<std::string_view>();// comment
    LOG_DEBUG(AVL_LOGGER, "pkg comment: {}", comment);

    auto ver = pkg.read<std::string_view>();
    LOG_NOTICE(AVL_LOGGER, "dungeons.pkg has game version {}", ver);
    if (ver != VConstants::GAME) {
        LOG_WARNING(AVL_LOGGER, "dungeons.pkg uses different game version than server ({})", ver);
    }

    std::int32_t count = pkg.read<std::int32_t>();
    for (int i = 0; i < count; i++) {
        

        //avledet::util::Hash hash = pkg.read<avledet::util::Hash>();

        // Debug anchor
        if (pkg.read<avledet::util::Hash>() != VUtils::get_stable_hash("dungeon")) {
            throw std::runtime_error("bad read anchor");
        }

        auto name = pkg.read<std::string_view>();

        auto dungeon = std::make_unique<Dungeon>(PrefabManager()->get_prefab(name));

        // TODO dungeon prefab is required (make a ref)
        //dungeon->m_prefab = PrefabManager()->get_prefab(name);

        //VLOG(2) << "Loading dungeon " << name;

        // Dungeon custom position
        {
            bool useTransform = pkg.read<bool>();
            if (useTransform) {
                dungeon->m_interior_position = pkg.read<Vector3f>();
                pkg.read<Quaternion>();// TODO
                dungeon->m_original_position = pkg.read<Vector3f>();
            }
        }

        static_assert(std::is_same_v<std::underlying_type_t<Dungeon::Algorithm>, std::int32_t>);
        dungeon->m_algorithm                 = (Dungeon::Algorithm) pkg.read<std::int32_t>();
        dungeon->m_alternative_functionality = pkg.read<bool>();
        dungeon->m_camp_radius_max           = pkg.read<float>();
        dungeon->m_camp_radius_min           = pkg.read<float>();
        dungeon->m_door_chance               = pkg.read<float>();

        auto doorCount = pkg.read<std::int32_t>();
        for (int i2 = 0; i2 < doorCount; i2++) {
            // Debug anchor
            if (pkg.read<avledet::util::Hash>() != VUtils::get_stable_hash("dungeonDoor")) {
                throw std::runtime_error("bad read anchor");
            }

            //Dungeon::DoorDef door;
            auto doorName = pkg.read<std::string_view>();//Debug
            (void) doorName;
            auto doorHash = pkg.read<avledet::util::Hash>();

            //auto compHash = VUtils::get_stable_hash(doorName);
            //if (doorHash != compHash) {
            //    throw std::runtime_error("dg door hash unequal to computed");
            //}

            auto door_prefab = PrefabManager()->find_prefab(doorHash);
            if (!door_prefab) {
                throw std::runtime_error("dungeon door missing prefab");
            }

            auto connection_type = pkg.read<std::string>();
            auto chance          = pkg.read<float>();

            //Dungeon::DoorDef door(*door_prefab, connection_type, chance);

            //door.m_prefab = *door_prefab;
            //door.m_connection_type = pkg.read<std::string>();
            //door.m_chance          = pkg.read<float>();

            //dungeon->m_door_types.push_back(door);
            dungeon->m_door_types.emplace_back(*door_prefab, std::move(connection_type), chance);
        }

        dungeon->m_grid_size          = pkg.read<std::int32_t>();
        dungeon->m_max_rooms          = pkg.read<std::int32_t>();
        dungeon->m_max_tilt           = pkg.read<float>();
        dungeon->m_min_altitude       = pkg.read<float>();
        dungeon->m_min_required_rooms = pkg.read<std::int32_t>();
        dungeon->m_min_rooms          = pkg.read<std::int32_t>();
        dungeon->m_perimeter_buffer   = pkg.read<float>();
        dungeon->m_perimeter_sections = pkg.read<std::int32_t>();
        dungeon->m_requiredRooms      = pkg.read<decltype(Dungeon::m_requiredRooms)>();

        dungeon->m_spawn_chance = pkg.read<float>();
        dungeon->m_themes       = pkg.read<avledet::util::Theme>();
        dungeon->m_tile_width   = pkg.read<float>();

        auto roomCount = pkg.read<std::int32_t>();
        for (int i2 = 0; i2 < roomCount; i2++) {
            auto room(std::make_unique<Room>());

            // Debug anchor
            if (pkg.read<avledet::util::Hash>() != VUtils::get_stable_hash("dungeonRoom")) {
                throw std::runtime_error("bad read anchor");
            }

            room->m_name          = pkg.read<std::string>();
            room->m_hash          = avledet::util::get_stable_hash(room->m_name);
            room->m_divider       = pkg.read<bool>();
            room->m_endCap        = pkg.read<bool>();
            room->m_endCapPrio    = pkg.read<std::int32_t>();
            room->m_entrance      = pkg.read<bool>();
            room->m_faceCenter    = pkg.read<bool>();
            room->m_minPlaceOrder = pkg.read<std::int32_t>();
            room->m_perimeter     = pkg.read<bool>();
            room->m_size          = pkg.read<Vector3f>();
            room->m_theme         = pkg.read<avledet::util::Theme>();
            room->m_weight        = pkg.read<float>();
            room->m_pos           = pkg.read<Vector3f>();
            room->m_rot           = pkg.read<Quaternion>();

            // Parsing room-connections
            auto connCount = pkg.read<std::int32_t>();
            for (int i3 = 0; i3 < connCount; i3++) {
                // TODO does this need to be unique? how are connections compared / referenced
                auto conn(std::make_unique<RoomConnection>());

                conn->m_type                          = pkg.read<std::string>();
                conn->m_entrance                      = pkg.read<bool>();
                conn->m_allowDoor                     = pkg.read<bool>();
                conn->m_doorOnlyIfOtherAlsoAllowsDoor = pkg.read<bool>();
                conn->m_localPos                      = pkg.read<Vector3f>();
                conn->m_localRot                      = pkg.read<Quaternion>();

                room->m_roomConnections.push_back(std::move(conn));
            }

            auto viewCount = pkg.read<std::int32_t>();
            for (int i3 = 0; i3 < viewCount; i3++) {
                Prefab::Instance instance;

                // Debug anchor
                if (pkg.read<avledet::util::Hash>() != VUtils::get_stable_hash("dungeonView")) {
                    throw std::runtime_error("bad read anchor");
                }

                auto viewName = pkg.read<std::string_view>();// For debug
                (void) viewName;
                instance.m_prefabHash = pkg.read<avledet::util::Hash>();
                instance.m_pos        = pkg.read<Vector3f>();
                instance.m_rot        = pkg.read<Quaternion>();

                // Write NV RandomSpawn

                // ensure prefab existence
                instance.get_prefab();

                room->m_netViews.push_back(instance);
            }

            // RandomSpawn list parsing
            room->m_random_spawns = avledet::gen::RandomSpawn::parse_list(pkg);

            dungeon->m_available_rooms.push_back(std::move(room));
        }

        avledet::util::Hash hash = dungeon->m_prefab.get().m_hash;
        m_dungeons.insert({hash, std::move(dungeon)});
    }

    LOG_NOTICE(AVL_LOGGER, "Loaded {} dungeons", count);
}

    #if AVL_IS_ON(AVL_DUNGEON_REGENERATION)
ZDO *IDungeonManager::TryRegenerateDungeon(ZDO dungeonZdo)
{
    static constexpr avledet::util::Hash LAST_RESET_HASH = avledet::util::get_stable_hash("Areas LastReset");

    // https://github.com/T3kla/ValMods/blob/52da19785190c2d9b6de93d09195d942e4da8686/~DungeonReset/Scripts/Extensions.cs#LL12C86-L12C86
    auto &&lastReset = std::chrono::seconds(dungeonZdo.GetLong(LAST_RESET_HASH));
    auto &&unixTime  = steady_clock::now().time_since_epoch();

    auto since = duration_cast<std::chrono::seconds>(unixTime) - lastReset;

    if (since > AVL_SETTINGS.dungeonsRegenerationInterval) {
        bool playerNear = false;

        // if a player is inside, do not reset
        for (auto &&peer : NetManager()->GetPeers()) {
            // if peer in dungeon sector, and they are high up (presumably inside the dungeon)
            if (dungeonZdo.get_zone() == ZoneManager()->WorldToZonePos(peer->m_pos) && peer->m_pos.y > 4000) {
                playerNear = true;
                break;
            }
        }

        auto &&dungeon = RequireDungeon(dungeonZdo.get_prefab().m_hash);

        // Destroy all zdos high in the sky near dungeon IN ZONE
        auto pos = dungeonZdo.get_position();
        auto rot = dungeonZdo.get_rotation();

        if (!playerNear) {
            auto zdos = ZDOManager()->GetZDOs(dungeonZdo.get_zone(), [](const ZDO zdo) {
                return zdo.get_position().y > 4000
                       && zdo.get_prefab().AllFlagsAbsent(Prefab::Flag::PLAYER | Prefab::Flag::TOMBSTONE);
            });

            for (auto &&ref : zdos) {
                auto &&zdo    = ref.get();
                auto &&prefab = zdo.get_prefab();

                assert(!(prefab.m_hash == avledet::util::hashes::Object::Player
                         || prefab.m_hash == avledet::util::hashes::Object::Player_tombstone));

                ZDOManager()->DestroyZDO(zdo);
            }

            LOG_INFO(AVL_LOGGER, "Regenerated {} at {}", dungeon.m_prefab->m_name, pos);

            auto &&zdo = Generate(dungeon, pos, rot).get();
            zdo.set(LAST_RESET_HASH, unixTime.count());

            return &zdo;
        } else {
            LOG_INFO(AVL_LOGGER, "Unable to regenerate {} at {} (peer is inside)", dungeon.m_prefab->m_name,
                     pos);
        }
    }

    return nullptr;
}

void IDungeonManager::TryRegenerateDungeons()
{
    std::size_t idx = m_nextIndex;
    while (idx
           < std::min(m_dungeonInstances.size(), m_nextIndex + AVL_SETTINGS.dungeonsRegenerationMaxSteps)) {
        auto &&itr = m_dungeonInstances.begin() + idx;

        ZDO *dungeonZdo = ZDOManager()->find_zdo(*itr);
        if (!dungeonZdo) {
            m_dungeonInstances.erase(itr);
            //LOG(WARNING) << "Dungeon ZDO no longer exists";
            break;
        } else {
            if (auto &&newDungeon = TryRegenerateDungeon(*dungeonZdo)) {
                *itr = newDungeon->get_id();
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


ZDO::reference IDungeonManager::generate(Dungeon const &dungeon, Vector3f pos, Quaternion rot)
{
    auto &&zdo = ZDOManager()->Instantiate(dungeon.m_prefab, pos);
    zdo->set_rotation(rot);

    DungeonGenerator(dungeon, zdo).Generate();

    return zdo;
}

ZDO::reference IDungeonManager::generate(Dungeon const &dungeon, Vector3f pos, Quaternion rot,
                                         avledet::util::Hash seed)
{
    auto &&zdo = ZDOManager()->Instantiate(dungeon.m_prefab, pos);
    zdo->set_rotation(rot);

    DungeonGenerator(dungeon, zdo).Generate(seed);

    return zdo;
}

void IDungeonManager::generate(Dungeon const &dungeon, ZDO::reference zdo)
{
    DungeonGenerator(dungeon, zdo).Generate();
}

std::vector<const Dungeon*> IDungeonManager::get_dungeons() const {
    return m_dungeons 
    | ranges::views::values 
    | ranges::views::transform([](auto const& p) -> const Dungeon* { return p.get(); })
    | ranges::to<std::vector>();
}

#endif