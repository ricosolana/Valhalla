#pragma once

#include "Vector.h"
#include "Quaternion.h"
#include "ZDO.h"

//class IPrefabManager;

class Prefab {
public:
    enum class Flag {
        // Single bit flags:

        SyncInitialScale = 0,
        Distant,
        Persistent,
        Piece,

        // Unique type flags (ie a bed is not a door, just like how an itemstand can never be a rock)

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

        //CharacterDrop,
        //DropOnDestroyed,
	};

	Prefab() = default;

	//const HASH_t m_hash;
	std::string m_name;
	HASH_t m_hash = 0; // precomputed from m_name

	// 7 6 5 4 3 2 1 0
	// 0 0 0 0 D P T T
	//uint8_t m_flags;

	//bool m_distant = false;
	//bool m_persistent = false;
	ZDO::ObjectType m_type = ZDO::ObjectType::Default;

	// if scale is not {1, 1, 1}, then assume m_syncInitialScale is true
	Vector3 m_localScale;

	uint64_t m_flags = 0;

    bool HasFlag(Flag flag) const {
        return m_flags & ((uint64_t)1 << std::to_underlying(flag));
    }

    bool operator==(const Prefab& other) const {
        return this->m_hash == other.m_hash;
    }

};
