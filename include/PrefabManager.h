#pragma once

#include "VUtils.h"
//#include <gtl/btree.hpp>

#include "VUtilsString.h"
#include "HashUtils.h"

#include "Prefab.h"
#include "DataReader.h"

// TODO consider moving Instantiate(...) to ZDOManager
//	this class doesnt do much besides try to simulate Unity in appearance
//		which is not the desired result...
class IPrefabManager {
private:
#if VH_IS_ON(VH_STANDARD_PREFABS)
	UNORDERED_SET_t<Prefab, ankerl::unordered_dense::prefab_hash, std::equal_to<>> m_prefabs;
#else
	//gtl::btree_set<HASH_t> m_prefabs;
	// Sorted by hash
	std::vector<HASH_t> m_prefabs;
#endif

public:
	void Init();

#if VH_IS_ON(VH_STANDARD_PREFABS)
	const Prefab* GetPrefab(HASH_t hash) const {
		auto&& find = m_prefabs.find(hash);
		if (find != m_prefabs.end())
			return &(*find);
		return nullptr;
	}

	// Get a prefab by name
	//	Returns the prefab or null
	const Prefab* GetPrefab(std::string_view name) const {
		return GetPrefab(VUtils::String::GetStableHashCode(name));
	}

	// Get a definite prefab
	//	Throws if prefab not found
	const Prefab& RequirePrefabByHash(HASH_t hash) const {
		auto prefab = GetPrefab(hash);
		if (!prefab)
			throw std::runtime_error("prefab not found");
		return *prefab;
	}

	// Retrieve a prefab by index
	//	Non-stable on container modifications
	//	Throws if index is out of bounds
	const Prefab& RequirePrefabByIndex(uint32_t index) const {
		if (m_prefabs.size() <= index) {
			throw std::runtime_error("prefab index out of bounds");
		}
		return m_prefabs.values()[index];
	}
#endif

	uint32_t RequirePrefabIndexByHash(HASH_t hash) const {
#if VH_IS_ON(VH_STANDARD_PREFABS)
		auto&& find = m_prefabs.find(hash);
		if (find != m_prefabs.end()) {
			return find._Ptr - m_prefabs.values().data();
		}
		throw std::runtime_error("prefab by hash not found");
#else
		// perform binary search
		auto&& find = std::lower_bound(m_prefabs.begin(), m_prefabs.end(), hash);
		if (find != m_prefabs.end()) {
			return find - m_prefabs.begin();
		}
		throw std::runtime_error("prefab by hash not found");
#endif
	}

	HASH_t RequirePrefabHashByIndex(uint32_t index) const {
#if VH_IS_ON(VH_STANDARD_PREFABS)
#error "Not implemented"
#else
		if (index < m_prefabs.size())
			return m_prefabs[index];
#endif		
		throw std::runtime_error("index exceeds prefabs");
	}

#if VH_IS_ON(VH_STANDARD_PREFABS)
	std::pair<const Prefab&, uint32_t> RequirePrefabAndIndexByHash(HASH_t hash) const {
		auto&& find = m_prefabs.find(hash);
		if (find != m_prefabs.end()) {
			return std::make_pair(*find, (uint32_t)(find._Ptr - m_prefabs.values().data()));
		}
		throw std::runtime_error("prefab by hash not found");
	}

	// Get a definite prefab
	//	Throws if prefab not found
	const Prefab& RequirePrefabByName(std::string_view name) const {
		return RequirePrefabByHash(VUtils::String::GetStableHashCode(name));
	}
#endif

#if VH_IS_ON(VH_STANDARD_PREFABS)
	void Register(std::string_view name, Prefab::Type type, Vector3f scale, Prefab::Flag flags, bool overwrite) {
		HASH_t hash = VUtils::String::GetStableHashCode(name);
		auto&& insert = m_prefabs.insert(Prefab(name, type, scale, flags)); // insert blank prefab

		if (!overwrite && !insert.second)
			throw std::runtime_error("prefab already registered");

		//VLOG(1) << "'" << prefab.m_name << "', '" << prefab.m_hash << "'";
	}
#endif

	/*
	void Register(DataReader& reader, bool overwrite) {
		auto name = reader.Read<std::string_view>();
		//auto hash = VUtils::String::GetStableHashCode(name);
		auto type = (Prefab::Type)reader.Read<int32_t>();
		auto localScale = reader.Read<Vector3f>();
		auto flags = reader.Read<Prefab::Flag>();

#if VH_IS_ON(VH_STANDARD_PREFABS)
		auto&& insert = m_prefabs.insert(Prefab(name, type, localScale, flags));
#else
		auto&& insert = m_prefabs.insert(m_prefabs.end(), VUtils::String::GetStableHashCode(name));
#endif

		//if (!overwrite && !insert.second)
			//throw std::runtime_error("prefab already registered");
		
		auto&& prefab = *insert.first->second;

		prefab.m_name = std::move(name);
		prefab.m_hash = hash;
		prefab.m_type = (Prefab::Type) reader.Read<int32_t>();
		prefab.m_localScale = reader.Read<Vector3f>();
		prefab.m_flags = reader.Read<Prefab::Flag>();
		
		//VLOG(1) << "'" << prefab.m_name << "', '" << prefab.m_hash << "'";
	}
	*/
};

// Manager class for everything related to ZDO-belonging Prefabs and their base data
IPrefabManager* PrefabManager();
