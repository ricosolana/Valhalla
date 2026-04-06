#include "DungeonGenerator.h"
#include "Avledet.h"
#include "DataStream.h"
#include "Quaternion.h"
#include "Vector.h"
#include <cstddef>
#include <limits>
#include <quill/LogMacros.h>
#include <quill/Utility.h>

#if AVL_IS_ON(AVL_DUNGEON_GENERATION)
    #include "GeoManager.h"
    #include "VUtilsMath2.h"
    #include "VUtilsMathf.h"
    #include "VUtilsPhysics.h"
    #include "WorldManager.h"
    #include "ZDOManager.h"
    #include "ZoneManager.h"

DungeonGenerator::DungeonGenerator(Dungeon const &dungeon, ZDO::reference zdo) :
    m_rot(zdo->get_rotation()),
    m_pos(zdo->get_position()),
    m_dungeon(dungeon),
    m_zdo(zdo)
{

    auto zone = IZoneManager::WorldToZonePos(m_pos);
    auto seed = GeoManager()->GetSeed();
    //this->m_generatedSeed = seed + zone.x * 4271 + zone.y * -7187 + (int)m_pos.x * -4271 + (int)m_pos.y * 9187 + (int)m_pos.z * -2134;
    //this->m_generatedSeed = seed + (int)m_pos.x * -4271 + (int)m_pos.y * 9187 + (int)m_pos.z * -2134;

    this->m_zone_center   = IZoneManager::ZoneToWorldPos(zone);
    this->m_zone_center.y = m_pos.y;// -this->m_dungeon.m_originalPosition.y;
}

// TODO generate seed during start
avledet::util::Hash DungeonGenerator::GetSeed()
{
    if (AVL_SETTINGS.dungeonsSeeded) {
        auto seed = GeoManager()->GetSeed();
        //auto zone = IZoneManager::WorldToZonePos(m_pos);
        return seed + (int) m_pos.x * -4271 + (int) m_pos.y * 9187 + (int) m_pos.z * -2134;
    } else {
        return VUtils::Random::State().range(INT_MIN, INT_MAX);
    }
}

void DungeonGenerator::Generate()
{
    this->Generate(GetSeed());
}

void DungeonGenerator::DungeonGenerator::Generate(avledet::util::Hash seed)
{
    VUtils::Random::State state(seed);

    this->GenerateRooms(state);
    this->Save();

    //this->m_generatedTime = steady_clock::now();

    //TODO it appears that quill logger isnt compiling for Vector3f / cant resolve the stream<< Operator
    //LOG_INFO(AVL_LOGGER, "Finished generating dungeon: '{}', pos: {}, seed: {}, rooms: {}/{}", m_dungeon.m_prefab->m_name, m_pos, seed, m_placedRooms.size(), m_dungeon.m_maxRooms);
}

//
//void DungeonGenerator::Regenerate(const ZoneID& zone) {
//	// Find the dungeon in that zone
//	//ZDOManager()->AnyZDO(zone).
//	//zdo
//}

//void DungeonGenerator::Regenerate(const ZDO zdo) {
//	// Find the dungeon in that zone
//	//ZDOManager()->AnyZDO(zone).
//	if (!zdo.get_prefab()->FlagsPresent(Prefab::FLAG_t::Dungeon))
//		throw std::runtime_error("not a dungeon");
//
//
//}


void DungeonGenerator::GenerateRooms(VUtils::Random::State &state)
{
    switch (this->m_dungeon.m_algorithm) {
    case Dungeon::Algorithm::Dungeon: this->GenerateDungeon(state); break;
    case Dungeon::Algorithm::CampGrid: this->GenerateCampGrid(state); break;
    case Dungeon::Algorithm::CampRadial: this->GenerateCampRadial(state); break;
    }
}

void DungeonGenerator::GenerateDungeon(VUtils::Random::State &state)
{
    this->PlaceStartRoom(state);
    this->PlaceRooms(state);







    if (AVL_SETTINGS.dungeonsEndcapsEnabled)
        this->PlaceEndCaps(state);

    if (AVL_SETTINGS.dungeonsDoors)
        this->PlaceDoors(state);

    //LOG_INFO(AVL_LOGGER, "Desmos: {}", desmos_dbg_ss.str());



    auto&& snap = [](float v, float eps = 0.001)
    {
        float r = std::round(v);
        if (std::fabs(v - r) < eps)
            return r;
        return v;
    };

    //auto&& formatFloat1 = [&](float v, int precision = 5) {
    //    std::ostringstream ss;
    //    ss << std::fixed; // << std::setprecision(precision);
    //    ss << snap(v);
    //    return ss.str();
    //};

    auto&& formatFloat = [&](float v) {
        // "{:g}" automatically handles removing trailing zeros 
        // and choosing the shortest representation, but to be 100% 
        // safe from 'e', we can use "{:.3f}" and trim.
        std::string s = std::format("{:.3f}", snap(v));
        s.erase(s.find_last_not_of('0') + 1, std::string::npos);
        if (s.back() == '.') s.pop_back();
        return s;
    };

    std::ostringstream math3d_ss, desmos_ss, desmos3d_ss;
    math3d_ss << "["; // Start the master list
    //desmos_ss << "B = ["; // Start the master list
    desmos3d_ss << "B = ["; // Start the master list
    bool first_face = true;

    for (const auto& room : m_placed_rooms) { // Assuming a collection of box objects
        // 1. Calculate the 8 corners for THIS box
        const auto& size = room->m_room.get().m_size;
        Vector3f v[8] = {
            room->m_pos + room->m_rot * Vector3f(-size.x*0.5f, -size.y*0.5f, -size.z*0.5f),
            room->m_pos + room->m_rot * Vector3f( size.x*0.5f, -size.y*0.5f, -size.z*0.5f),
            room->m_pos + room->m_rot * Vector3f( size.x*0.5f,  size.y*0.5f, -size.z*0.5f),//br
            room->m_pos + room->m_rot * Vector3f(-size.x*0.5f,  size.y*0.5f, -size.z*0.5f),//bl
            room->m_pos + room->m_rot * Vector3f(-size.x*0.5f, -size.y*0.5f,  size.z*0.5f),
            room->m_pos + room->m_rot * Vector3f( size.x*0.5f, -size.y*0.5f,  size.z*0.5f),
            room->m_pos + room->m_rot * Vector3f( size.x*0.5f,  size.y*0.5f,  size.z*0.5f),//ur
            room->m_pos + room->m_rot * Vector3f(-size.x*0.5f,  size.y*0.5f,  size.z*0.5f) //ul
        };

        ////Vector3f v3_a_br = pos + rot * Vector3f(size.x*0.5f, pos.y, -size.z*0.5f);
        ////Vector3f v3_a_bl = pos + rot * Vector3f(-size.x*0.5f, pos.y, -size.z*0.5f);
        ////Vector3f v3_a_ur = pos + rot * Vector3f(size.x*0.5f, pos.y, size.z*0.5f);
        ////Vector3f v3_a_ul = pos + rot * Vector3f(-size.x*0.5f, pos.y, size.z*0.5f);
        //desmos_dbg_ss << "polygon((" << v3_a_br.x << "," << v3_a_br.z << "),(" << v3_a_ur.x << "," << v3_a_ur.z << "),("
        //    << v3_a_ul.x << "," << v3_a_ul.z << "),(" << v3_a_bl.x << "," << v3_a_bl.z << ")),";
        ///desmos_dbg_ss << "polygon((" 
        ///    << snap(v3_a_br.x) << "," << snap(v3_a_br.z) << "),("
        ///    << snap(v3_a_ur.x) << "," << snap(v3_a_ur.z) << "),("
        ///    << snap(v3_a_ul.x) << "," << snap(v3_a_ul.z) << "),("
        ///    << snap(v3_a_bl.x) << "," << snap(v3_a_bl.z) << ")),"; 
        desmos_ss << "polygon(("
            << formatFloat(v[2].x) << "," << formatFloat(v[2].z) << "),("
            << formatFloat(v[6].x) << "," << formatFloat(v[6].z) << "),("
            << formatFloat(v[7].x) << "," << formatFloat(v[7].z) << "),("
            << formatFloat(v[3].x) << "," << formatFloat(v[3].z) << ")),";

        // 2. Define the 6 faces (indices into v)
        //int faces[6][4] = {
        //    {0, 3, 2, 1}, {4, 5, 6, 7}, // Bottom, Top
        //    {0, 1, 5, 4}, {2, 3, 7, 6}, // Sides
        //    {0, 4, 7, 3}, {1, 2, 6, 5}
        //};

        //for (int i = 0; i < 6; ++i) {
        //    if (!first_face) math3d_ss << ",";
        //    
        //    math3d_ss << "["; // Start of ONE face (list of points)
        //    for (int j = 0; j < 4; ++j) {
        //        Vector3f p = v[faces[i][j]];
        //        math3d_ss << "[" << formatFloat(p.x) << "," << formatFloat(p.y) << "," << formatFloat(p.z) << "]";
        //        if (j < 3) math3d_ss << ",";
        //    }
        //    math3d_ss << "]"; // End of face
        //    first_face = false;
        //}

        //for (int i = 0; i < 6; ++i) {
        //    if (!first_face) desmos_ss << ",";
        //    
        //    desmos_ss << "polygon(";
        //    for (int j = 0; j < 4; ++j) {
        //        Vector3f p = v[faces[i][j]];
        //        // Note: Desmos 3D uses (x, y, z) parentheses
        //        desmos_ss << "(" << formatFloat(p.x) << "," << formatFloat(p.y) << "," << formatFloat(p.z) << ")";
        //        if (j < 3) desmos_ss << ",";
        //    }
        //    desmos_ss << ")";
        //    first_face = false;
        //}

        // 2. Define 12 triangles (2 per face)
        // Desmos triangle syntax: triangle((x,y,z), (x,y,z), (x,y,z))
        int tri_indices[12][3] = {
            {0,1,2}, {0,2,3}, // Bottom
            {4,5,6}, {4,6,7}, // Top
            {0,1,5}, {0,5,4}, // Front
            {2,3,7}, {2,7,6}, // Back
            {0,3,7}, {0,7,4}, // Left
            {1,2,6}, {1,6,5}  // Right
        };

        for (int i = 0; i < 12; ++i) {
            if (!first_face) desmos3d_ss << ",";
            
            desmos3d_ss << "triangle(";
            for (int j = 0; j < 3; ++j) {
                Vector3f p = v[tri_indices[i][j]];
                desmos3d_ss << "(" << formatFloat(p.x) << "," << formatFloat(p.y) << "," << formatFloat(p.z) << ")";
                if (j < 2) desmos3d_ss << ",";
            }
            desmos3d_ss << ")";
            first_face = false;
        }
    }

    math3d_ss << "]"; // End the master list
    desmos3d_ss << "]";

    //LOG_INFO(AVL_LOGGER, "https://math3d.org: {}", math3d_ss.str());
    LOG_INFO(AVL_LOGGER, "desmos: {}", desmos_ss.str());
    LOG_INFO(AVL_LOGGER, "desmos3d: {}", desmos3d_ss.str());


}

void DungeonGenerator::GenerateCampGrid(VUtils::Random::State &state)
{
    float num  = std::cos(0.017453292f * this->m_dungeon.m_max_tilt);
    Vector3f a = this->m_pos
                 + Vector3f((float) (-this->m_dungeon.m_grid_size) * this->m_dungeon.m_tile_width * 0.5f, 0.f,
                            (float) (-this->m_dungeon.m_grid_size) * this->m_dungeon.m_tile_width * 0.5f);

    for (int i = 0; i < this->m_dungeon.m_grid_size; i++) {
        for (int j = 0; j < this->m_dungeon.m_grid_size; j++) {
            if (state.value() <= this->m_dungeon.m_spawn_chance) {
                Vector3f pos = a
                               + Vector3f((float) j * this->m_dungeon.m_tile_width, 0.f,
                                          (float) i * this->m_dungeon.m_tile_width);
                auto randomWeightedRoom = this->GetRandomWeightedRoom(state, false);
                if (randomWeightedRoom) {
                    Vector3f vector;
                    avledet::util::Biome biome;
                    avledet::util::BiomeArea biomeArea;
                    ZoneManager()->GetGroundData(pos, vector, biome, biomeArea);
                    if (vector.y < num)
                        continue;

                    Quaternion rot = Quaternion::euler(0, 22.5f * state.range(0, 16), 0.f);
                    this->PlaceRoom(*randomWeightedRoom, pos, rot);
                }
            }
        }
    }
}

void DungeonGenerator::GenerateCampRadial(VUtils::Random::State &state)
{
    float num  = state.range(this->m_dungeon.m_camp_radius_min, this->m_dungeon.m_camp_radius_max);
    float num2 = std::cos(0.017453292f * this->m_dungeon.m_max_tilt);
    int num3   = state.range(this->m_dungeon.m_min_rooms, this->m_dungeon.m_max_rooms);
    int num4   = num3 * 20;
    int num5   = 0;
    for (int i = 0; i < num4; i++) {
        Vector3f vector = this->m_pos
                          + Quaternion::euler(0.f, state.range(0, 360), 0.f) * Vector3f::FORWARD
                                    * state.range(0.f, num - this->m_dungeon.m_perimeter_buffer);

        auto randomWeightedRoom = this->GetRandomWeightedRoom(state, false);
        if (randomWeightedRoom) {
            Vector3f vector2;
            avledet::util::Biome biome;
            avledet::util::BiomeArea biomeArea;
            ZoneManager()->GetGroundData(vector, vector2, biome, biomeArea);
            if (vector2.y < num2 || vector.y - IZoneManager::WATER_LEVEL < this->m_dungeon.m_min_altitude)
                continue;

            Quaternion campRoomRotation = this->GetCampRoomRotation(state, *randomWeightedRoom, vector);
            if (!this->TestCollision(*randomWeightedRoom, vector, campRoomRotation)) {
                this->PlaceRoom(*randomWeightedRoom, vector, campRoomRotation);
                num5++;
                if (num5 >= num3)
                    break;
            }
        }
    }

    if (this->m_dungeon.m_perimeter_sections > 0)
        this->PlaceWall(state, num, this->m_dungeon.m_perimeter_sections);
}

Quaternion DungeonGenerator::GetCampRoomRotation(VUtils::Random::State &state, Room const &room, Vector3f pos)
{
    if (room.m_faceCenter) {
        Vector3f vector = m_pos - pos;
        vector.y        = 0;
        if (vector == Vector3f::ZERO)
            vector = Vector3f::FORWARD;

        vector.normal();
        float y = VUtils::Mathf::Round(VUtils::Math::YawFromDirection(vector) / 22.5f) * 22.5f;
        return Quaternion::euler(0, y, 0);
    }

    return Quaternion::euler(0, 22.5f * state.range(0, 16), 0);
}

void DungeonGenerator::PlaceWall(VUtils::Random::State &state, float radius, int sections)
{
    float num = std::cos(0.017453292f * this->m_dungeon.m_max_tilt);
    int num2  = 0;
    int num3  = sections * 20;
    for (int i = 0; i < num3; i++) {
        auto &&randomWeightedRoom = this->GetRandomWeightedRoom(state, true);
        if (randomWeightedRoom) {
            Vector3f vector
                    = this->m_pos + Quaternion::euler(0, state.range(0, 360), 0) * Vector3f::FORWARD * radius;

            Quaternion campRoomRotation = this->GetCampRoomRotation(state, *randomWeightedRoom, vector);

            Vector3f vector2;
            avledet::util::Biome biome;
            avledet::util::BiomeArea biomeArea;
            ZoneManager()->GetGroundData(vector, vector2, biome, biomeArea);
            if (vector2.y < num || vector.y - IZoneManager::WATER_LEVEL < this->m_dungeon.m_min_altitude)
                continue;

            if (!this->TestCollision(*randomWeightedRoom, vector, campRoomRotation)) {
                this->PlaceRoom(*randomWeightedRoom, vector, campRoomRotation);
                num2++;
                if (num2 >= sections) {
                    break;
                }
            }
        }
    }
}

void TEST_Load(DungeonGenerator const& gen) {
    //ZLog.Log("Test loading dungeon");
    //ZDO zdo = this.m_nview.GetZDO();
    const auto& zdo = gen.m_zdo;
    
    auto&& bytes = zdo->find_bytes(avledet::util::hashes::ZDO::DungeonGenerator::ROOM_DATA);
    if (bytes) {
        DataReader binaryReader(*bytes);

        auto num2 = binaryReader.read<std::int32_t>();
        //this.m_loadedRooms = new DungeonGenerator.RoomPlacementData[num2];
        for (int i = 0; i < num2; i++)
        {
            int num3 = binaryReader.read<std::int32_t>();
            auto vector = binaryReader.read<Vector3f>();
            auto quaternion = Quaternion::euler(binaryReader.read<Vector3f>());

            (void) vector;
            (void) quaternion;
            
            //gen.m_dungeon.ro

            if (num3 == 0) {
                LOG_ERROR(AVL_LOGGER, "dungeon room hash not valid: {} (fix your fucking code)", num3);
            }
        }
        //ZLog.Log(string.Format("Dungeon loaded with {0} rooms in {1} ms.", num2, (DateTime.Now - now).TotalMilliseconds));
    }
    else {
        LOG_ERROR(AVL_LOGGER, "failed load dungeon test");
    }
}

void DungeonGenerator::Save()
{
    //bytes.reserve(sizeof(std::int32_t) +
    //m_placedRooms.size() * (sizeof(avledet::util::Hash) + sizeof(Vector3f) + sizeof(Quaternion)));
    DataWriter writer;

    writer.write((std::uint32_t) m_placed_rooms.size());
    for (const auto& instance : m_placed_rooms) {
        const auto &room     = instance->m_room.get();

        Vector3f pos   = instance->m_pos;
        Quaternion rot = instance->m_rot;

        if (m_dungeon.m_algorithm == Dungeon::Algorithm::Dungeon) {
            std::tie(pos, rot) = VUtils::Physics::LocalToGlobal(instance->m_pos, instance->m_rot, this->m_pos,
                                                                this->m_rot);
        }

        auto hash = room.GetHash();
        //LOG_INFO(AVL_LOGGER, "hash: {}", hash);

        writer.write(hash);
        writer.write(pos);
        writer.write(rot.euler_angles());
    }

    // write to console
    //writer.get_buf()
    //auto buf = writer.get_buf();
    //LOG_INFO(AVL_LOGGER, "roomData: {},", quill::utility::to_hex(buf.data(), buf.size()));

    m_zdo->set(avledet::util::hashes::ZDO::DungeonGenerator::ROOM_DATA, writer.release());

    //TEST_Load(*this);
}

Dungeon::DoorDef const *DungeonGenerator::FindDoorType(VUtils::Random::State &state, std::string_view type)
{
    std::vector<std::reference_wrapper<Dungeon::DoorDef const>> list;
    for (auto &&doorDef : this->m_dungeon.m_door_types) {
        if (doorDef.m_connection_type == type) {
            list.push_back(doorDef);
        }
    }

    // This case is possible with 'dvergropen' (Mistlands)
    if (list.empty())
        return nullptr;

    return &list[state.range(0, list.size())].get();
}

void DungeonGenerator::PlaceDoors(VUtils::Random::State &state)
{
    int num = 0;
    for (auto &&roomConnection : m_door_connections) {
        auto &&doorDef = this->FindDoorType(state, roomConnection.get().m_connection.get().m_type);
        if (!doorDef) {
            LOG_INFO(AVL_LOGGER, "No door type for connection: {}",
                     roomConnection.get().m_connection.get().m_type);
        } else if ((doorDef->m_chance <= 0 || state.value() <= doorDef->m_chance)
                   && (doorDef->m_chance > 0 || state.value() <= this->m_dungeon.m_door_chance)) {
            auto global = VUtils::Physics::LocalToGlobal(
                    roomConnection.get().m_pos, roomConnection.get().m_rot, this->m_pos, this->m_rot);

            auto &&zdo = ZDOManager()->Instantiate(doorDef->m_prefab, global.first);
            zdo->set_rotation(global.second);
            num++;
        }
    }

    LOG_INFO(AVL_LOGGER, "Placed {} doors", num);
}

void DungeonGenerator::PlaceEndCaps(VUtils::Random::State &state)
{
    //for (int i = 0; i < m_openConnections.size(); i++) {
    for (auto &&itr1 = m_open_connections.begin(); itr1 != m_open_connections.end();) {
        //auto&& roomConnection = m_openConnections[i];
        auto &&roomConnection                         = *itr1;
        RoomConnectionInstance const *roomConnection2 = nullptr;

        // Find the other matching connection
        //for (int j = 0; j < m_openConnections.size(); j++) {
        for (auto &&itr2 = m_open_connections.begin(); itr2 != m_open_connections.end(); ++itr2) {
            //if (j != i && roomConnection.get().TestContact(m_openConnections[j])) {
            if (itr2 != itr1 && roomConnection.get().TestContact(*itr2)) {
                roomConnection2 = &itr2->get();
                break;
            }
        }

        if (roomConnection2) {
            if (roomConnection.get().m_connection.get().m_type
                != roomConnection2->m_connection.get().m_type) {
                auto &&tempRooms = this->FindDividers(state);
                if (!tempRooms.empty()) {
                    auto &&weightedRoom = this->GetWeightedRoom(state, tempRooms);
                    auto &&connections  = weightedRoom.GetConnections();

                    Vector3f vector;
                    Quaternion rot;
                    this->CalculateRoomPosRot(*connections[0], roomConnection.get().m_pos,
                                              roomConnection.get().m_rot, vector, rot);

                    bool flag = false;
                    for (auto &&room : m_placed_rooms) {
                        if (room->m_room.get().m_divider
                            && room->m_pos.sq_distance_to(vector) < 0.5f * 0.5f) {
                            flag = true;
                            break;
                        }
                    }

                    if (!flag) {
                        LOG_WARNING(AVL_LOGGER, "Cyclic detected: Door mismatch for cyclic room");
                    }
                } else {
                    LOG_WARNING(AVL_LOGGER, "Cyclic detected: Door mismatch for cyclic room");
                }
            } else {
                LOG_INFO(AVL_LOGGER, "Cyclic detected: Door types successfully match");
            }

            ++itr1;
        } else {
            auto &&tempRooms = this->FindEndCaps(state, roomConnection.get().m_connection);
            bool flag2       = false;

            bool erased = false;

            if (!flag2 && this->m_dungeon.m_alternative_functionality) {
                for (int k = 0; k < 5; k++) {
                    auto &&weightedRoom2 = this->GetWeightedRoom(state, tempRooms);
                    if (this->PlaceRoom(state, itr1, weightedRoom2, &erased)) {
                        flag2 = true;
                        break;
                    }
                }
            }

            if (!flag2) {
                // std::stable_sort is used because equal element value order are maintained
                std::stable_sort(tempRooms.begin(), tempRooms.end(),
                                 [](std::reference_wrapper<Room const> const &a,
                                    std::reference_wrapper<Room const> const &b) {
                                     return a.get().m_endCapPrio > b.get().m_endCapPrio;
                                 });

                for (auto &&roomData : tempRooms) {
                    if (this->PlaceRoom(state, itr1, roomData, &erased)) {
                        flag2 = true;
                        break;
                    }
                }
            }

            if (!flag2) {
                LOG_WARNING(AVL_LOGGER, "Failed to place end cap");
            }

            if (!erased) {
                ++itr1;
            }
        }
    }
}

std::vector<std::reference_wrapper<Room const>> DungeonGenerator::FindDividers(VUtils::Random::State &state)
{
    std::vector<std::reference_wrapper<Room const>> rooms;

    for (auto &&roomData : m_dungeon.m_available_rooms) {
        if (roomData->m_divider)
            rooms.push_back(*roomData);
    }

    auto i = rooms.size();
    while (i > 1) {
        i--;
        int index    = state.range(0, i);
        auto &&value = rooms[index];
        rooms[index] = rooms[i];
        rooms[i]     = value;
    }

    return rooms;
}

std::vector<std::reference_wrapper<Room const>>
DungeonGenerator::FindEndCaps(VUtils::Random::State &state, RoomConnection const &connection)
{
    std::vector<std::reference_wrapper<Room const>> rooms;

    for (auto &&roomData : m_dungeon.m_available_rooms) {
        if (roomData->m_endCap && roomData->HaveConnection(connection))
            rooms.push_back(*roomData);
    }

    // Inlined .Shuffle
    auto i = rooms.size();
    while (i > 1) {
        i--;
        int index    = state.range(0, i);
        auto &&value = rooms[index];
        rooms[index] = rooms[i];
        rooms[i]     = value;
    }

    return rooms;
}

void DungeonGenerator::PlaceRooms(VUtils::Random::State &state)
{
    for (int i = 0; i < this->m_dungeon.m_max_rooms; i++) {
        this->PlaceOneRoom(state);
        if (this->CheckRequiredRooms() && m_placed_rooms.size() > this->m_dungeon.m_min_rooms) {
            LOG_INFO(AVL_LOGGER, "All required rooms have been placed, stopping generation");
            return;
        }
    }
}

void DungeonGenerator::PlaceStartRoom(VUtils::Random::State &state)
{
    auto &&roomData = this->FindStartRoom(state);
    auto &&entrance = roomData.GetEntrance();

    Vector3f pos;
    Quaternion rot;
    this->CalculateRoomPosRot(entrance, Vector3f::ZERO, Quaternion::IDENTITY, pos, rot);

    // TODO room.POS and room.ROT are ultimately redundant
    auto global = VUtils::Physics::LocalToGlobal(entrance.m_localPos, entrance.m_localRot, roomData.m_pos,
                                                 roomData.m_rot);

    RoomConnectionInstance dummy = RoomConnectionInstance(entrance, global.first, global.second, 0);

    {
        Vector3f size = rot * roomData.m_size;

        size.x = std::abs(size.x);
        size.z = std::abs(size.z);

        size *= .5f;

        //LOG(INFO) << "start: "
        //	<< "polygon(("
        //	<< pos.x - size.x << "," << pos.z - size.z << "),("
        //	<< pos.x - size.x << "," << pos.z + size.z << "),("
        //	<< pos.x + size.x << "," << pos.z + size.z << "),("
        //	<< pos.x + size.x << "," << pos.z - size.z << "))";
    }

    this->PlaceRoom(roomData, pos, rot, dummy);
}

bool DungeonGenerator::PlaceOneRoom(VUtils::Random::State &state)
{

    // Get a random attachment point for the next room
    auto &&itr = this->GetOpenConnection(state);
    if (itr == m_open_connections.end())
        return false;

    auto &&openConnection = itr->get();

    for (int i = 0; i < 10; i++) {
        // Get a new random room to attach to the existing open instanced connect point
        Room const *roomData = this->m_dungeon.m_alternative_functionality
                                       ? this->GetRandomWeightedRoom(state, &openConnection)
                                       : this->GetRandomRoom(state, &openConnection);
        if (!roomData)
            break;

        if (this->PlaceRoom(state, itr, *roomData, nullptr))
            return true;
    }
    return false;
}

void DungeonGenerator::CalculateRoomPosRot(RoomConnection const &roomCon, Vector3f pos, Quaternion rot,
                                           Vector3f &outPos, Quaternion &outRot)
{
    outRot = rot * Quaternion::inverse(roomCon.m_localRot);
    outPos = pos - outRot * roomCon.m_localPos;
}

bool DungeonGenerator::PlaceRoom(VUtils::Random::State &state, decltype(m_open_connections)::iterator &itr,
                                 Room const &room, bool *outErased)
{

    auto &&connection = itr->get();

    auto &&connection2 = room.GetConnection(state, connection.m_connection);

    if (outErased)
        *outErased = false;

    Vector3f pos;
    Quaternion rot;
    this->CalculateRoomPosRot(connection2, connection.m_pos,
                              connection.m_rot
                                      * (AVL_SETTINGS.dungeonsRoomsFlipped ? Quaternion::euler(0, 180, 0)
                                                                           : Quaternion::IDENTITY),
                              pos, rot);

    // this is making me want to rip my hair out
    // https://www.desmos.com/calculator/hykg8ckp3i

    //pos += connection2.m_localPos;

    //pos += connection.m_connection.get().m_localPos;
    //pos += {0, 0, 1};
    //rot = Quaternion::IDENTITY;

    const auto& size = room.m_size;

    if (size.x != 0 && size.z != 0 && this->TestCollision(room, pos, rot)) {
        return false;
    }

    //auto&& snap = [](float v, float eps = 0.001)
    //{
    //    float r = std::round(v);
    //    if (std::fabs(v - r) < eps)
    //        return r;
    //    return v;
    //};
//
    //auto&& formatFloat1 = [&](float v, int precision = 5) {
    //    std::ostringstream ss;
    //    ss << std::fixed; // << std::setprecision(precision);
    //    ss << snap(v);
    //    return ss.str();
    //};
//
    //auto&& formatFloat = [&](float v) {
    //    // "{:g}" automatically handles removing trailing zeros 
    //    // and choosing the shortest representation, but to be 100% 
    //    // safe from 'e', we can use "{:.3f}" and trim.
    //    std::string s = std::format("{:.3f}", snap(v));
    //    s.erase(s.find_last_not_of('0') + 1, std::string::npos);
    //    if (s.back() == '.') s.pop_back();
    //    return s;
    //};

    //// show room boundaries
    ////std::string* desmos_dbg
    //Vector3f v3_a_br = pos + rot * Vector3f(size.x*0.5f, pos.y, -size.z*0.5f);
    //Vector3f v3_a_bl = pos + rot * Vector3f(-size.x*0.5f, pos.y, -size.z*0.5f);
    //Vector3f v3_a_ur = pos + rot * Vector3f(size.x*0.5f, pos.y, size.z*0.5f);
    //Vector3f v3_a_ul = pos + rot * Vector3f(-size.x*0.5f, pos.y, size.z*0.5f);
    ////desmos_dbg_ss << "polygon((" << v3_a_br.x << "," << v3_a_br.z << "),(" << v3_a_ur.x << "," << v3_a_ur.z << "),("
    ////    << v3_a_ul.x << "," << v3_a_ul.z << "),(" << v3_a_bl.x << "," << v3_a_bl.z << ")),";
    /////desmos_dbg_ss << "polygon((" 
    /////    << snap(v3_a_br.x) << "," << snap(v3_a_br.z) << "),("
    /////    << snap(v3_a_ur.x) << "," << snap(v3_a_ur.z) << "),("
    /////    << snap(v3_a_ul.x) << "," << snap(v3_a_ul.z) << "),("
    /////    << snap(v3_a_bl.x) << "," << snap(v3_a_bl.z) << ")),"; 
    //desmos_dbg_ss << "polygon(("
    //    << formatFloat(v3_a_br.x) << "," << formatFloat(v3_a_br.z) << "),("
    //    << formatFloat(v3_a_ur.x) << "," << formatFloat(v3_a_ur.z) << "),("
    //    << formatFloat(v3_a_ul.x) << "," << formatFloat(v3_a_ul.z) << "),("
    //    << formatFloat(v3_a_bl.x) << "," << formatFloat(v3_a_bl.z) << ")),";
    ////LOG_INFO(AVL_LOGGER, "{}", ss.str());






    this->PlaceRoom(room, pos, rot, connection);
    if (!room.m_endCap) {
        if (connection.m_connection.get().m_allowDoor
            && (!connection.m_connection.get().m_doorOnlyIfOtherAlsoAllowsDoor || connection2.m_allowDoor)) {
            m_door_connections.push_back(connection);
        }

        itr = m_open_connections.erase(itr);

        if (outErased)
            *outErased = true;
    }

    return true;
}

void DungeonGenerator::PlaceRoom(Room const &room, Vector3f pos, Quaternion rot)
{
    // TODO seed is only useful for RandomSpawn
    int seed = (int) pos.x * 4271 + (int) pos.y * 9187 + (int) pos.z * 2134;

    //VUtils::Random::State state(seed);
    //for (auto&& randomSpawn : room.m_randomSpawns)
    //	randomSpawn.Randomize();

    for (auto &&view : room.m_netViews) {
        Vector3f pos1   = pos + rot * view.m_pos;
        Quaternion rot1 = rot * view.m_rot;

        // TODO
        //  see the below PlaceRoom(room, pos, rot, CONN)
        //  why does that one convert to global coords? ie, LocalToGlobal used,
        //  but this one does not?!?
        auto &&zdo = ZDOManager()->Instantiate(view.m_prefabHash, pos1);
        zdo->set_rotation(rot1);
    }

    // TODO this might be redundant for dummy 'dungeons' (plains villages shouldnt be considered dungeons)
    auto component2 = std::make_unique<RoomInstance>(room, pos, rot, 0, seed);
    m_placed_rooms.push_back(std::move(component2));
}

void DungeonGenerator::PlaceRoom(Room const &room, Vector3f pos, Quaternion rot,
                                 RoomConnectionInstance const &fromConnection)
{
    // wtf is the point of this?
    //	only setting a seed? seems really extraneous
    //Vector3f vector = pos;
    //if (this->m_dungeon.m_useCustomInteriorTransform)
    //vector -= this->m_pos;
    //int seed = (int)vector.x * 4271 + (int)vector.y * 9187 + (int)vector.z * 2134;

    // TODO seed is only useful for RandomSpawn
    int seed = (int) pos.x * 4271 + (int) pos.y * 9187 + (int) pos.z * 2134;

    // Per Unity docs
    //  https://docs.unity3d.com/ScriptReference/MonoBehaviour.Awake.html
    //"Example2. This causes Example1.Awake() to be called. The Space key is used to perform this"
    // Calling SetActive() on a scene disabled object will call Awake()

    // TODO init RandomSpawns
    //VUtils::Random::State state(seed);
    //for (auto&& randomSpawn : room.m_randomSpawns)
    //	randomSpawn.Randomize();

    if (AVL_SETTINGS.dungeonsRoomsFurnishing) {
        for (auto &&view : room.m_netViews) {
            Vector3f pos1   = pos + rot * view.m_pos;
            Quaternion rot1 = rot * view.m_rot;

            // Prefabs can be instantiated exactly in world space (not local room space)
            auto [gPos, gRot] = VUtils::Physics::LocalToGlobal(pos1, rot1, this->m_pos, this->m_rot);

            auto &&zdo = ZDOManager()->Instantiate(view.m_prefabHash, gPos);
            zdo->set_rotation(gRot);
        }
    }

    auto component2 = std::make_unique<RoomInstance>(room, pos, rot, fromConnection.m_placeOrder + 1, seed);

    this->AddOpenConnections(*component2, fromConnection);

    m_placed_rooms.push_back(std::move(component2));
}

void DungeonGenerator::AddOpenConnections(RoomInstance &newRoom, RoomConnectionInstance const &skipConnection)
{
    auto &&connections = newRoom.m_connections;
    for (auto &&roomConnection : connections) {
        if (!roomConnection->m_connection.get().m_entrance
            && roomConnection->m_pos.sq_distance_to(skipConnection.m_pos) >= .1f * .1f) {
            roomConnection->m_placeOrder = newRoom.m_placeOrder;
            m_open_connections.push_back(*roomConnection.get());
        }
    }
}

// Determine whether a room (with center at origin of room) is completely contained within a zone
// TODO rename something better, wtf is 'IsInsideDungeon'
//	this just makes sure that a rotated rectangle is within the zone
bool DungeonGenerator::IsInsideZone(Room const &room, Vector3f pos, Quaternion rot)
{
    if (!AVL_SETTINGS.dungeonsRoomsZoneBounded)
        return true;

    Vector3f semiSize = room.m_size * .5f;

    if (room.m_endCap)
        semiSize *= AVL_SETTINGS.dungeonsEndcapsInsetFrac;

    if (pos.y + semiSize.y < m_zone_center.y - m_zone_size.y * .5f
        || pos.y - semiSize.y > m_zone_center.y + m_zone_size.y * .5f)
        return false;

    Vector3f a = pos + rot * Vector3f(-semiSize.x, 0, -semiSize.z);
    Vector3f b = pos + rot * Vector3f(-semiSize.x, 0, semiSize.z);
    Vector3f c = pos + rot * Vector3f(semiSize.x, 0, semiSize.z);
    Vector3f d = pos + rot * Vector3f(semiSize.x, 0, -semiSize.z);

    //Vector3f e = pos + rot * Vector3f(-semiSize.x,	0,	-semiSize.z);
    //Vector3f f = pos + rot * Vector3f(-semiSize.x,	0,	semiSize.z);
    //Vector3f g = pos + rot * Vector3f(semiSize.x,		0,	semiSize.z);
    //Vector3f h = pos + rot * Vector3f(semiSize.x,		0,	-semiSize.z);

    static auto &&inRectSemi = [](Vector3f const &semiSize, Vector3f const &pos, Vector3f const &point) {
        return point.x >= pos.x - semiSize.x
               && point.x <= pos.x + semiSize.x
               //&& point.y >= pos.y - semiSize.y && point.y <= pos.y + semiSize.y
               && point.z >= pos.z - semiSize.z && point.z <= pos.z + semiSize.z;
    };

    Vector3f semiZone = m_zone_size * .5f;

    return inRectSemi(semiZone, m_zone_center, a) && inRectSemi(semiZone, m_zone_center, b)
           && inRectSemi(semiZone, m_zone_center, c) && inRectSemi(semiZone, m_zone_center, d);
    //&& inRectSemi(semiZone, m_zoneCenter, e)
    //&& inRectSemi(semiZone, m_zoneCenter, f)
    //&& inRectSemi(semiZone, m_zoneCenter, g)
    //&& inRectSemi(semiZone, m_zoneCenter, h);

    //return VUtils::Physics::RectInsideRect(
    //	m_zoneSize, m_zoneCenter, Quaternion::IDENTITY,
    //	room.m_size, pos, rot);
}

//[[deprecated("axis aligned only")]]
//tatic bool RectOverlapRect(Vector3f size1, Vector3f pos1, Vector3f size2, Vector3f pos2)
//
//   assert(size1.x >= 0 && size1.y >= 0 && size1.z >= 0 && size2.x >= 0 && size2.y >= 0 && size2.z >= 0);
//
//   size1 *= .5f;
//   size2 *= .5f;
//
//   return !(pos1.x + size1.x < pos2.x - size2.x || pos1.y + size1.y < pos2.y - size2.y
//            || pos1.z + size1.z < pos2.z - size2.z || pos1.x - size1.x > pos2.x + size2.x
//            || pos1.y - size1.y > pos2.y + size2.y || pos1.z - size1.z > pos2.z + size2.z);
//




bool DungeonGenerator::TestCollision(Room const &room, Vector3f pos, Quaternion rot)
{

    {
        Vector3f newPos   = pos;
        Quaternion newRot = rot;

        if (m_dungeon.m_algorithm == Dungeon::Algorithm::Dungeon)
            std::tie(newPos, newRot) = VUtils::Physics::LocalToGlobal(pos, rot, this->m_pos, this->m_rot);

        // Constrain dungeon within zone
        if (!this->IsInsideZone(room, newPos, newRot))
            return true;
    }

    auto size1 = room.m_size - Vector3f::ONE * AVL_SETTINGS.dungeonsRoomsInsetSize;

    for (auto const& other : m_placed_rooms) {
        if (VUtils::Physics::BoxBoxOverlap(
            pos, size1, rot, 
            other->m_pos, other->m_room.get().m_size, other->m_rot)) 
            {
                return true;
            }
    }

    return false;
}

Room const *DungeonGenerator::GetRandomWeightedRoom(VUtils::Random::State &state, bool perimeterRoom)
{
    std::vector<Room const *> tempRooms;

    float num = 0;
    for (auto &&roomData : m_dungeon.m_available_rooms) {
        if (!roomData->m_entrance && !roomData->m_endCap && !roomData->m_divider
            && roomData->m_perimeter == perimeterRoom) {
            num += roomData->m_weight;
            tempRooms.push_back(roomData.get());
        }
    }

    if (tempRooms.empty())
        return nullptr;

    float num2 = state.range(0.f, num);
    float num3 = 0;
    for (auto &&roomData2 : tempRooms) {
        num3 += roomData2->m_weight;
        if (num2 <= num3)
            return roomData2;
    }

    std::unreachable();
}

Room const *DungeonGenerator::GetRandomWeightedRoom(VUtils::Random::State &state,
                                                    RoomConnectionInstance const *connection)
{
    std::vector<std::reference_wrapper<Room const>> tempRooms;

    for (auto &&roomData : m_dungeon.m_available_rooms) {
        if (!roomData->m_entrance && !roomData->m_endCap && !roomData->m_divider
            && (!connection
                || (roomData->HaveConnection(connection->m_connection)
                    && connection->m_placeOrder >= roomData->m_minPlaceOrder))) {
            tempRooms.push_back(*roomData);
        }
    }

    // This case is possible with DG_DvergrBoss
    if (tempRooms.empty())
        return nullptr;

    return &this->GetWeightedRoom(state, tempRooms);
}

Room const &DungeonGenerator::GetWeightedRoom(VUtils::Random::State &state,
                                              std::vector<std::reference_wrapper<Room const>> const &rooms)
{
    float num = 0;
    for (auto &&roomData : rooms) num += roomData.get().m_weight;

    float num2 = state.range(0.f, num);
    float num3 = 0;
    for (auto &&roomData2 : rooms) {
        num3 += roomData2.get().m_weight;
        if (num2 <= num3)
            return roomData2;
    }

    std::unreachable();

    //throw std::runtime_error("unexpected");
    //return *m_tempRooms[0];
}

Room const *DungeonGenerator::GetRandomRoom(VUtils::Random::State &state,
                                            RoomConnectionInstance const *connection)
{
    std::vector<std::reference_wrapper<Room const>> tempRooms;

    for (auto &&roomData : m_dungeon.m_available_rooms) {
        if (!roomData->m_entrance && !roomData->m_endCap && !roomData->m_divider
            && (!connection
                || (roomData->HaveConnection(connection->m_connection)
                    && connection->m_placeOrder >= roomData->m_minPlaceOrder))) {
            tempRooms.push_back(*roomData.get());
        }
    }

    if (tempRooms.empty())
        return nullptr;

    return &tempRooms[state.range(0, tempRooms.size())].get();
}

decltype(DungeonGenerator::m_open_connections)::iterator
DungeonGenerator::GetOpenConnection(VUtils::Random::State &state)
{
    if (m_open_connections.empty())
        return m_open_connections.end();

    return std::next(m_open_connections.begin(), state.range(0, m_open_connections.size()));
}

Room const &DungeonGenerator::FindStartRoom(VUtils::Random::State &state)
{
    std::vector<std::reference_wrapper<Room const>> tempRooms;

    for (auto &&roomData : m_dungeon.m_available_rooms) {
        if (roomData->m_entrance)
            tempRooms.push_back(*roomData.get());
    }

    return tempRooms[state.range(0, tempRooms.size())];
}

bool DungeonGenerator::CheckRequiredRooms()
{
    if (this->m_dungeon.m_min_required_rooms == 0 || this->m_dungeon.m_requiredRooms.empty())
        return false;

    int num = 0;
    for (auto &&room : m_placed_rooms) {
        if (this->m_dungeon.m_requiredRooms.contains(room->m_room.get().m_name))
            num++;
    }

    return num >= this->m_dungeon.m_min_required_rooms;
}
#endif// AVL_DUNGEON_GENERATION