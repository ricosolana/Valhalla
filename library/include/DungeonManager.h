#pragma once

#include "VUtils.h"

#if AVL_IS_ON(AVL_DUNGEON_GENERATION)

    #include "Dungeon.h"
    #include "Prefab.h"
    #include "ZDO.h"

class IDungeonManager
{
    friend class IZDOManager;
    friend class IZoneManager;

  private:
    avledet::util::Map<avledet::util::Hash, std::unique_ptr<Dungeon>> m_dungeons;

    //robin_hood::unordered_set<ZDOID> m_dungeonInstances;
    std::vector<ZDOID> m_dungeonInstances;
    std::size_t m_nextIndex = 0;

  public:
    void post_prefab_init();

    Dungeon const *find_dungeon(avledet::util::Hash hash) const
    {
        auto &&find = m_dungeons.find(hash);
        if (find != m_dungeons.end())
            return find->second.get();
        return nullptr;
    }

    Dungeon const &get_dungeon(avledet::util::Hash hash) const
    {
        auto &&dungeon = find_dungeon(hash);
        if (!dungeon)
            throw std::runtime_error("unknown dungeon");
        return *dungeon;
    }

    // Try to replace the target dungeon with a newly generated one
    //	Returns the new dungeon (dungeonZdo is invalidated)
    //	Returns null if replacement failed (dungeonZdo remains valid)
    #if AVL_IS_ON(AVL_DUNGEON_REGENERATION)
    ZDO *TryRegenerateDungeon(ZDO dungeonZdo);
    void TryRegenerateDungeons();
    #endif

    ZDO::reference generate(Dungeon const &dungeon, Vector3f pos, Quaternion rot);
    ZDO::reference generate(Dungeon const &dungeon, Vector3f pos, Quaternion rot, avledet::util::Hash seed);
    void generate(Dungeon const &dungeon, ZDO::reference zdo);
};

// Manager for everything related to dungeon spawning
IDungeonManager *DungeonManager();

#endif