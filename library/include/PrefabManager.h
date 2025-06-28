#pragma once

#include <cstddef>

#include <gtl/btree.hpp>

#include "DataStream.h"
#include "Prefab.h"
#include "Types.h"

// TODO consider moving Instantiate(...) to ZDOManager
//	this class doesnt do much besides try to simulate Unity in appearance
//		which is not the desired result...
class IPrefabManager
{
    friend class IDiscordManager;

  private:
    struct cmp
    {
        using is_transparent = void;

        bool operator()(Prefab const &lhs, Prefab const &rhs) const
        {
            return lhs.m_hash < rhs.m_hash;
        }

        bool operator()(avledet::util::Hash const &lhs, Prefab const &rhs) const
        {
            return lhs < rhs.m_hash;
        }

        bool operator()(Prefab const &lhs, avledet::util::Hash const &rhs) const
        {
            return lhs.m_hash < rhs;
        }
    };

    // TODO use set and use hash within from prefab
    avledet::util::Set<Prefab, ankerl::unordered_dense::hash<Prefab>, std::equal_to<>> m_prefabs;
    //gtl::btree_set<Prefab,
    //               decltype([](Prefab const &lhs, Prefab const &rhs) { return lhs.m_hash < rhs.m_hash; })>
    //        m_prefabs;// ordered

    //gtl::btree_set<Prefab,
    //               cmp> m_prefabs;// ordered


  public:
    void Init();

    Prefab const *find_prefab(avledet::util::Hash hash) const;

    // Get a prefab by name
    //	Returns the prefab or null
    Prefab const *find_prefab(std::string_view name) const;

    // Get a definite prefab
    //	Throws if prefab not found
    Prefab const &get_prefab(avledet::util::Hash hash) const;

    // Get a definite prefab
    //	Throws if prefab not found
    Prefab const &get_prefab(std::string_view name) const;

    Prefab const &get_indexed_prefab(std::size_t index) const;

    std::size_t get_prefab_index(avledet::util::Hash hash) const;

    std::size_t get_prefab_index(Prefab &prefab) const;

    void Register(std::string name, Vector3f scale, Prefab::Flag flags);

    void Register(DataReader &reader);
};

// Manager class for everything related to ZDO-belonging Prefabs and their base data
IPrefabManager *PrefabManager();
