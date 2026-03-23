#include "CompileSettings.h"
#include "Prefab.h"
#include <sol/property.hpp>

#if AVL_IS_ON(AVL_ENABLE_SCRIPTING)
    #include <sol/forward.hpp>

    #include "ModManager.h"
    #include "PrefabManager.h"

void IScriptManager::load_userdata_prefab()
{
    // clang-format off

    LOG_DEBUG(AVL_LOGGER, "Initializing API types - prefab");

    this->new_enum("Flag", 
        "NONE", Prefab::Flag::NONE,
        "SCALE", Prefab::Flag::SYNC_INITIAL_SCALE, 
        "DISTANT", Prefab::Flag::DISTANT,
        "PERSISTENT", Prefab::Flag::PERSISTENT, 
        "TYPE1", Prefab::Flag::TYPE1, 
        "TYPE2", Prefab::Flag::TYPE2,

        "PIECE", Prefab::Flag::PIECE, 
        "BED", Prefab::Flag::BED, 
        "DOOR", Prefab::Flag::DOOR,
        "CHAIR", Prefab::Flag::CHAIR, 
        "SHIP", Prefab::Flag::SHIP, 
        "FISH", Prefab::Flag::FISH,
        "PLANT", Prefab::Flag::PLANT, 
        "ARMOR_STAND", Prefab::Flag::ARMOR_STAND,

        "PROJECTILE", Prefab::Flag::PROJECTILE, 
        "ITEM_DROP", Prefab::Flag::ITEM_DROP, 
        "PICKABLE", Prefab::Flag::PICKABLE, 
        "PICKABLE_ITEM", Prefab::Flag::PICKABLE_ITEM,

        "CONTAINER", Prefab::Flag::CONTAINER, 
        "COOKING_STATION", Prefab::Flag::COOKING_STATION,
        "CRAFTING_STATION", Prefab::Flag::CRAFTING_STATION, 
        "SMELTER", Prefab::Flag::SMELTER,
        "FIREPLACE", Prefab::Flag::FIREPLACE,

        "WEAR_N_TEAR", Prefab::Flag::WEAR_N_TEAR, 
        "DESTRUCTIBLE", Prefab::Flag::DESTRUCTIBLE,
        "ITEM_STAND", Prefab::Flag::ITEM_STAND,

        "ANIMAL_AI", Prefab::Flag::ANIMAL_AI, 
        "MONSTER_AI", Prefab::Flag::MONSTER_AI, 
        "TAMEABLE", Prefab::Flag::TAMEABLE, 
        "PROCREATION", Prefab::Flag::PROCREATION,

        "MINE_ROCK_5", Prefab::Flag::MINE_ROCK_5, 
        "TREE_BASE", Prefab::Flag::TREE_BASE,
        "TREE_LOG", Prefab::Flag::TREE_LOG,

        "DUNGEON", Prefab::Flag::DUNGEON, 
        "TERRAIN_MODIFIER", Prefab::Flag::TERRAIN_MODIFIER,
        "CREATURE_SPAWNER", Prefab::Flag::CREATURE_SPAWNER);

    this->new_usertype<Prefab>("Prefab", 
        sol::no_constructor, 
        "name", sol::readonly(&Prefab::m_name), 
        "hash", sol::readonly(&Prefab::m_hash),
        "flags_all", &Prefab::AllFlagsPresent,
        "flags_any", &Prefab::AnyFlagsPresent, 
        "flags_none", &Prefab::AllFlagsAbsent,
        "flags_absent", &Prefab::AnyFlagsAbsent,
        "distant", sol::property(&Prefab::is_distant),
        "persistent", sol::property(&Prefab::is_persistent),
        "type", sol::property(&Prefab::GetObjectType)
    );

    // https://commons.wikimedia.org/wiki/File:IEEE754.svg#/media/File:IEEE754.svg
    // When converting flag double to int from lua->c++, double finely represents all integral values with about
    //  32 bits being perfectly represented
    //  when masking and combining about 35+ bits, a double cannot represent this integral number accurately
    //  ive about reached the limit of using bitflags with lua, and will have to opt for a different type (I dont want to use the intwrapper for flags)


    this->new_usertype<IPrefabManager>("IPrefabManager", 
        sol::no_constructor,
        "find_prefab", sol::overload(
            sol::resolve<Prefab const *(avledet::util::Hash) const>(&IPrefabManager::find_prefab),
            sol::resolve<Prefab const *(std::string_view) const>(&IPrefabManager::find_prefab)),
        // TODO restrict prefab registration to startup only
        //  Will require an event handler to be called prior to ZDOs being loaded
        "register", sol::overload(
            sol::resolve<void(std::string, Vector3f, Prefab::Flag)>(&IPrefabManager::Register),
            sol::resolve<void(DataReader&)>(&IPrefabManager::Register))
    );

    // clang-format on
}

#endif
