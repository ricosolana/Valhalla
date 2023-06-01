#pragma once

#include "VUtils.h"

#if VH_IS_ON(VH_STANDARD_PREFABS)
#include "VUtilsString.h"

#include "Vector.h"
#include "Quaternion.h"

class Prefab {
public:
    struct Instance {
        const Prefab* m_prefab = nullptr;
        Vector3f m_pos;
        Quaternion m_rot;
    };

    // MineRock/5 is interesting
    //  MineRock is not used throughout the game, except for Leviathan
    //  MineRock5 is used by every pickaxe mineable thing (rocks / copper / icon / scrap / Plains monoliths / scrap piles / pretty much everything)
    //  - The '5' in MineRock5 could mean:
    //      - an earlier naming where it used to have 5 mineable parts?
    //      - the 5th revision of the MineRockX
    //      - im not really sure of the naming besides this

    enum class Flag : uint64_t {
        NONE = 0,

        SYNC_INITIAL_SCALE = 1ULL << 0,
        DISTANT = 1ULL << 1,
        SESSIONED = 1ULL << 2,

        PIECE = 1ULL << 3,
        BED = 1ULL << 4,
        DOOR = 1ULL << 5,
        CHAIR = 1ULL << 6,
        SHIP = 1ULL << 7,
        FISH = 1ULL << 8,
        PLANT = 1ULL << 9,
        ARMOR_STAND = 1ULL << 10,

        ITEM_DROP = 1ULL << 11,
        PICKABLE = 1ULL << 12,
        PICKABLE_ITEM = 1ULL << 13,

        COOKING_STATION = 1ULL << 14,
        CRAFTING_STATION = 1ULL << 15,
        SMELTER = 1ULL << 16,
        FIREPLACE = 1ULL << 17,

        WEAR_N_TEAR = 1ULL << 18,
        DESTRUCTIBLE = 1ULL << 19,
        ITEM_STAND = 1ULL << 20,

        ANIMAL_AI = 1ULL << 21,
        MONSTER_AI = 1ULL << 22,
        TAMEABLE = 1ULL << 23,
        PROCREATION = 1ULL << 24,

        // Unused rock, except for Leviathan
        MINE_ROCK = 1ULL << 25, 
        // Use this for all normal in-game Mineables
        MINE_ROCK_5 = 1ULL << 26,
        TREE_BASE = 1ULL << 27, // vegetation
        TREE_LOG = 1ULL << 28, // chopped down

        SFX = 1ULL << 29, // sound effect
        VFX = 1ULL << 30, // visual effect
        AOE = 1ULL << 31, // AOE attacks

        DUNGEON = 1ULL << 32,

        TERRAIN_MODIFIER = 1ULL << 33,

        PLAYER = 1ULL << 34,
        TOMBSTONE = 1ULL << 35,

        // TODO add (order of importance): 
        //  - Container
        //  - Projectile
        //  - MusicLocation
        //  - Floating
        //  - Gibber        
        //  - Mister (area/edge/small)
        //  - NpcTalk
        // TODO remove (if more bits needed):
        //  - MineRock
        //  - ItemStand (4 prefabs, also used by boss stones...)
        //  - Tombstone
        //  - Player
        //  - Bed? (there are 2 types of beds)
        //  - ArmorStand (3 types: M/F/default)
        //  - CookingStation (3 types: reg/iron/oven)
        //  - Procreation (4 types)
        //  - Ship (4 types: includes trailership)


        //static constexpr FLAG_t BossStone = 1ULL << 36;
        //static constexpr FLAG_t Container = 1ULL << 37;
        //static constexpr FLAG_t Corpse = 1ULL << 38;
        //static constexpr FLAG_t DropOnDestroyed = 1ULL << 39;
        //static constexpr FLAG_t CharacterDrop = 1ULL << 40;
        //static constexpr FLAG_t Incinerator = 1ULL << 41;
        //static constexpr FLAG_t DropOnDestroyed = 1ULL << 39;
    };


    // TODO remove VH_USE_PREFABS
    //  replace with VH_COMPACT_PREFAB
    //  prefabs will always be needed, just a matter of how much is needed
    //  since prefabs go up to 2271, can simply use indexes instead of large 32 bit hash per zdo (much better this way)
    //  until the day Valheim has more prefabs than are available in ~12 bits, everything should work
    
public:
    const HASH_t m_hash;

    std::string m_name;

    Type m_type = Type::DEFAULT; // TODO store in flags

    Vector3f m_localScale;
    Flag m_flags = Flag::NONE;

public:
    Prefab(std::string_view name, Type type, Vector3f localScale, Flag flags)
        : m_hash(VUtils::String::GetStableHashCode(name)), m_name(std::string(name)), m_type(type), m_localScale(localScale), m_flags(flags) {}

    Prefab(const Prefab& other) = default;



    bool AllFlagsPresent(Flag prefabFlags) const {
        return prefabFlags == Flag::NONE 
            || (m_flags & prefabFlags) == prefabFlags;
    }

    bool AnyFlagsPresent(Flag prefabFlags) const {
        return prefabFlags == Flag::NONE 
            || (m_flags & prefabFlags) != Flag::NONE;
    }

    bool AllFlagsAbsent(Flag prefabFlags) const {
        return prefabFlags == Flag::NONE
            || (m_flags & prefabFlags) == Flag::NONE;
    }

    bool AnyFlagsAbsent(Flag prefabFlags) const {
        return prefabFlags == Flag::NONE
            || (m_flags & prefabFlags) != prefabFlags;
    }



    bool operator==(const Prefab& other) const {
        return this->m_hash == other.m_hash;
    }
};
#endif