#pragma once

#include "VUtils.h"

#if VH_IS_ON(VH_DUNGEON_GENERATION)

#include "VUtilsMath.h"

#include "Vector.h"
#include "Quaternion.h"

class RoomConnection {
public:
	std::string m_type;

	bool m_entrance = false;
	bool m_allowDoor = true;
	bool m_doorOnlyIfOtherAlsoAllowsDoor = false;

	Vector3f m_localPos;
	Quaternion m_localRot;
};

class RoomConnectionInstance {
public:
	std::reference_wrapper<const RoomConnection> m_connection;
	Vector3f m_pos;
	Quaternion m_rot;
	int m_placeOrder;

	RoomConnectionInstance(const RoomConnection& connection, Vector3f pos, Quaternion rot, int placeOrder) 
		: m_connection(connection), 
			m_pos(pos), 
			m_rot(rot), 
			m_placeOrder(placeOrder) {}

	//RoomConnectionInstance(const RoomConnectionInstance& other) = delete;

	// Returns whether 2 RoomConnections are touching
	bool TestContact(const RoomConnectionInstance& other) const {
		return m_pos.SqDistance(other.m_pos) < .1f * .1f;
	}
};
#endif