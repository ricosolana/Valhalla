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
	friend class IDiscordManager;

private:
	// TODO use set and use hash within from prefab
	UNORDERED_MAP_t<HASH_t, std::unique_ptr<Prefab>> m_prefabs;

public:
	void Init();

	const Prefab* GetPrefab(HASH_t hash) const {
		auto&& find = m_prefabs.find(hash);
		if (find != m_prefabs.end())
			return find->second.get();
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

	// Get a definite prefab
	//	Throws if prefab not found
	const Prefab& RequirePrefabByName(std::string_view name) const {
		return RequirePrefabByHash(VUtils::String::GetStableHashCode(name));
	}

	void Register(std::string_view name, ObjectType type, Vector3f scale, Prefab::Flag flags) {
		HASH_t hash = VUtils::String::GetStableHashCode(name);
		m_prefabs[hash] = std::make_unique<Prefab>(name, type, scale, flags);

		//VLOG(1) << "'" << prefab.m_name << "', '" << prefab.m_hash << "'";
	}

	void Register(DataReader& reader) {
		auto name = reader.Read<std::string_view>();
		auto type = (ObjectType)reader.Read<int32_t>();
		auto localScale = reader.Read<Vector3f>();
		auto flags = reader.Read<Prefab::Flag>();

		auto hash = VUtils::String::GetStableHashCode(name);
		m_prefabs[hash] = std::make_unique<Prefab>(std::string(name), type, localScale, flags);
		
		/*
		auto&& prefab = *insert.first->second;

		prefab.m_name = std::move(name);
		prefab.m_hash = hash;
		prefab.m_type = (Prefab::Type) reader.Read<int32_t>();
		prefab.m_localScale = reader.Read<Vector3f>();
		prefab.m_flags = reader.Read<Prefab::Flag>();*/
		
		//VLOG(1) << "'" << prefab.m_name << "', '" << prefab.m_hash << "'";
	}
};

// Manager class for everything related to ZDO-belonging Prefabs and their base data
IPrefabManager* PrefabManager();
