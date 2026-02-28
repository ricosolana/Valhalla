#include "CompileSettings.h"
#include "Dungeon.h"
#include <sol/property.hpp>

#if AVL_IS_ON(AVL_ENABLE_SCRIPTING)

    #include <sol/forward.hpp>

    #include "DungeonManager.h"
    #include "ModManager.h"
    #include "ZoneManager.h"

void IScriptManager::load_userdata_zone()
{
    LOG_DEBUG(AVL_LOGGER, "Initializing API types - Zone");

    // clang-format off

    //or AVL_DUNGEON_GENERATION?
#if AVL_IS_ON(AVL_ZONE_GENERATION)
    this->new_usertype<Dungeon>("Dungeon", 
        sol::no_constructor,
        "name", sol::property(&Dungeon::get_name)
        //"", Dungeon::
        //"Generate", sol::resolve<void(const Vector3f& pos, const Quaternion& rot) const>(&Dungeon::Generate)
    );

    this->new_usertype<IDungeonManager>("IDungeonManager", 
        sol::no_constructor,
        "find_dungeon", [](IDungeonManager &self, std::string_view name) {
            return self.find_dungeon(avledet::util::get_stable_hash(name));
        },
        // TODO must make get_dungeon for lua use reference I guess...
        //"get_dungeon", [](IDungeonManager& self, std::string_view name) { return self.get_dungeon(avledet::util::get_stable_hash(name)); }, //breaks compilation
        "generate", [](IDungeonManager &self, Dungeon &dungeon, Vector3f pos, Quaternion rot) {
            self.generate(dungeon, pos, rot);
        }
        //"dungeons", sol::property(&IDungeonManager::get_dungeons)
        // TODO dungeons meant to be immutable
        //  which readonly doesnt quite achieve (prevents assignment, not index assign)
        //"dungeons", sol::readonly(&IDungeonManager::m_dungeons)
    );
//DungeonManager()->m_dungeons.end()
    this->new_usertype<IZoneManager::Feature::Instance>("FeatureInstance", 
        sol::no_constructor,
        "pos", sol::property([](IZoneManager::Feature::Instance &self) { return self.m_pos; })
    );

#endif

    this->new_usertype<IZoneManager>("IZoneManager",
        sol::no_constructor,
#if AVL_IS_ON(AVL_ZONE_GENERATION)
        "populate_zone", sol::resolve<void(ZoneID)>(&IZoneManager::PopulateZone),
#endif
        "find_nearest_feature", &IZoneManager::find_nearest_feature, 
        "to_zone_pos", &IZoneManager::WorldToZonePos, 
        "to_world_pos", &IZoneManager::ZoneToWorldPos, 
        "global_keys", sol::property(&IZoneManager::m_globalKeys)
    );

    // clang-format on
}

#endif
