#pragma once

#include "VUtils.h"
#include "VUtilsString.h"
#include "Prefab.h"
#include "DataReader.h"

// TODO consider moving Instantiate(...) to ZDOManager
//	this class doesnt do much besides try to simulate Unity in appearance
//		which is not the desired result...
class IPrefabManager {

private:
	UNORDERED_MAP_t<HASH_t, std::unique_ptr<Prefab>> m_prefabs;

public:
	void Init();

	const Prefab* GetPrefab(HASH_t hash);

	// Get a prefab by name
	//	Returns the prefab or null
	const Prefab* GetPrefab(const std::string& name) {
		return GetPrefab(VUtils::String::GetStableHashCode(name));
	}

	// Get a definite prefab
	//	Throws if prefab not found
	const Prefab& RequirePrefab(HASH_t hash) {
		auto prefab = GetPrefab(hash);
		if (!prefab)
			throw std::runtime_error("prefab not found");
		return *prefab;
	}

	// Get a definite prefab
	//	Throws if prefab not found
	const Prefab& RequirePrefab(const std::string& name) {
		return RequirePrefab(VUtils::String::GetStableHashCode(name));
	}

	void Register(const std::string& name, Prefab::Type type, const Vector3f &scale, Prefab::Flag flags, bool overwrite) {
		HASH_t hash = VUtils::String::GetStableHashCode(name);
		auto&& insert = m_prefabs.insert({ hash, std::make_unique<Prefab>() });

		if (!overwrite && !insert.second)
			throw std::runtime_error("prefab already registered");

		auto&& prefab = *insert.first->second;
		
		prefab.m_name = name;
		prefab.m_hash = hash;
		prefab.m_type = type;
		prefab.m_localScale = scale;
		prefab.m_flags = flags;

		VLOG(1) << "'" << prefab.m_name << "', '" << prefab.m_hash << "'";
	}

	void Register(DataReader& reader, bool overwrite) {
		auto name = reader.Read<std::string>();
		auto hash = VUtils::String::GetStableHashCode(name);

		auto&& insert = m_prefabs.insert({ hash, std::make_unique<Prefab>() });

		if (!overwrite && !insert.second)
			throw std::runtime_error("prefab already registered");

		auto&& prefab = *insert.first->second;

		prefab.m_name = name;
		prefab.m_hash = hash;
		prefab.m_type = (Prefab::Type) reader.Read<int32_t>();
		prefab.m_localScale = reader.Read<Vector3f>();
		prefab.m_flags = reader.Read<Prefab::Flag>();

		VLOG(1) << "'" << prefab.m_name << "', '" << prefab.m_hash << "'";
	}

};

// Manager class for everything related to ZDO-belonging Prefabs and their base data
IPrefabManager* PrefabManager();
