#pragma once

#include <robin_hood.h>

#include "VUtils.h"
#include "WorldGenerator.h"
#include "PrefabTemplate.h"

class ZDO;

class IObjectManager {

private:
	robin_hood::unordered_map<HASH_t, std::unique_ptr<PrefabZDO>> m_prefabs;

private:
	void Init();

public:
	ZDO* Instantiate(HASH_t hash, const Vector3& pos, const Quaternion& rot = Quaternion::IDENTITY);

};

IObjectManager* ObjectManager();
