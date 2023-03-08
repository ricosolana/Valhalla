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

    using Flag = uint64_t;

    struct Flags {
        static constexpr Flag None = 0;

        static constexpr Flag SyncInitialScale = 1ULL << 0;
        static constexpr Flag Distant = 1ULL << 1;
        static constexpr Flag Persistent = 1ULL << 2;

        static constexpr Flag Piece = 1ULL << 3;
        static constexpr Flag Bed = 1ULL << 4;
        static constexpr Flag Door = 1ULL << 5;
        static constexpr Flag Chair = 1ULL << 6;
        static constexpr Flag Ship = 1ULL << 7;
        static constexpr Flag Fish = 1ULL << 8;
        static constexpr Flag Plant = 1ULL << 9;
        static constexpr Flag ArmorStand = 1ULL << 10;

        static constexpr Flag ItemDrop = 1ULL << 11;
        static constexpr Flag Pickable = 1ULL << 12;
        static constexpr Flag PickableItem = 1ULL << 13;

        static constexpr Flag CookingStation = 1ULL << 14;
        static constexpr Flag CraftingStation = 1ULL << 15;
        static constexpr Flag Smelter = 1ULL << 16;
        static constexpr Flag Fireplace = 1ULL << 17;

        static constexpr Flag WearNTear = 1ULL << 18;
        static constexpr Flag Destructible = 1ULL << 19;
        static constexpr Flag ItemStand = 1ULL << 20;

        static constexpr Flag AnimalAI = 1ULL << 21;
        static constexpr Flag MonsterAI = 1ULL << 22;
        static constexpr Flag Tameable = 1ULL << 23;
        static constexpr Flag Procreation = 1ULL << 24;

        static constexpr Flag MineRock = 1ULL << 25;
        static constexpr Flag MineRock5 = 1ULL << 26;
        static constexpr Flag TreeBase = 1ULL << 27; // vegetation
        static constexpr Flag TreeLog = 1ULL << 28; // chopped down

        static constexpr Flag SFX = 1ULL << 29; // sound effect
        static constexpr Flag VFX = 1ULL << 30; // visual effect
        static constexpr Flag AOE = 1ULL << 31; // AOE attacks

        static constexpr Flag Dungeon = 1ULL << 32;

        static constexpr Flag TerrainModifier = 1ULL << 33;

        static constexpr Flag Player = 1ULL << 34;
        static constexpr Flag Tombstone = 1ULL << 35;
    };

	Prefab() = default;

	std::string m_name;
	HASH_t m_hash = 0;

	ZDO::ObjectType m_type = ZDO::ObjectType::Default; // TODO store in flags

	Vector3 m_localScale;

	uint64_t m_flags = 0;

    bool FlagsPresent(Flag prefabFlags) const {
        return (m_flags & prefabFlags) == prefabFlags;
    }

    bool FlagsAbsent(Flag prefabFlags) const {
        return !prefabFlags 
            || (m_flags & prefabFlags) != prefabFlags;
    }

    bool operator==(const Prefab& other) const {
        return this->m_hash == other.m_hash;
    }
};
