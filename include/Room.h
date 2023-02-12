#pragma once

class Room {
public:
	enum class Theme {
		Crypt = 1,
		SunkenCrypt,
		Cave = 4,
		ForestCrypt = 8,
		GoblinCamp = 16,
		MeadowsVillage = 32,
		MeadowsFarm = 64,
		DvergerTown = 128,
		DvergerBoss = 256
	};

private:
	private static List<RoomConnection> tempConnections = new List<RoomConnection>();

	private RoomConnection[] m_roomConnections;

public:
	Vector3Int m_size = new Vector3Int(8, 4, 8);

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

	//[NonSerialized]
	//public int m_placeOrder;

	//[NonSerialized]
	//public int m_seed;

	//public MusicVolume m_musicPrefab;

private:
	void OnEnable();

public:
	int GetHash();

	RoomConnection[] GetConnections();

	RoomConnection GetConnection(RoomConnection other);

	RoomConnection GetEntrance();

	bool HaveConnection(RoomConnection other);
};
