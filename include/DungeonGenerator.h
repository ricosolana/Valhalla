#pragma once

#include <vector>
#include "VUtils.h"
#include "Vector.h"
#include "Quaternion.h"
#include "PrefabManager.h"
#include "VUtilsRandom.h"

// TODO give more verbose direct name
struct RoomData {

};

class DungeonGenerator {

public:
	//class Room {
	//public:
	//
	//};
	//
	//class Data {
	//
	//};

	//[Serializable]
	struct DoorDef {
		//GameObject m_prefab;
		const Prefab* m_prefab = nullptr;

		std::string m_connectionType = "";

		//[global::Tooltip("Will use default door chance set in DungeonGenerator if set to zero to default to old behaviour")]
		//[Range(0f, 1f)]
		float m_chance = 0;
	};

	enum class Algorithm {
		Dungeon,
		CampGrid,
		CampRadial
	};

public:
	void Clear();

	void Generate();

	int GetSeed();

	void Generate(int seed);

private:
	void GenerateRooms(VUtils::Random::State& state);

	void GenerateDungeon(VUtils::Random::State& state);

	void GenerateCampGrid(VUtils::Random::State& state);

	void GenerateCampRadial(VUtils::Random::State& state);

	Quaternion GetCampRoomRotation(VUtils::Random::State& state, RoomData *room, Vector3 pos);

	void PlaceWall(VUtils::Random::State& state, float radius, int sections);

	void Save();

	void SetupAvailableRooms();

	DoorDef FindDoorType(VUtils::Random::State& state, std::string type);

	void PlaceDoors(VUtils::Random::State& state);

	void PlaceEndCaps();

	void FindDividers(VUtils::Random::State& state, std::vector<RoomData*> &rooms);

	void FindEndCaps(VUtils::Random::State& state, RoomConnection connection, std::vector<RoomData*> &rooms);

	RoomData* FindEndCap(VUtils::Random::State& state, RoomConnection connection);

	void PlaceRooms();

	void PlaceStartRoom();

	bool PlaceOneRoom();

	void CalculateRoomPosRot(RoomConnection roomCon, Vector3 exitPos, Quaternion exitRot, Vector3 &outPos, Quaternion &outRot);

	bool PlaceRoom(RoomConnection connection, RoomData* roomData);

	void PlaceRoom(RoomData* room, Vector3 pos, Quaternion rot, RoomConnection fromConnection);

	void AddOpenConnections(Room newRoom, RoomConnection skipConnection);

	void SetupColliders();

	bool IsInsideDungeon(Room room, Vector3 pos, Quaternion rot);

	bool TestCollision(Room room, Vector3 pos, Quaternion rot);

	RoomData* GetRandomWeightedRoom(VUtils::Random::State& state, bool perimeterRoom);

	RoomData* GetRandomWeightedRoom(RoomConnection connection);

	RoomData* GetWeightedRoom(VUtils::Random::State& state, std::vector<RoomData*> rooms);

	RoomData* GetRandomRoom(VUtils::Random::State& state, RoomConnection connection);

	RoomConnection GetOpenConnection(VUtils::Random::State& state);

	RoomData* FindStartRoom(VUtils::Random::State& state);

	bool CheckRequiredRooms();

private:
	bool m_hasGeneratedSeed;

	static std::vector<Room> m_placedRooms;

	static std::vector<RoomConnection> m_openConnections;

	static std::vector<RoomConnection> m_doorConnections;

	static std::vector<RoomData*> m_availableRooms;

	static std::vector<RoomData*> m_tempRooms;

	BoxCollider m_colliderA;

	BoxCollider m_colliderB;

	//private ZNetView m_nview;

public:
	Algorithm m_algorithm;

	int m_maxRooms = 3;

	int m_minRooms = 20;

	int m_minRequiredRooms;

	std::vector<std::string> m_requiredRooms;

	bool m_alternativeFunctionality;

	Room.Theme m_themes = Room.Theme.Crypt;

	std::vector<DoorDef> m_doorTypes;

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

	Vector3 m_pos; // custom

	Vector3 m_zoneCenter;

	bool m_useCustomInteriorTransform;

	int m_generatedSeed;

	int m_generatedHash;

	Vector3 m_originalPosition;

	ZDO* m_zdo;
};
