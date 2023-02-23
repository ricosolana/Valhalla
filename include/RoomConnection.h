#pragma once

#include "VUtils.h"
#include "VUtilsMath.h"

class RoomConnection {
public:
	std::string m_type;

	bool m_entrance = false;
	bool m_allowDoor = true;
	bool m_doorOnlyIfOtherAlsoAllowsDoor = false;
};

class RoomConnectionInstance {
public:
	RoomConnection& m_connection;
	Vector3 m_pos;
	public int m_placeOrder;

	bool TestContact(const RoomConnectionInstance& other) {
		return m_pos.SqDistance(other.m_pos) < .1f * .1f;
	}
};
