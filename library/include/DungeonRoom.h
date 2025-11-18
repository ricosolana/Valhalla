#pragma once

#include "VUtils.h"

#if AVL_IS_ON(AVL_DUNGEON_GENERATION)

    #include "DungeonRoomConnection.h"

    #include "Prefab.h"
    #include "Quaternion.h"
    #include "RandomSpawn.h"
    #include "VUtilsPhysics.h"
    #include "VUtilsRandom.h"

class Room
{
    friend class IDungeonManager;

  public:
  private:
    //static std::vector<RoomConnection*> tempConnections;

    std::vector<std::unique_ptr<RoomConnection>> m_roomConnections;

  public:
    //Vector3Int m_size = new Vector3Int(8, 4, 8);
    Vector3f m_size = Vector3f(8, 4, 8);

    avledet::util::Theme m_theme = avledet::util::Theme::Crypt;

    bool m_entrance;

    bool m_endCap;

    bool m_divider;

    int m_endCapPrio;

    int m_minPlaceOrder;

    float m_weight = 1;

    bool m_faceCenter;

    bool m_perimeter;

    std::string m_name;        // custom (unity gameobject name of this Room)

    avledet::util::Hash m_hash;// based off name

    Vector3f m_pos;
    Quaternion m_rot;

    std::vector<Prefab::Instance> m_netViews;
    std::vector<avledet::gen::RandomSpawn> m_random_spawns;

    // TODO later...
    //public MusicVolume m_musicPrefab;

  public:
    Room() = default;

    Room(Room const &other) = delete;

    avledet::util::Hash GetHash() const;

    std::vector<std::unique_ptr<RoomConnection>> const &GetConnections() const
    {
        return m_roomConnections;
    }

    // Nullable
    RoomConnection const &GetConnection(VUtils::Random::State &state, RoomConnection const &other) const;

    RoomConnection const &GetEntrance() const;

    bool HaveConnection(RoomConnection const &other) const;
};

struct RoomInstance
{
    std::reference_wrapper<Room const> m_room;
    Vector3f m_pos;
    Quaternion m_rot;
    int m_placeOrder = 0;
    int m_seed       = 0;
    std::vector<std::unique_ptr<RoomConnectionInstance>> m_connections;

    RoomInstance(Room const &room, Vector3f pos, Quaternion rot, int placeOrder, int seed) :
        m_room(room),
        m_pos(pos),
        m_rot(rot),
        m_placeOrder(placeOrder),
        m_seed(seed)
    {
        for (auto &&conn : room.GetConnections()) {
            // Find the world position of the connection,
            //	given parent (Room) position and localPosition (Connection)
            //m_connections.emplace_back(conn, room.m_pos + conn.get()->m_localPos)

            // https://stackoverflow.com/questions/73652767/get-new-child-object-postion-based-on-parent-transform

            //Quaternion childWorldRot = rot * conn->m_localRot;
            //Vector3f pointOnRot = (childWorldRot * Vector3f::FORWARD).Normalized() * conn->m_localPos.magnitude();
            //
            //Vector3f childWorldPos = pointOnRot + room.m_pos;

            //m_connections.emplace_back(*conn.get(), childWorldPos, childWorldRot, placeOrder);

            auto global = VUtils::Physics::LocalToGlobal(conn->m_localPos, conn->m_localRot, pos, rot);

            m_connections.push_back(std::make_unique<RoomConnectionInstance>(*conn.get(), global.first,
                                                                             global.second, placeOrder));
        }
    }
};
#endif