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
        // Single bit flags:

        SyncInitialScale = 1ULL << 0,
        Distant = 1ULL << 1,
        Persistent = 1ULL << 2,

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
        TreeLog = 1ULL << 28, // chopped down physics tree
        
        SFX = 1ULL << 29, // sound effect
        VFX = 1ULL << 30, // visual effect
        AOE = 1ULL << 31, // AOE attacks

        Dungeon = 1ULL << 32,

        //CharacterDrop,
        //DropOnDestroyed,
	};

	Prefab() = default;

	std::string m_name;
	HASH_t m_hash = 0;

	ZDO::ObjectType m_type = ZDO::ObjectType::Default; // TODO store in flags

	Vector3 m_localScale;

	uint64_t m_flags = 0;

    bool HasFlag(Flag flag) const {
        return m_flags & std::to_underlying(flag);
    }

    bool operator==(const Prefab& other) const {
        return this->m_hash == other.m_hash;
    }
};
