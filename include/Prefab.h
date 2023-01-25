#pragma once

#include "Vector.h"
#include "Quaternion.h"
#include "ZDO.h"

//class IPrefabManager;

class Prefab {
	//friend class IPrefabManager;

//private:
	//Prefab() = default;

public:
	Prefab() = default;

	//const HASH_t m_hash;
	std::string m_name;
	HASH_t m_hash = 0; // precomputed from m_name

	// 7 6 5 4 3 2 1 0
	// 0 0 0 0 D P T T
	//uint8_t m_flags;

	bool m_distant = false;
	bool m_persistent = false;
	ZDO::ObjectType m_type = ZDO::ObjectType::Default;

	// if scale is not {1, 1, 1}, then assume m_syncInitialScale is true
	Vector3 m_localScale;
};
