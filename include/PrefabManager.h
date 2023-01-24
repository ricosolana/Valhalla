#pragma once

#include <robin_hood.h>

#include "VUtils.h"
#include "WorldGenerator.h"
#include "Prefab.h"

class ZDO;

class IPrefabManager {

private:
	robin_hood::unordered_map<HASH_t, std::unique_ptr<Prefab>> m_prefabs;

private:
	void Init();

public:
	// std::optional<std::pair<const Prefab*, ZDO*>>

	const Prefab* GetPrefab(HASH_t hash);

	const Prefab* GetPrefab(const std::string& name) {
		return GetPrefab(VUtils::String::GetStableHashCode(name));
	}

	ZDO* Instantiate(HASH_t hash, const Vector3& pos, const Quaternion& rot = Quaternion::IDENTITY, const Prefab** outPrefab = nullptr);
	ZDO* Instantiate(const Prefab* prefab, const Vector3& pos, const Quaternion& rot = Quaternion::IDENTITY);

};

IPrefabManager* PrefabManager();
