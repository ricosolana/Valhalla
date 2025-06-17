#include "DungeonManager.h"
#include "ModManager.h"
#include "ZoneManager.h"

#if VH_IS_ON(VH_USE_MODS)

void avledet::api::init_zone(sol::table table)
{
    LOG_DEBUG(VH_LOGGER, "Initializing API types - Zone");

    //or VH_DUNGEON_GENERATION?
    #if VH_IS_ON(VH_ZONE_GENERATION)
    table.new_usertype<Dungeon>(
            "Dungeon", sol::no_constructor
            //"Generate", sol::resolve<void(const Vector3f& pos, const Quaternion& rot) const>(&Dungeon::Generate)
    );

    table["DungeonManager"] = DungeonManager();
    table.new_usertype<IDungeonManager>(
            "IDungeonManager", "find_dungeon",
            [](IDungeonManager &self, std::string_view name) {
                return self.find_dungeon(avledet::util::get_stable_hash(name));
            },
            // TODO must make get_dungeon for lua use reference I guess...
            //"get_dungeon", [](IDungeonManager& self, std::string_view name) { return self.get_dungeon(get_stable_hash(name)); }, //breaks compilation
            "generate",
            [](IDungeonManager &self, Dungeon &dungeon, Vector3f pos, Quaternion rot) {
                self.generate(dungeon, pos, rot);
            });


    table.new_usertype<IZoneManager::Feature::Instance>(
            "FeatureInstance", "pos",
            sol::property([](IZoneManager::Feature::Instance &self) { return self.m_pos; }));
    #endif

    table["ZoneManager"] = ZoneManager();
    table.new_usertype<IZoneManager>("IZoneManager",
    #if VH_IS_ON(VH_ZONE_GENERATION)
                                     "populate_zone", sol::resolve<void(ZoneID)>(&IZoneManager::PopulateZone),
    #endif
                                     "get_nearest_feature", &IZoneManager::GetNearestFeature, "to_zone_pos",
                                     &IZoneManager::WorldToZonePos, "to_world_pos",
                                     &IZoneManager::ZoneToWorldPos, "global_keys",
                                     sol::property(&IZoneManager::GlobalKeys));
}

#endif
