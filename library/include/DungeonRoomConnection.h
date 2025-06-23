#pragma once

#include "VUtils.h"

#if AVL_IS_ON(AVL_DUNGEON_GENERATION)

    #include "VUtilsMath.h"

    #include "Quaternion.h"
    #include "Vector.h"

class RoomConnection
{
  public:
    std::string m_type;

    bool m_entrance                      = false;
    bool m_allowDoor                     = true;
    bool m_doorOnlyIfOtherAlsoAllowsDoor = false;

    Vector3f m_localPos;
    Quaternion m_localRot;
};

class RoomConnectionInstance
{
  public:
    std::reference_wrapper<RoomConnection const> m_connection;
    Vector3f m_pos;
    Quaternion m_rot;
    int m_placeOrder;

    RoomConnectionInstance(RoomConnection const &connection, Vector3f pos, Quaternion rot, int placeOrder) :
        m_connection(connection),
        m_pos(pos),
        m_rot(rot),
        m_placeOrder(placeOrder)
    {
    }

    //RoomConnectionInstance(const RoomConnectionInstance& other) = delete;

    // Returns whether 2 RoomConnections are touching
    bool TestContact(RoomConnectionInstance const &other) const
    {
        return m_pos.sq_distance_to(other.m_pos) < .1f * .1f;
    }
};
#endif