#pragma once

#include "VUtils.h"

#if VH_IS_ON(VH_DUNGEON_GENERATION)

#include "DungeonRoomConnection.h"

#include "VUtilsRandom.h"
#include "Quaternion.h"
#include "Prefab.h"
#include "VUtilsPhysics.h"

class Room {
	friend class IDungeonManager;

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
		DvergerBoss = 256,
        ForestCryptHildir = 512,
        CaveHildir = 1024,
        PlainsFortHildir = 2048
	};

private:
	//static std::vector<RoomConnection*> tempConnections;

	std::vector<std::unique_ptr<RoomConnection>> m_roomConnections;

public:
	//Vector3Int m_size = new Vector3Int(8, 4, 8);
	Vector3f m_size = Vector3f(8, 4, 8);

	Theme m_theme = Theme::Crypt;

	bool m_entrance;

	bool m_endCap;

	bool m_divider;

	int m_endCapPrio;

	int m_minPlaceOrder;

	float m_weight = 1;

	bool m_faceCenter;

	bool m_perimeter;

	std::string m_name; // custom (unity gameobject name of this Room)

	HASH_t m_hash; // based off name

	Vector3f m_pos;
	Quaternion m_rot;

	std::vector<Prefab::Instance> m_netViews;

	// TODO later...
	//std::vector<RandomSpawn>

	// TODO later...
	//public MusicVolume m_musicPrefab;

public:
	Room() = default;
	Room(const Room& other) = delete;

public:
	HASH_t get_hash() const;

	const std::vector<std::unique_ptr<RoomConnection>>& get_connections() const {
		return m_roomConnections;
	}

	// TODO ref
	const RoomConnection &get_connection(VUtils::Random::State& state, const RoomConnection &other) const;

    // TODO ref
	const RoomConnection &get_entrance() const;

	bool is_connection_present(const RoomConnection &other) const;
};

struct RoomInstance {
	std::reference_wrapper<const Room> m_room;
	Vector3f m_pos;
	Quaternion m_rot;
	int m_placeOrder = 0;
	int m_seed = 0;
	std::vector<std::unique_ptr<RoomConnectionInstance>> m_connections;

	RoomInstance(const Room& room, Vector3f pos, Quaternion rot, int placeOrder, int seed) 
		: m_room(room), m_pos(pos), m_rot(rot), m_placeOrder(placeOrder),  m_seed(seed) {
		for (auto&& conn : room.get_connections()) {
			// Find the world position of the connection, 
			//	given parent (Room) position and localPosition (Connection)
			//m_connections.emplace_back(conn, room.m_pos + conn.get()->m_localPos)

			// https://stackoverflow.com/questions/73652767/get-new-child-object-postion-based-on-parent-transform

			//Quaternion childWorldRot = rot * conn->m_localRot;
			//Vector3f pointOnRot = (childWorldRot * Vector3f::FORWARD).Normalized() * conn->m_localPos.Magnitude();
			//
			//Vector3f childWorldPos = pointOnRot + room.m_pos;
			
			//m_connections.emplace_back(*conn.get(), childWorldPos, childWorldRot, placeOrder);

			auto global = VUtils::Physics::LocalToGlobal(conn->m_localPos, conn->m_localRot,
				pos, rot);

			m_connections.push_back(std::make_unique<RoomConnectionInstance>(*conn.get(), global.first, global.second, placeOrder));
		}
	}
};
#endif