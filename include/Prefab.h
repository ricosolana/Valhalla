#pragma once

#include "Vector.h"
#include "Quaternion.h"
#include "ZDO.h"

//class IPrefabManager;

class Prefab {
public:

    // TODO use in ZoneManager::ZoneLocations
    struct Instance {
        const Prefab* m_prefab = nullptr;
        Vector3 m_pos;
        Quaternion m_rot;
    };

    enum class Flag : uint64_t {
        None = 0,

        SyncInitialScale = 1ULL << 0,
        Distant = 1ULL << 1,
        Persistent = 1ULL << 2,
        Sessioned = 1ULL << 2,

        Piece = 1ULL << 3,
        Bed = 1ULL << 4,
        Door = 1ULL << 5,
        Chair = 1ULL << 6,
        Ship = 1ULL << 7,
        Fish = 1ULL << 8,
        Plant = 1ULL << 9,
        ArmorStand = 1ULL << 10,

        ItemDrop = 1ULL << 11,
        Pickable = 1ULL << 12,
        PickableItem = 1ULL << 13,

        CookingStation = 1ULL << 14,
        CraftingStation = 1ULL << 15,
        Smelter = 1ULL << 16,
        Fireplace = 1ULL << 17,

        WearNTear = 1ULL << 18,
        Destructible = 1ULL << 19,
        ItemStand = 1ULL << 20,

        AnimalAI = 1ULL << 21,
        MonsterAI = 1ULL << 22,
        Tameable = 1ULL << 23,
        Procreation = 1ULL << 24,

        MineRock = 1ULL << 25,
        MineRock5 = 1ULL << 26,
        TreeBase = 1ULL << 27, // vegetation
        TreeLog = 1ULL << 28, // chopped down

        SFX = 1ULL << 29, // sound effect
        VFX = 1ULL << 30, // visual effect
        AOE = 1ULL << 31, // AOE attacks

        Dungeon = 1ULL << 32,

        TerrainModifier = 1ULL << 33,

        Player = 1ULL << 34,
        Tombstone = 1ULL << 35,

        //static constexpr FLAG_t BossStone = 1ULL << 36;
        //static constexpr FLAG_t Container = 1ULL << 37;
        //static constexpr FLAG_t Corpse = 1ULL << 38;
        //static constexpr FLAG_t DropOnDestroyed = 1ULL << 39;
        //static constexpr FLAG_t CharacterDrop = 1ULL << 40;
        //static constexpr FLAG_t Incinerator = 1ULL << 41;
        //static constexpr FLAG_t DropOnDestroyed = 1ULL << 39;
    };

	Prefab() = default;

	std::string m_name;
	HASH_t m_hash = 0;

	ZDO::ObjectType m_type = ZDO::ObjectType::Default; // TODO store in flags

	Vector3 m_localScale;

	Flag m_flags = Flag::None;

    bool FlagsPresent(Flag prefabFlags) const {
        return prefabFlags == Flag::None || (m_flags & prefabFlags) == prefabFlags;
    }

    bool FlagsAbsent(Flag prefabFlags) const {
        return prefabFlags == Flag::None
            || (m_flags & prefabFlags) != prefabFlags;
    }

    bool operator==(const Prefab& other) const {
        return this->m_hash == other.m_hash;
    }
};
