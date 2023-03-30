#pragma once

#include <vector>
#include <robin_hood.h>

#include "VUtils.h"
#include "Vector.h"
#include "Quaternion.h"
#include "PrefabManager.h"
#include "VUtilsRandom.h"
#include "ZoneManager.h"
#include "ZDO.h"
#include "Dungeon.h"

// TODO give more verbose direct name
//struct RoomData {
//	//std::reference_wrapper<Room> m_room;
//	Room& m_room;
//};

class Dungeon;

class DungeonGenerator {
private:
	// Instanced
	std::vector<std::unique_ptr<RoomInstance>> m_placedRooms;
	// Instanced
	std::list<std::reference_wrapper<const RoomConnectionInstance>> m_openConnections;
	// Instanced
	std::vector<std::reference_wrapper<const RoomConnectionInstance>> m_doorConnections;

public:
	// TODO use reference
	const Dungeon& m_dungeon;

	Vector3f m_pos; // instanced position
	Quaternion m_rot; // instanced rotation

	Vector3f m_zoneCenter;

	// TODO make Constexpr
	const Vector3f m_zoneSize = Vector3f(64, 64, 64);

	//bool m_useCustomInteriorTransform; // templated

	//HASH_t m_generatedSeed;

	//Vector3f m_originalPosition; // templated

	//steady_clock::time_point m_generatedTime;

	ZDO& m_zdo;

private:
	void GenerateRooms(VUtils::Random::State& state);

	void GenerateDungeon(VUtils::Random::State& state);

	void GenerateCampGrid(VUtils::Random::State& state);

	void GenerateCampRadial(VUtils::Random::State& state);

	Quaternion GetCampRoomRotation(VUtils::Random::State& state, const Room &room, const Vector3f &pos);

	void PlaceWall(VUtils::Random::State& state, float radius, int sections);

	void Save();

	// Nullable
	const Dungeon::DoorDef* FindDoorType(VUtils::Random::State& state, const std::string& type);

	void PlaceDoors(VUtils::Random::State& state);

	void PlaceEndCaps(VUtils::Random::State& state);

	std::vector<std::reference_wrapper<const Room>> FindDividers(VUtils::Random::State& state);

	std::vector<std::reference_wrapper<const Room>> FindEndCaps(VUtils::Random::State& state, const RoomConnection &connection);

	void PlaceRooms(VUtils::Random::State& state);

	void PlaceStartRoom(VUtils::Random::State& state);

	bool PlaceOneRoom(VUtils::Random::State& state);

	void CalculateRoomPosRot(const RoomConnection &roomCon, const Vector3f &pos, const Quaternion &rot, Vector3f &outPos, Quaternion &outRot);

	bool PlaceRoom(VUtils::Random::State& state, decltype(m_openConnections)::iterator &itr, const Room& roomData, bool* outErased);

	// Camps/grid meadows
	void PlaceRoom(const Room& room, Vector3f pos, Quaternion rot);

	// Dungeon placement
	void PlaceRoom(const Room& room, Vector3f pos, Quaternion rot, const RoomConnectionInstance& fromConnection);

	void AddOpenConnections(RoomInstance &newRoom, const RoomConnectionInstance &skipConnection);

	bool IsInsideZone(const Room &room, const Vector3f &pos, const Quaternion &rot);

	bool TestCollision(const Room& room, const Vector3f& pos, const Quaternion& rot);

	// Nullable
	const Room* GetRandomWeightedRoom(VUtils::Random::State& state, bool perimeterRoom);

	// Nullable
	const Room* GetRandomWeightedRoom(VUtils::Random::State& state, const RoomConnectionInstance *connection);

	const Room& GetWeightedRoom(VUtils::Random::State& state, const std::vector<std::reference_wrapper<const Room>> &rooms);

	const Room* GetRandomRoom(VUtils::Random::State& state, const RoomConnectionInstance *connection);

	// Nullable
	decltype(m_openConnections)::iterator GetOpenConnection(VUtils::Random::State& state);

	const Room& FindStartRoom(VUtils::Random::State& state);

	bool CheckRequiredRooms();

public:
	DungeonGenerator(const Dungeon& dungeon, ZDO& zdo);

	DungeonGenerator(const DungeonGenerator& other) = delete;

	void Generate();
	void Generate(HASH_t seed);

	HASH_t GetSeed();

	// i hate the split between zoneloc inst and dungeon
	// it should have dungeon type immediately within it...
	//	reduce indirection where possible to avoid continuous retrieval and annoyances
	//static void Regenerate(const ZoneID& zone);

	//static void Regenerate(const ZDO& zdo);

	//static void RegenerateDungeons();

};
