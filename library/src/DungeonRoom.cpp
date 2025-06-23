#include "DungeonRoom.h"

#if AVL_IS_ON(AVL_DUNGEON_GENERATION)
    #include "VUtilsString.h"

//TODO why not return hash?
avledet::util::Hash Room::GetHash() const
{
    return m_hash;
}

RoomConnection const &Room::GetConnection(VUtils::Random::State &state, RoomConnection const &other) const
{
    std::vector<std::reference_wrapper<RoomConnection const>> tempConnections;
    for (auto &&roomConnection : m_roomConnections) {
        if (roomConnection->m_type == other.m_type) {
            tempConnections.push_back(*roomConnection.get());
        }
    }

    if (tempConnections.empty())
        throw std::runtime_error("missing guaranteed room");

    return tempConnections[state.range(0, tempConnections.size())];
}

RoomConnection const &Room::GetEntrance() const
{
    //LOG(INFO) <<  "Room connections: " << m_roomConnections.size();
    for (auto &&roomConnection : m_roomConnections) {
        if (roomConnection->m_entrance)
            return *roomConnection.get();
    }

    throw std::runtime_error("unexpected");
}

bool Room::HaveConnection(RoomConnection const &other) const
{
    for (auto &&connection : m_roomConnections) {
        if (connection->m_type == other.m_type)
            return true;
    }

    return false;
}
#endif