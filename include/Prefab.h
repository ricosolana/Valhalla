#pragma once

#include "VUtils.h"

#include "VUtilsString.h"

#include "Vector.h"
#include "Quaternion.h"
#include "Types.h"

class Prefab {
public:
    struct Instance {
        Quaternion m_rot;       // 16 bytes
        Vector3f m_pos;         // 12 bytes
        HASH_t m_prefabHash;    // 4 bytes

        const Prefab& GetPrefab() const;
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
        PERSISTENT = 1ULL << 2,
        TYPE1 = 1ULL << 3,
        TYPE2 = 1ULL << 4,

        PIECE = 1ULL << 5,
        BED = 1ULL << 6,
        DOOR = 1ULL << 7,
        CHAIR = 1ULL << 8,
        SHIP = 1ULL << 9,
        FISH = 1ULL << 10,
        PLANT = 1ULL << 11,
        ARMOR_STAND = 1ULL << 12,

        PROJECTILE = 1ULL << 13,
        ITEM_DROP = 1ULL << 14,
        PICKABLE = 1ULL << 15,
        PICKABLE_ITEM = 1ULL << 16,

        CONTAINER = 1ULL << 17,
        COOKING_STATION = 1ULL << 18,
        CRAFTING_STATION = 1ULL << 19,
        SMELTER = 1ULL << 20,
        FIREPLACE = 1ULL << 21,

        WEAR_N_TEAR = 1ULL << 22,
        DESTRUCTIBLE = 1ULL << 23,
        ITEM_STAND = 1ULL << 24,

        ANIMAL_AI = 1ULL << 25,
        MONSTER_AI = 1ULL << 26,
        TAMEABLE = 1ULL << 27,
        PROCREATION = 1ULL << 28,

        MINE_ROCK_5 = 1ULL << 29, // rocks
        TREE_BASE = 1ULL << 30, // vegetation
        TREE_LOG = 1ULL << 31, // chopped down

        DUNGEON = 1ULL << 32,
        TERRAIN_MODIFIER = 1ULL << 33,
        CREATURE_SPAWNER = 1ULL << 34,
        SYNCED_TRANSFORM = 1ULL << 35
    };
        
public:
    std::string m_name;         // 40 bytes
    Vector3f m_localScale;      // 12 bytes
    Flag m_flags = Flag::NONE;  // 8 bytes
    HASH_t m_hash;              // 4 bytes

public:
    Prefab(std::string_view name, Vector3f localScale, Flag flags)
        : m_hash(VUtils::String::GetStableHashCode(name)), m_name(std::string(name)), m_localScale(localScale), m_flags(flags) {}

    Prefab(const Prefab& other) = default;



    bool AllFlagsPresent(Flag prefabFlags) const noexcept {
        return prefabFlags == Flag::NONE 
            || (std::to_underlying(m_flags) & std::to_underlying(prefabFlags)) == std::to_underlying(prefabFlags);
    }

    bool AnyFlagsPresent(Flag prefabFlags) const noexcept {
        return prefabFlags == Flag::NONE 
            || (std::to_underlying(m_flags) & std::to_underlying(prefabFlags)) != std::to_underlying(Flag::NONE);
    }

    bool AllFlagsAbsent(Flag prefabFlags) const noexcept {
        return prefabFlags == Flag::NONE
            || (std::to_underlying(m_flags) & std::to_underlying(prefabFlags)) == std::to_underlying(Flag::NONE);
    }

    bool AnyFlagsAbsent(Flag prefabFlags) const noexcept {
        return prefabFlags == Flag::NONE
            || (std::to_underlying(m_flags) & std::to_underlying(prefabFlags)) != std::to_underlying(prefabFlags);
    }



    ObjectType GetObjectType() const noexcept;



    bool operator==(const Prefab& other) const noexcept {
        return this->m_hash == other.m_hash;
    }

    bool operator==(HASH_t other) const noexcept {
        return this->m_hash == other;
    }

    bool operator==(std::string_view other) const noexcept {
        return this->m_hash == VUtils::String::GetStableHashCode(other);
    }
};