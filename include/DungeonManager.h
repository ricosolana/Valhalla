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
    void PostPrefabInit();

	const Dungeon* GetDungeon(HASH_t hash) const {
		auto&& find = m_dungeons.find(hash);
		if (find != m_dungeons.end())
			return find->second.get();
		return nullptr;
	}

	const Dungeon& RequireDungeon(HASH_t hash) const {
		auto&& dungeon = GetDungeon(hash);
		if (!dungeon)
			throw std::runtime_error("unknown dungeon");
		return *dungeon;
	}

	// Try to replace the target dungeon with a newly generated one
	//	Returns the new dungeon (dungeonZdo is invalidated)
	//	Returns null if replacement failed (dungeonZdo remains valid)
#if VH_IS_ON(VH_DUNGEON_REGENERATION)
	ZDO* TryRegenerateDungeon(ZDO dungeonZdo);
	void TryRegenerateDungeons();
#endif

	std::reference_wrapper<ZDO> Generate(const Dungeon& dungeon, Vector3f pos, Quaternion rot);
	std::reference_wrapper<ZDO> Generate(const Dungeon& dungeon, Vector3f pos, Quaternion rot, HASH_t seed);
	void Generate(const Dungeon& dungeon, ZDO zdo);
};

// Manager for everything related to dungeon spawning 
IDungeonManager* DungeonManager();

#endif