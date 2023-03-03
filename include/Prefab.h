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

    enum class Flag {
        // Single bit flags:

        SyncInitialScale = 0,
        Distant,
        Persistent,

        Piece,
        Bed,
        Door,
        Chair,
        Ship,
        Fish,
        Plant,
        ArmorStand,

        ItemDrop,
        Pickable,
        PickableItem,

        CookingStation,
        CraftingStation,
        Smelter,
        Fireplace,

        WearNTear,
        Destructible,
        ItemStand,

        AnimalAI,
        MonsterAI,
        Tameable,
        Procreation,

        MineRock,
        MineRock5, 
        TreeBase, // vegetation
        TreeLog, // chopped down physics tree
        
        SFX, // sound effect
        VFX, // visual effect
        AOE, // AOE attacks

        Dungeon,

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
        return m_flags & ((uint64_t)1 << std::to_underlying(flag));
    }

    bool operator==(const Prefab& other) const {
        return this->m_hash == other.m_hash;
    }
};
