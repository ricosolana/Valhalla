#include "Room.h"
#include "VUtilsString.h"

HASH_t Room::GetHash() {
	return VUtils::String::GetStableHashCode(m_name);
}

RoomConnection& Room::GetConnection(VUtils::Random::State& state, RoomConnection &other) {
	std::vector<RoomConnection*> tempConnections;
	for (auto&& roomConnection : m_roomConnections) {
		if (roomConnection->m_type == other.m_type)
		{
			tempConnections.push_back(roomConnection.get());
		}
	}

	if (tempConnections.empty())
		throw std::runtime_error("missing guaranteed room");

	return *tempConnections[state.Range(0, tempConnections.size())];
}

RoomConnection &Room::GetEntrance() {
	LOG(INFO) <<  "Connections " << m_roomConnections.size();
	for (auto&& roomConnection : m_roomConnections) {
		if (roomConnection->m_entrance)
			return *roomConnection.get();
	}

	throw std::runtime_error("unexpected branch");
}

bool Room::HaveConnection(RoomConnection &other) {
	for (auto&& connection : m_roomConnections) {
		if (connection->m_type == other.m_type)
			return true;
	}

	return false;
}
