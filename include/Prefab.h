#pragma once

#include "Vector.h"
#include "Quaternion.h"
#include "ZDO.h"

struct Prefab {
	//const HASH_t m_hash;
	std::string m_name;


	// 7 6 5 4 3 2 1 0
	// 0 0 0 0 D P T T
	//uint8_t m_flags;

	bool m_distant;
	bool m_persistent;
	ZDO::ObjectType m_type;

	Vector3 m_localScale; // used by m_syncInitialScale; if scale is not {1, 1, 1}, then assume m_syncInitialScale is true
};
