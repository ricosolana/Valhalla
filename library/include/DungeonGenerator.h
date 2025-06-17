#pragma once

#include "VUtils.h"

#if VH_IS_ON(VH_DUNGEON_GENERATION)
    #include <vector>

    #include "Dungeon.h"
    #include "PrefabManager.h"
    #include "Quaternion.h"
    #include "Vector.h"
    #include "VUtilsRandom.h"
    #include "ZDO.h"
    #include "ZoneManager.h"

// TODO give more verbose direct name
//struct RoomData {
//	//std::reference_wrapper<Room> m_room;
//	Room& m_room;
//};

class Dungeon;

class DungeonGenerator
{
  private:
    // Instanced
    std::vector<std::unique_ptr<RoomInstance>> m_placed_rooms;
    // Instanced
    std::list<std::reference_wrapper<RoomConnectionInstance const>> m_open_connections;
    // Instanced
    std::vector<std::reference_wrapper<RoomConnectionInstance const>> m_door_connections;

  public:
    // TODO use reference
    Dungeon const &m_dungeon;

    Vector3f m_pos;  // instanced position
    Quaternion m_rot;// instanced rotation

    Vector3f m_zone_center;

    // TODO make Constexpr
    Vector3f const m_zone_size = Vector3f(64, 64, 64);

    //bool m_useCustomInteriorTransform; // templated

    //avledet::util::Hash m_generatedSeed;

    //Vector3f m_originalPosition; // templated

    //steady_clock::time_point m_generatedTime;

    ZDO::reference m_zdo;

  private:
    void GenerateRooms(VUtils::Random::State &state);

    void GenerateDungeon(VUtils::Random::State &state);

    void GenerateCampGrid(VUtils::Random::State &state);

    void GenerateCampRadial(VUtils::Random::State &state);

    Quaternion GetCampRoomRotation(VUtils::Random::State &state, Room const &room, Vector3f pos);

    void PlaceWall(VUtils::Random::State &state, float radius, int sections);

    void Save();

    // Nullable
    Dungeon::DoorDef const *FindDoorType(VUtils::Random::State &state, std::string_view type);

    void PlaceDoors(VUtils::Random::State &state);

    void PlaceEndCaps(VUtils::Random::State &state);

    std::vector<std::reference_wrapper<Room const>> FindDividers(VUtils::Random::State &state);

    std::vector<std::reference_wrapper<Room const>> FindEndCaps(VUtils::Random::State &state,
                                                                RoomConnection const &connection);

    void PlaceRooms(VUtils::Random::State &state);

    void PlaceStartRoom(VUtils::Random::State &state);

    bool PlaceOneRoom(VUtils::Random::State &state);

    void CalculateRoomPosRot(RoomConnection const &roomCon, Vector3f pos, Quaternion rot, Vector3f &outPos,
                             Quaternion &outRot);

    bool PlaceRoom(VUtils::Random::State &state, decltype(m_open_connections)::iterator &itr,
                   Room const &roomData, bool *outErased);

    // Camps/grid meadows
    void PlaceRoom(Room const &room, Vector3f pos, Quaternion rot);

    // Dungeon placement
    void PlaceRoom(Room const &room, Vector3f pos, Quaternion rot,
                   RoomConnectionInstance const &fromConnection);

    void AddOpenConnections(RoomInstance &newRoom, RoomConnectionInstance const &skipConnection);

    bool IsInsideZone(Room const &room, Vector3f pos, Quaternion rot);

    bool TestCollision(Room const &room, Vector3f pos, Quaternion rot);

    // Nullable
    Room const *GetRandomWeightedRoom(VUtils::Random::State &state, bool perimeterRoom);

    // Nullable
    Room const *GetRandomWeightedRoom(VUtils::Random::State &state, RoomConnectionInstance const *connection);

    Room const &GetWeightedRoom(VUtils::Random::State &state,
                                std::vector<std::reference_wrapper<Room const>> const &rooms);

    Room const *GetRandomRoom(VUtils::Random::State &state, RoomConnectionInstance const *connection);

    // Nullable
    decltype(m_open_connections)::iterator GetOpenConnection(VUtils::Random::State &state);

    Room const &FindStartRoom(VUtils::Random::State &state);

    bool CheckRequiredRooms();

  public:
    DungeonGenerator(Dungeon const &dungeon, ZDO::reference zdo);

    DungeonGenerator(DungeonGenerator const &other) = delete;

    void Generate();
    void Generate(avledet::util::Hash seed);

    avledet::util::Hash GetSeed();

    // i hate the split between zoneloc inst and dungeon
    // it should have dungeon type immediately within it...
    //	reduce indirection where possible to avoid continuous retrieval and annoyances
    //static void Regenerate(const ZoneID& zone);

    //static void Regenerate(const ZDO& zdo);

    //static void RegenerateDungeons();
};
#endif