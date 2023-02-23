#pragma once

#include "RoomConnection.h"

class Room {
public:
	enum class Theme {
		Crypt = 1,
		SunkenCrypt = 2,
		Cave = 4,
		ForestCrypt = 8,
		GoblinCamp = 16,
		MeadowsVillage = 32,
		MeadowsFarm = 64,
		DvergerTown = 128,
		DvergerBoss = 256
	};

private:
	static std::vector<RoomConnection*> tempConnections;

	std::vector<RoomConnection*> m_roomConnections;

public:
	//Vector3Int m_size = new Vector3Int(8, 4, 8);
	Vector3 m_size = Vector3(8, 4, 8);

	//[BitMask(typeof(Room.Theme))]
	Theme m_theme = Theme::Crypt;

	bool m_enabled = true;

	bool m_entrance;

	bool m_endCap;

	bool m_divider;

	int m_endCapPrio;

	int m_minPlaceOrder;

	float m_weight = 1;

	bool m_faceCenter;

	bool m_perimeter;

	std::string m_name; // custom (unity gameobject name of this Room)

	//[NonSerialized]
	//public int m_placeOrder;

	//[NonSerialized]
	//public int m_seed;

	//public MusicVolume m_musicPrefab;

private:
	void OnEnable();

public:
	int GetHash();

	std::vector<RoomConnection*>& GetConnections();

	RoomConnection *GetConnection(RoomConnection *other);

	RoomConnection *GetEntrance();

	bool HaveConnection(RoomConnection *other);
};

struct RoomInstance {
	Room* m_room = nullptr;
	Vector3 m_pos;
	int m_placeOrder = 0;
	int m_seed = 0;
};
