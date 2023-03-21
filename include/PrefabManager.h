#pragma once

#include <robin_hood.h>

#include "VUtils.h"
#include "Prefab.h"
#include "ZDO.h"

// TODO consider moving Instantiate(...) to ZDOManager
//	this class doesnt do much besides try to simulate Unity in appearance
//		which is not the desired result...
class IPrefabManager {

private:
	robin_hood::unordered_map<HASH_t, std::unique_ptr<Prefab>> m_prefabs;

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

	//// Instantiate a new ZDO in world with prefab at position and rotation
	//ZDO* Instantiate(HASH_t hash, const Vector3& pos, const Quaternion& rot = Quaternion::IDENTITY, const Prefab** outPrefab = nullptr);
	//
	//// Instantiate a new ZDO in world with prefab at position and rotation
	//ZDO& Instantiate(const Prefab& prefab, const Vector3& pos, const Quaternion& rot = Quaternion::IDENTITY);
	//
	//// Instantiate a copy of a ZDO (everything will be cloned)
	//ZDO& Instantiate(const ZDO& zdo);

};

// Manager class for everything related to ZDO-belonging Prefabs and their base data
IPrefabManager* PrefabManager();
