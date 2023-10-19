#pragma once

#include "VUtils.h"

#if VH_IS_ON(VH_DUNGEON_GENERATION)

#include "Prefab.h"
#include "Dungeon.h"
#include "ZDO.h"

class IDungeonManager {
	friend class IZDOManager;
	friend class IZoneManager;

private:
	UNORDERED_MAP_t<HASH_t, std::unique_ptr<Dungeon>> m_dungeons;

	//robin_hood::unordered_set<ZDOID> m_dungeonInstances;
	std::vector<ZDOID> m_dungeonInstances;
	size_t m_nextIndex = 0;

public:
    void post_prefab_init();

    // Nullable
	const Dungeon* get_dungeon(HASH_t hash) const {
		auto&& find = m_dungeons.find(hash);
		if (find != m_dungeons.end())
			return find->second.get();
		return nullptr;
	}

    // TODO use reference_wrapper
	const Dungeon& require_dungeon(HASH_t hash) const {
		auto&& dungeon = get_dungeon(hash);
		if (!dungeon)
			throw std::runtime_error("unknown dungeon");
		return *dungeon;
	}

	// Try to replace the target dungeon with a newly generated one
	//	Returns the new dungeon (dungeonZdo is invalidated)
	//	Returns null if replacement failed (dungeonZdo remains valid)
#if VH_IS_ON(VH_DUNGEON_REGENERATION)
	ZDO* regenerate_dungeon(ZDO& dungeonZdo);
	void regenerate_dungeons();
#endif

	std::reference_wrapper<ZDO> generate(const Dungeon& dungeon, Vector3f pos, Quaternion rot);
	std::reference_wrapper<ZDO> generate(const Dungeon& dungeon, Vector3f pos, Quaternion rot, HASH_t seed);
	void generate(const Dungeon& dungeon, ZDO& zdo);
};

// Manager for everything related to dungeon spawning 
IDungeonManager* DungeonManager();

#endif