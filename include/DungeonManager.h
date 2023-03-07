#pragma once

#include <robin_hood.h>

#include "VUtils.h"
#include "Prefab.h"
#include "Dungeon.h"

class IDungeonManager {
	friend class IZDOManager;
	friend class IZoneManager;

private:
	robin_hood::unordered_map<HASH_t, std::unique_ptr<Dungeon>> m_dungeons;

	robin_hood::unordered_set<ZDOID> m_dungeonInstances;

public:
    void Init();

	const Dungeon* GetDungeon(HASH_t hash) const {
		auto&& find = m_dungeons.find(hash);
		if (find != m_dungeons.end())
			return find->second.get();
		return nullptr;
	}

	void RegenerateDungeons();

	ZDO& Generate(const Dungeon& dungeon, const Vector3& pos, const Quaternion& rot);
	ZDO& Generate(const Dungeon& dungeon, const Vector3& pos, const Quaternion& rot, HASH_t seed);
	void Generate(const Dungeon& dungeon, ZDO& zdo);
};

// Manager for everything related to dungeon spawning 
IDungeonManager* DungeonManager();
