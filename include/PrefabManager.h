#pragma once

#include "VUtils.h"
//#include <gtl/btree.hpp>

#include "VUtilsString.h"
#include "HashUtils.h"

#include "Prefab.h"
#include "DataStream.h"

// TODO consider moving Instantiate(...) to ZDOManager
//	this class doesnt do much besides try to simulate Unity in appearance
//		which is not the desired result...
class IPrefabManager {
	friend class IDiscordManager;

private:
	// TODO use set and use hash within from prefab
	avledet::util::Set<Prefab, ankerl::unordered_dense::hash<Prefab>, std::equal_to<>> m_prefabs;

public:
	void Init();

	const Prefab* GetPrefab(avledet::util::Hash hash) const {
		auto&& find = m_prefabs.find(hash);
		if (find != m_prefabs.end())
			return &(*find);
		return nullptr;
	}

	// Get a prefab by name
	//	Returns the prefab or null
	const Prefab* GetPrefab(std::string_view name) const {
		return GetPrefab(avledet::util::get_stable_hash(name));
	}

	// Get a definite prefab
	//	Throws if prefab not found
	const Prefab& RequirePrefabByHash(avledet::util::Hash hash) const {
		auto prefab = GetPrefab(hash);
		if (!prefab)
			throw std::runtime_error("prefab not found");
		return *prefab;
	}

	// Get a definite prefab
	//	Throws if prefab not found
	const Prefab& RequirePrefabByName(std::string_view name) const {
		return RequirePrefabByHash(avledet::util::get_stable_hash(name));
	}

	void Register(std::string_view name, Vector3f scale, Prefab::Flag flags) {
		avledet::util::Hash hash = avledet::util::get_stable_hash(name);
		Prefab prefab(name, scale, flags);
		m_prefabs.emplace(prefab);

		if (name == "_TerrainCompiler") {
			assert(prefab.GetObjectType() == avledet::util::ObjectType::TERRAIN);
		}

		//VLOG(1) << "'" << prefab.m_name << "', '" << prefab.m_hash << "'";
	}

	void Register(DataReader& reader) {
		auto name = reader.read<std::string_view>();
		auto localScale = reader.read<Vector3f>();
		auto flags = reader.read<Prefab::Flag>();

		auto hash = avledet::util::get_stable_hash(name);
		Register(name, localScale, flags);
				
		//VLOG(1) << "'" << prefab.m_name << "', '" << prefab.m_hash << "'";
	}
};

// Manager class for everything related to ZDO-belonging Prefabs and their base data
IPrefabManager* PrefabManager();
