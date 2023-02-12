#include "Room.h"

int Room::GetHash()
{
	return Utils.GetPrefabName(base.gameObject).GetStableHashCode();
}

void Room::OnEnable()
{
	this.m_roomConnections = null;
}

RoomConnection[] Room::GetConnections()
{
	if (this.m_roomConnections == null)
	{
		this.m_roomConnections = base.GetComponentsInChildren<RoomConnection>(false);
	}
	return this.m_roomConnections;
}

RoomConnection Room::GetConnection(RoomConnection other)
{
	RoomConnection[] connections = this.GetConnections();
	Room.tempConnections.Clear();
	foreach(RoomConnection roomConnection in connections)
	{
		if (roomConnection.m_type == other.m_type)
		{
			Room.tempConnections.Add(roomConnection);
		}
	}
	if (Room.tempConnections.Count == 0)
	{
		return null;
	}
	return Room.tempConnections[UnityEngine.Random.Range(0, Room.tempConnections.Count)];
}

RoomConnection Room::GetEntrance()
{
	RoomConnection[] connections = this.GetConnections();
	ZLog.Log("Connections " + connections.Length.ToString());
	foreach(RoomConnection roomConnection in connections)
	{
		if (roomConnection.m_entrance)
		{
			return roomConnection;
		}
	}
	return null;
}

bool Room::HaveConnection(RoomConnection other)
{
	RoomConnection[] connections = this.GetConnections();
	for (int i = 0; i < connections.Length; i++)
	{
		if (connections[i].m_type == other.m_type)
		{
			return true;
		}
	}
	return false;
}
