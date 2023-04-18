#include "DungeonRoom.h"
#include "VUtilsString.h"

HASH_t Room::GetHash() const {
	return VUtils::String::GetStableHashCode(m_name);
}

const RoomConnection& Room::GetConnection(VUtils::Random::State& state, const RoomConnection &other) const {
	std::vector<std::reference_wrapper<const RoomConnection>> tempConnections;
	for (auto&& roomConnection : m_roomConnections) {
		if (roomConnection->m_type == other.m_type)
		{
			tempConnections.push_back(*roomConnection.get());
		}
	}

	if (tempConnections.empty())
		throw std::runtime_error("missing guaranteed room");

	return tempConnections[state.Range(0, tempConnections.size())];
}

const RoomConnection &Room::GetEntrance() const {
	LOG(INFO) <<  "Room connections: " << m_roomConnections.size();
	for (auto&& roomConnection : m_roomConnections) {
		if (roomConnection->m_entrance)
			return *roomConnection.get();
	}

	throw std::runtime_error("unexpected");
}

bool Room::HaveConnection(const RoomConnection &other) const {
	for (auto&& connection : m_roomConnections) {
		if (connection->m_type == other.m_type)
			return true;
	}

	return false;
}
