#pragma once

#include "VUtils.h"

#if VH_IS_ON(VH_DUNGEON_GENERATION)
#include <vector>

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
	void generate_rooms(VUtils::Random::State& state);

	void generate_dungeon(VUtils::Random::State& state);

	void generate_camp_grid(VUtils::Random::State& state);

	void generate_camp_radial(VUtils::Random::State& state);

	Quaternion get_camp_room_rotation(VUtils::Random::State& state, const Room &room, Vector3f pos);

	void place_wall(VUtils::Random::State& state, float radius, int sections);

	void save();

	// Nullable
	const Dungeon::DoorDef* get_door_type(VUtils::Random::State& state, std::string_view type);

	void place_doors(VUtils::Random::State& state);

	void place_end_caps(VUtils::Random::State& state);

	std::vector<std::reference_wrapper<const Room>> get_dividers(VUtils::Random::State& state);

	std::vector<std::reference_wrapper<const Room>> get_end_caps(VUtils::Random::State& state, const RoomConnection &connection);

	void place_rooms(VUtils::Random::State& state);

	void place_start_room(VUtils::Random::State& state);

	bool place_one_room(VUtils::Random::State& state);

	void calc_room_transform(const RoomConnection &roomCon, Vector3f pos, Quaternion rot, Vector3f &outPos, Quaternion &outRot);

	bool place_room(VUtils::Random::State& state, decltype(m_openConnections)::iterator &itr, const Room& roomData, bool* outErased);

	// Camps/grid meadows
	void place_room(const Room& room, Vector3f pos, Quaternion rot);

	// Dungeon placement
	void place_room(const Room& room, Vector3f pos, Quaternion rot, const RoomConnectionInstance& fromConnection);

	void add_open_connections(RoomInstance &newRoom, const RoomConnectionInstance &skipConnection);

	bool is_room_inside_zone(const Room &room, Vector3f pos, Quaternion rot);

	bool does_room_collide(const Room& room, Vector3f pos, Quaternion rot);

	// Nullable
	const Room* get_random_weighted_room(VUtils::Random::State& state, bool perimeterRoom);

	// Nullable
	const Room* get_random_weighted_room(VUtils::Random::State& state, const RoomConnectionInstance *connection);

    // TODO use reference_wrapper
	const Room& get_weighted_room(VUtils::Random::State& state, const std::vector<std::reference_wrapper<const Room>> &rooms);

    // Nullable
	const Room* get_random_room(VUtils::Random::State& state, const RoomConnectionInstance *connection);

	// Nullable
	decltype(m_openConnections)::iterator get_open_connection(VUtils::Random::State& state);

	const Room& get_start_room(VUtils::Random::State& state);

	bool are_required_rooms_placed();

public:
	DungeonGenerator(const Dungeon& dungeon, ZDO& zdo);

	DungeonGenerator(const DungeonGenerator& other) = delete;

	void generate();
	void generate(HASH_t seed);

	HASH_t get_seed();

	// i hate the split between zoneloc inst and dungeon
	// it should have dungeon type immediately within it...
	//	reduce indirection where possible to avoid continuous retrieval and annoyances
	//static void regenerate(const ZoneID& zone);

	//static void regenerate(const ZDO& zdo);

	//static void RegenerateDungeons();

};
#endif