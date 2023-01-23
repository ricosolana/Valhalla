#pragma once

#include "Vector.h"
#include "Quaternion.h"
#include "ZDO.h"

struct PrefabZDO {
	const HASH_t m_hash;

	const Vector3 m_pos;
	const Quaternion m_rot;
	const Vector3 m_localScale; // used by m_syncInitialScale; if scale is not {1, 1, 1}, then assume m_syncInitialScale is true

	// TODO use bitmask
	const bool m_persistent;
	const ZDO::ObjectType m_type;
	const bool m_distant;
};
