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

    using FLAG_t = uint64_t;

    struct Flags {
        static constexpr FLAG_t None = 0;

        static constexpr FLAG_t SyncInitialScale = 1ULL << 0;
        static constexpr FLAG_t Distant = 1ULL << 1;
        //static constexpr FLAG_t Persistent = 1ULL << 2;
        static constexpr FLAG_t Sessioned = 1ULL << 2;

        static constexpr FLAG_t Piece = 1ULL << 3;
        static constexpr FLAG_t Bed = 1ULL << 4;
        static constexpr FLAG_t Door = 1ULL << 5;
        static constexpr FLAG_t Chair = 1ULL << 6;
        static constexpr FLAG_t Ship = 1ULL << 7;
        static constexpr FLAG_t Fish = 1ULL << 8;
        static constexpr FLAG_t Plant = 1ULL << 9;
        static constexpr FLAG_t ArmorStand = 1ULL << 10;

        static constexpr FLAG_t ItemDrop = 1ULL << 11;
        static constexpr FLAG_t Pickable = 1ULL << 12;
        static constexpr FLAG_t PickableItem = 1ULL << 13;

        static constexpr FLAG_t CookingStation = 1ULL << 14;
        static constexpr FLAG_t CraftingStation = 1ULL << 15;
        static constexpr FLAG_t Smelter = 1ULL << 16;
        static constexpr FLAG_t Fireplace = 1ULL << 17;

        static constexpr FLAG_t WearNTear = 1ULL << 18;
        static constexpr FLAG_t Destructible = 1ULL << 19;
        static constexpr FLAG_t ItemStand = 1ULL << 20;

        static constexpr FLAG_t AnimalAI = 1ULL << 21;
        static constexpr FLAG_t MonsterAI = 1ULL << 22;
        static constexpr FLAG_t Tameable = 1ULL << 23;
        static constexpr FLAG_t Procreation = 1ULL << 24;

        static constexpr FLAG_t MineRock = 1ULL << 25;
        static constexpr FLAG_t MineRock5 = 1ULL << 26;
        static constexpr FLAG_t TreeBase = 1ULL << 27; // vegetation
        static constexpr FLAG_t TreeLog = 1ULL << 28; // chopped down

        static constexpr FLAG_t SFX = 1ULL << 29; // sound effect
        static constexpr FLAG_t VFX = 1ULL << 30; // visual effect
        static constexpr FLAG_t AOE = 1ULL << 31; // AOE attacks

        static constexpr FLAG_t Dungeon = 1ULL << 32;

        static constexpr FLAG_t TerrainModifier = 1ULL << 33;

        static constexpr FLAG_t Player = 1ULL << 34;
        static constexpr FLAG_t Tombstone = 1ULL << 35;

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

	uint64_t m_flags = 0;

    bool FlagsPresent(FLAG_t prefabFlags) const {
        return prefabFlags == Flags::None || (m_flags & prefabFlags) == prefabFlags;
    }

    bool FlagsAbsent(FLAG_t prefabFlags) const {
        return prefabFlags == Flags::None
            || (m_flags & prefabFlags) != prefabFlags;
    }

    bool operator==(const Prefab& other) const {
        return this->m_hash == other.m_hash;
    }
};
