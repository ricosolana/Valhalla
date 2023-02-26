#pragma once

#include <vector>
#include <robin_hood.h>

#include "VUtils.h"
#include "Vector.h"
#include "Quaternion.h"
#include "PrefabManager.h"
#include "VUtilsRandom.h"
#include "Room.h"
#include "RoomConnection.h"

// TODO give more verbose direct name
//struct RoomData {
//	//std::reference_wrapper<Room> m_room;
//	Room& m_room;
//};

class Dungeon {
public:
	enum class Algorithm {
		Dungeon,
		CampGrid,
		CampRadial
	};

	struct DoorDef {
		//GameObject m_prefab;
		const Prefab* m_prefab = nullptr;

		std::string m_connectionType = "";

		//[global::Tooltip("Will use default door chance set in DungeonGenerator if set to zero to default to old behaviour")]
		//[Range(0f, 1f)]
		float m_chance = 0;
	};

	Algorithm m_algorithm;

	int m_maxRooms = 3;

	int m_minRooms = 20;

	int m_minRequiredRooms;

	robin_hood::unordered_set<std::string> m_requiredRooms;

	bool m_alternativeFunctionality;

	Room::Theme m_themes = Room::Theme::Crypt;

	// Order is significant (polled with Seeded Random)
	std::vector<std::unique_ptr<DoorDef>> m_doorTypes; // Serialized

	float m_doorChance = 0.5f;

	float m_maxTilt = 10;

	float m_tileWidth = 8;

	static constexpr int m_gridSize = 4;

	float m_spawnChance = 1;

	float m_campRadiusMin = 15;

	float m_campRadiusMax = 30;

	float m_minAltitude = 1;

	int m_perimeterSections;

	float m_perimeterBuffer = 2;

	bool m_useCustomInteriorTransform;

	Vector3 m_originalPosition;

	// Order is significant (polled with Seeded Random)
	std::vector<Room*> m_availableRooms;
};


class DungeonGenerator {
public:
	void Generate();
	HASH_t GetSeed();

private:
	void GenerateRooms(VUtils::Random::State& state);

	void GenerateDungeon(VUtils::Random::State& state);

	void GenerateCampGrid(VUtils::Random::State& state);

	void GenerateCampRadial(VUtils::Random::State& state);

	Quaternion GetCampRoomRotation(VUtils::Random::State& state, Room &room, Vector3 pos);

	void PlaceWall(VUtils::Random::State& state, float radius, int sections);

	void Save();

	// Nullable
	Dungeon::DoorDef* FindDoorType(VUtils::Random::State& state, std::string type);

	void PlaceDoors(VUtils::Random::State& state);

	void PlaceEndCaps(VUtils::Random::State& state);

	void FindDividers(VUtils::Random::State& state, std::vector<Room*> &rooms);

	void FindEndCaps(VUtils::Random::State& state, RoomConnection &connection, std::vector<Room*> &rooms);

	Room* FindEndCap(VUtils::Random::State& state, RoomConnection &connection);

	void PlaceRooms(VUtils::Random::State& state);

	void PlaceStartRoom(VUtils::Random::State& state);

	bool PlaceOneRoom(VUtils::Random::State& state);

	void CalculateRoomPosRot(RoomConnection &roomCon, Vector3 exitPos, Quaternion exitRot, Vector3 &outPos, Quaternion &outRot);

	bool PlaceRoom(VUtils::Random::State& state, RoomConnectionInstance &connection, Room& roomData);

	void PlaceRoom(Room& room, Vector3 pos, Quaternion rot, RoomConnectionInstance *fromConnection);

	void AddOpenConnections(RoomInstance &newRoom, RoomConnectionInstance *skipConnection);

	bool IsInsideDungeon(Room &room, Vector3 pos, Quaternion rot);

	bool TestCollision(Room &room, Vector3 pos, Quaternion rot);

	Room* GetRandomWeightedRoom(VUtils::Random::State& state, bool perimeterRoom);

	Room* GetRandomWeightedRoom(VUtils::Random::State& state, RoomConnectionInstance *connection);

	Room& GetWeightedRoom(VUtils::Random::State& state, std::vector<Room*> &rooms);

	Room* GetRandomRoom(VUtils::Random::State& state, RoomConnectionInstance *connection);

	RoomConnectionInstance*GetOpenConnection(VUtils::Random::State& state);

	Room& FindStartRoom(VUtils::Random::State& state);

	bool CheckRequiredRooms();

private:
	bool m_hasGeneratedSeed;

	// Instanced
	std::vector<RoomInstance> m_placedRooms;
	// Instanced
	std::vector<RoomConnectionInstance> m_openConnections;
	// Instanced
	std::vector<RoomConnectionInstance> m_doorConnections;

	// Templated per type of generator
	//std::vector<RoomData*> m_availableRooms;

	std::vector<Room*> m_tempRooms;

	//BoxCollider m_colliderA;

	//BoxCollider m_colliderB;

	//private ZNetView m_nview;

public:
	Dungeon* m_dungeon;

	Vector3 m_pos; // instanced position
	Quaternion m_rot; // instanced rotation

	Vector3 m_zoneCenter;

	// TODO make Constexpr
	Vector3 m_zoneSize = Vector3(64, 64, 64);

	//bool m_useCustomInteriorTransform; // templated

	HASH_t m_generatedSeed;

	//Vector3 m_originalPosition; // templated

	ZDO* m_zdo;

public:
	DungeonGenerator(Vector3 pos, Quaternion rot);

};
