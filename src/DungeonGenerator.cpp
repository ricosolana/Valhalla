#include "DungeonGenerator.h"

#include "ZoneManager.h"
#include "WorldManager.h"
#include "GeoManager.h"
#include "VUtilsMathf.h"
#include "VUtilsPhysics.h"

DungeonGenerator::DungeonGenerator(Vector3 pos, Quaternion rot) :
	m_pos(pos), m_rot(rot) {
	auto seed = GeoManager()->GetSeed();
	auto zone = IZoneManager::WorldToZonePos(m_pos);
	this->m_generatedSeed = seed + zone.x * 4271 + zone.y * -7187 + (int)m_pos.x * -4271 + (int)m_pos.y * 9187 + (int)m_pos.z * -2134;

	this->m_zoneCenter = IZoneManager::ZoneToWorldPos(IZoneManager::WorldToZonePos(m_pos));
	this->m_zoneCenter.y = m_pos.y - this->m_dungeon->m_originalPosition.y;
}

// TODO generate seed during start
HASH_t DungeonGenerator::GetSeed() {
	return m_generatedSeed;
}

void DungeonGenerator::DungeonGenerator::Generate() {
	LOG(INFO) << "Available rooms:" << m_dungeon->m_availableRooms.size();
	LOG(INFO) << "To place: " << m_dungeon->m_maxRooms;

	VUtils::Random::State state(m_generatedSeed);

	this->GenerateRooms(state);
	this->Save();

	LOG(INFO) << "Placed " << m_placedRooms.size() << " rooms";
	LOG(INFO) << "Dungeon generated with seed " << m_generatedSeed;
}

void DungeonGenerator::GenerateRooms(VUtils::Random::State& state) {
	switch (this->m_dungeon->m_algorithm) {
	case Dungeon::Algorithm::Dungeon:
		this->GenerateDungeon(state);
		break;
	case Dungeon::Algorithm::CampGrid:
		this->GenerateCampGrid(state);
		break;
	case Dungeon::Algorithm::CampRadial:
		this->GenerateCampRadial(state);
		break;
	}
}

void DungeonGenerator::GenerateDungeon(VUtils::Random::State& state) {
	this->PlaceStartRoom(state);
	this->PlaceRooms(state);
	this->PlaceEndCaps(state);
	this->PlaceDoors(state);
}

void DungeonGenerator::GenerateCampGrid(VUtils::Random::State& state) {
	float num = std::cos(0.017453292f * this->m_dungeon->m_maxTilt);
	Vector3 a = m_pos + Vector3((float)(-this->m_dungeon->m_gridSize) * this->m_dungeon->m_tileWidth * 0.5f, 
		0.f, 
		(float)(-this->m_dungeon->m_gridSize) * this->m_dungeon->m_tileWidth * 0.5f);
	for (int i = 0; i < this->m_dungeon->m_gridSize; i++)
	{
		for (int j = 0; j < this->m_dungeon->m_gridSize; j++)
		{
			if (state.Value() <= this->m_dungeon->m_spawnChance) {
				Vector3 pos = a + Vector3((float)j * this->m_dungeon->m_tileWidth, 0.f, (float)i * this->m_dungeon->m_tileWidth);
				auto randomWeightedRoom = this->GetRandomWeightedRoom(state, false);
				if (randomWeightedRoom)
				{
					Vector3 vector;
					Biome biome;
					BiomeArea biomeArea;
					ZoneManager()->GetGroundData(pos, vector, biome, biomeArea);
					if (vector.y < num)
						continue;

					Quaternion rot = Quaternion::Euler(0, (float)state.Range(0, 16) * 22.5f, 0.f);
					this->PlaceRoom(*randomWeightedRoom, pos, rot, nullptr);
				}
			}
		}
	}
}

void DungeonGenerator::GenerateCampRadial(VUtils::Random::State& state) {
	float num = state.Range(this->m_dungeon->m_campRadiusMin, this->m_dungeon->m_campRadiusMax);
	float num2 = std::cos(0.017453292f * this->m_dungeon->m_maxTilt);
	int num3 = state.Range(this->m_dungeon->m_minRooms, this->m_dungeon->m_maxRooms);
	int num4 = num3 * 20;
	int num5 = 0;
	for (int i = 0; i < num4; i++) {
		Vector3 vector = m_pos + Quaternion::Euler(0.f, (float)state.Range(0, 360), 0.f) 
			* Vector3::FORWARD * state.Range(0.f, num - this->m_dungeon->m_perimeterBuffer);
		auto randomWeightedRoom = this->GetRandomWeightedRoom(state, false);
		if (randomWeightedRoom) {
			Vector3 vector2;
			Biome biome;
			BiomeArea biomeArea;
			ZoneManager()->GetGroundData(vector, vector2, biome, biomeArea);
			if (vector2.y < num2 || vector.y - IZoneManager::WATER_LEVEL < this->m_dungeon->m_minAltitude)
				continue;

			Quaternion campRoomRotation = this->GetCampRoomRotation(state, *randomWeightedRoom, vector);
			if (!this->TestCollision(*randomWeightedRoom, vector, campRoomRotation))
			{
				this->PlaceRoom(*randomWeightedRoom, vector, campRoomRotation, nullptr);
				num5++;
				if (num5 >= num3)
					break;
			}
		}
	}

	if (this->m_dungeon->m_perimeterSections > 0)
		this->PlaceWall(state, num, this->m_dungeon->m_perimeterSections);
}

Quaternion DungeonGenerator::GetCampRoomRotation(VUtils::Random::State& state, Room& room, Vector3 pos) {
	if (room.m_faceCenter) {
		Vector3 vector = m_pos - pos;
		vector.y = 0;
		if (vector == Vector3::ZERO)
		{
			vector = Vector3::FORWARD;
		}
		vector.Normalize();
		float y = VUtils::Mathf::Round(VUtils::Math::YawFromDirection(vector) / 22.5f) * 22.5f;
		return Quaternion::Euler(0, y, 0);
	}

	return Quaternion::Euler(0, (float) state.Range(0, 16) * 22.5f, 0);
}

void DungeonGenerator::PlaceWall(VUtils::Random::State& state, float radius, int sections) {
	float num = std::cos(0.017453292f * this->m_dungeon->m_maxTilt);
	int num2 = 0;
	int num3 = sections * 20;
	for (int i = 0; i < num3; i++)
	{
		Room* randomWeightedRoom = this->GetRandomWeightedRoom(state, true);
		if (randomWeightedRoom)
		{
			Vector3 vector = m_pos + Quaternion::Euler(0, (float)state.Range(0, 360), 0) * Vector3::FORWARD * radius;
			Quaternion campRoomRotation = this->GetCampRoomRotation(state, *randomWeightedRoom, vector);

			Vector3 vector2;
			Biome biome;
			BiomeArea biomeArea;
			ZoneManager()->GetGroundData(vector, vector2, biome, biomeArea);
			if (vector2.y < num || vector.y - IZoneManager::WATER_LEVEL < this->m_dungeon->m_minAltitude)
				continue;

			if (!this->TestCollision(*randomWeightedRoom, vector, campRoomRotation))
			{
				this->PlaceRoom(*randomWeightedRoom, vector, campRoomRotation, nullptr);
				num2++;
				if (num2 >= sections) {
					break;
				}
			}
		}
	}
}

void DungeonGenerator::Save() {
	m_zdo->Set("rooms", (int32_t) m_placedRooms.size());
	for (int i = 0; i < m_placedRooms.size(); i++) {
		auto&& instance = m_placedRooms[i];
		auto&& room = instance.m_room;
		std::string text = "room" + std::to_string(i);
		m_zdo->Set(text, room.get().GetHash());
		m_zdo->Set(text + "_pos", instance.m_pos); // Do NOT use room templated transform; instead use the room instance transform (make a new class called RoomInstance)
		m_zdo->Set(text + "_rot", instance.m_rot);
		m_zdo->Set(text + "_seed", instance.m_seed);
	}
}

Dungeon::DoorDef *DungeonGenerator::FindDoorType(VUtils::Random::State& state, std::string type) {
	std::vector<Dungeon::DoorDef*> list;
	for (auto&& doorDef : this->m_dungeon->m_doorTypes) {
		if (doorDef->m_connectionType == type) {
			list.push_back(doorDef.get());
		}
	}

	if (list.empty())
		return nullptr;

	return list[state.Range(0, list.size())];
}

void DungeonGenerator::PlaceDoors(VUtils::Random::State& state) {
	int num = 0;
	for (auto&& roomConnection : m_doorConnections) {
		auto&& doorDef = this->FindDoorType(state, roomConnection.m_connection.get().m_type);
		if (!doorDef)
		{
			LOG(INFO) << "No door type for connection: " << roomConnection.m_connection.get().m_type;
		}
		else if ((doorDef->m_chance <= 0 || state.Value() <= doorDef->m_chance) 
			&& (doorDef->m_chance > 0 || state.Value() <= this->m_dungeon->m_doorChance))
		{
			PrefabManager()->Instantiate(doorDef->m_prefab, roomConnection.m_pos, roomConnection.m_rot);
			num++;
		}
	}

	LOG(INFO) << "placed " << num << " doors";
}

void DungeonGenerator::PlaceEndCaps(VUtils::Random::State &state) {
	for (int i = 0; i < m_openConnections.size(); i++) {
		auto&& roomConnection = m_openConnections[i];
		RoomConnectionInstance *roomConnection2 = nullptr;

		for (auto&& con2 : m_openConnections) {
			if (&con2 == &roomConnection)
				continue;

			if (roomConnection.TestContact(con2))
			{
				roomConnection2 = &con2; // m_openConnections[j];
				break;
			}
		}

		for (int j = 0; j < m_openConnections.size(); j++)
		{
			if (j != i && roomConnection.TestContact(m_openConnections[j]))
			{
				roomConnection2 = &m_openConnections[j];
				break;
			}
		}

		if (roomConnection2) {
			if (roomConnection.m_connection.get().m_type != roomConnection2->m_connection.get().m_type)
			{
				this->FindDividers(state, m_tempRooms);
				if (!m_tempRooms.empty())
				{
					auto&& weightedRoom = this->GetWeightedRoom(state, m_tempRooms);
					auto&& connections = weightedRoom.GetConnections();
					Vector3 vector;
					Quaternion rot = Quaternion::IDENTITY;
					this->CalculateRoomPosRot(*connections[0], roomConnection.m_pos, 
						roomConnection.m_rot, vector, rot);
					bool flag = false;
					for (auto&& room : m_placedRooms)
					{
						if (room.m_room.get().m_divider && room.m_pos.Distance(vector) < 0.5f) // TODO sqdistance
						{
							flag = true;
							break;
						}
					}

					if (!flag) LOG(WARNING) << "Cyclic detected: Door mismatch for cyclic room";
				}
				else
				{
					LOG(WARNING) << "Cyclic detected: Door mismatch for cyclic room";
				}
			}
			else
			{
				LOG(INFO) << "Cyclic detected: Door types successfully match";
			}
		}
		else
		{
			this->FindEndCaps(state, roomConnection.m_connection, m_tempRooms);
			bool flag2 = false;
			if (this->m_dungeon->m_alternativeFunctionality)
			{
				for (int k = 0; k < 5; k++)
				{
					auto&& weightedRoom2 = this->GetWeightedRoom(state, m_tempRooms);
					if (this->PlaceRoom(state, roomConnection, weightedRoom2))
					{
						flag2 = true;
						break;
					}
				}
			}

			if (!flag2) {
				// make a copy for sorting
				std::vector<Room*> sorted = m_tempRooms;

				// std::stable_sort is used because equal element value order are maintained
				std::stable_sort(sorted.begin(), sorted.end(), [](const Room* a, const Room* b) {
					return a->m_endCapPrio > b->m_endCapPrio;
				});

				for (auto&& roomData : sorted) {
					if (this->PlaceRoom(state, roomConnection, *roomData)) {
						flag2 = true;
						break;
					}
				}

			}

			if (!flag2)
				LOG(WARNING) << "Failed to place end cap";
		}
	}
}

void DungeonGenerator::FindDividers(VUtils::Random::State& state, std::vector<Room*>& rooms) {
	rooms.clear();
	for (auto&& roomData : m_dungeon->m_availableRooms)
	{
		if (roomData->m_divider)
		{
			rooms.push_back(roomData);
		}
	}

	auto i = rooms.size();
	while (i > 1) {
		i--;
		int index = state.Range(0, i);
		auto&& value = rooms[index];
		rooms[index] = rooms[i];
		rooms[i] = value;
	}
}

void DungeonGenerator::FindEndCaps(VUtils::Random::State& state, RoomConnection &connection, std::vector<Room*>& rooms) {
	rooms.clear();
	for (auto&& roomData : m_dungeon->m_availableRooms)
	{
		if (roomData->m_endCap && roomData->HaveConnection(connection))
		{
			rooms.push_back(roomData);
		}
	}

	// Inlined .Shuffle
	auto i = rooms.size();
	while (i > 1)
	{
		i--;
		int index = state.Range(0, i);
		auto&& value = rooms[index];
		rooms[index] = rooms[i];
		rooms[i] = value;
	}
}

Room* DungeonGenerator::FindEndCap(VUtils::Random::State& state, RoomConnection &connection) {
	m_tempRooms.clear();
	for (auto&& roomData : m_dungeon->m_availableRooms)
	{
		if (roomData->m_endCap && roomData->HaveConnection(connection))
		{
			m_tempRooms.push_back(roomData);
		}
	}

	if (m_tempRooms.empty())
		return nullptr;

	return m_tempRooms[state.Range(0, m_tempRooms.size())];
}

void DungeonGenerator::PlaceRooms(VUtils::Random::State& state) {
	for (int i = 0; i < this->m_dungeon->m_maxRooms; i++)
	{
		this->PlaceOneRoom(state);
		if (this->CheckRequiredRooms() 
			&& m_placedRooms.size() > this->m_dungeon->m_minRooms) 
		{
			LOG(INFO) << "All required rooms have been placed, stopping generation";
			return;
		}
	}
}

void DungeonGenerator::PlaceStartRoom(VUtils::Random::State& state) {
	auto&& roomData = this->FindStartRoom(state);
	auto&& entrance = roomData.GetEntrance();

	// localPos used because CalculateRoomPosRot(); 
	// pos is not used for the later PlaceRoom...AddOpenConnections because this room is entrance (pos is skipped)
	RoomConnectionInstance dummy = RoomConnectionInstance(entrance, entrance.m_localPos, entrance.m_localRot, 0);

	Vector3 pos;
	Quaternion rot = Quaternion::IDENTITY;
	this->CalculateRoomPosRot(entrance, this->m_pos, this->m_rot, pos, rot);
	this->PlaceRoom(roomData, pos, rot, &dummy);
}

bool DungeonGenerator::PlaceOneRoom(VUtils::Random::State& state) {
	auto&& openConnection = this->GetOpenConnection(state);
	if (!openConnection)
		return false;

	for (int i = 0; i < 10; i++)
	{
		Room* roomData = this->m_dungeon->m_alternativeFunctionality 
			? this->GetRandomWeightedRoom(state, openConnection) 
			: this->GetRandomRoom(state, openConnection);
		if (!roomData)
			break;

		if (this->PlaceRoom(state, *openConnection, *roomData))
			return true;
	}
	return false;
}

void DungeonGenerator::CalculateRoomPosRot(RoomConnection &roomCon, Vector3 exitPos, Quaternion exitRot, Vector3 &pos, Quaternion &rot) {
	Quaternion rhs = Quaternion::Inverse(roomCon.m_localRot);
	rot = exitRot * rhs;
	Vector3 localPosition = roomCon.m_localPos;
	pos = exitPos - rot * localPosition;
}

bool DungeonGenerator::PlaceRoom(VUtils::Random::State& state, RoomConnectionInstance &connection, Room& room) {
	Quaternion quaternion = connection.m_rot;
	quaternion *= Quaternion::Euler(0, 180, 0);
	auto&& connection2 = room.GetConnection(state, connection.m_connection);
	//if (connection2.transform.parent.gameObject != room.gameObject)
		//LOG(WARNING) << "Connection is not placed as child of room";

	Vector3 pos;
	Quaternion rot = Quaternion::IDENTITY;
	this->CalculateRoomPosRot(connection2, connection.m_pos, quaternion, pos, rot);
	if (room.m_size.x != 0 && room.m_size.z != 0 && this->TestCollision(room, pos, rot))
		return false;

	this->PlaceRoom(room, pos, rot, &connection);
	if (!room.m_endCap)
	{
		if (connection.m_connection.get().m_allowDoor
			&& (!connection.m_connection.get().m_doorOnlyIfOtherAlsoAllowsDoor || connection2.m_allowDoor))
		{
			m_doorConnections.push_back(connection);
		}

		// TODO identify whether RoomConnections are modified after their initial creation then passing
		//	will determine whether they must be alive always, or can use copies (instead of references)
		//	because openConnections ends up being popped
		for (auto&& itr = m_openConnections.begin(); itr != m_openConnections.end(); ) {
			if (&*itr == &connection) {
				itr = m_openConnections.erase(itr);
				break;
			}
			else
				++itr;
		}
		//m_openConnections.erase(m_openConnections.begin(), m_openConnections.end(), &connection);
	}
	return true;
}

void DungeonGenerator::PlaceRoom(Room& room, Vector3 pos, Quaternion rot, RoomConnectionInstance *fromConnection) {
	Vector3 vector = pos;
	if (this->m_dungeon->m_useCustomInteriorTransform)
		vector -= this->m_pos;

	int seed = (int)vector.x * 4271 + (int)vector.y * 9187 + (int)vector.z * 2134;

	VUtils::Random::State state(seed);

	//for (auto&& randomSpawn : room.m_randomSpawns)
	//	randomSpawn.Randomize();

	Vector3 position = room.m_pos;
	Quaternion quaternion = Quaternion::Inverse(room.m_rot);
	for (auto&& znetView2 : room.m_netViews) {
		Vector3 point = quaternion * (znetView2.m_pos - position);
		Vector3 position2 = pos + rot * point;
		Quaternion rhs = quaternion * znetView2.m_rot;
		Quaternion rotation = rot * rhs;
		PrefabManager()->Instantiate(znetView2.m_prefab, position2, rotation);
	}
	
	RoomInstance component2 = RoomInstance(room, pos, rot, (fromConnection ? (fromConnection->m_placeOrder + 1) : 0), seed);
	
	//RoomInstance *component2 = UnityEngine.Object.Instantiate<GameObject>(room.m_room.gameObject, pos, rot, base.transform)
		//.GetComponent<Room>();

	m_placedRooms.push_back(component2);
	this->AddOpenConnections(component2, fromConnection);
}

void DungeonGenerator::AddOpenConnections(RoomInstance &newRoom, RoomConnectionInstance *skipConnection) {
	auto&& connections = newRoom.m_connections;
	if (skipConnection) {
		for (auto&& roomConnection : connections) {
			if (!roomConnection.m_connection.get().m_entrance
				&& roomConnection.m_pos.Distance(skipConnection->m_pos) >= .1f) 
			{
				roomConnection.m_placeOrder = newRoom.m_placeOrder;
				m_openConnections.push_back(roomConnection);
			}
		}
	}
	else {
		for (auto&& connection : connections) {
			connection.m_placeOrder = newRoom.m_placeOrder;
		}

		m_openConnections.insert(m_openConnections.end(), 
			connections.begin(), connections.end());
	}
}

// Determine whether a room (with center at origin of room) is completely contained within a zone
bool DungeonGenerator::IsInsideDungeon(Room &room, Vector3 pos, Quaternion rot) {
	return VUtils::Physics::RectInsideRect(
		m_zoneSize, m_zoneCenter, Quaternion::IDENTITY,
		room.m_size, pos, rot);

	//Bounds bounds = new Bounds(this->m_zoneCenter, this->m_zoneSize);
	//Vector3 vector = room.m_size;
	//vector *= 0.5f;
	//
	//// This checks every corner of in the room relative to the room origin
	//return bounds.Contains(pos + rot * Vector3(vector.x, vector.y, -vector.z)) 
	//	&& bounds.Contains(pos + rot * Vector3(-vector.x, vector.y, -vector.z)) 
	//	&& bounds.Contains(pos + rot * Vector3(vector.x, vector.y, vector.z)) 
	//	&& bounds.Contains(pos + rot * Vector3(-vector.x, vector.y, vector.z)) 
	//	&& bounds.Contains(pos + rot * Vector3(vector.x, -vector.y, -vector.z)) 
	//	&& bounds.Contains(pos + rot * Vector3(-vector.x, -vector.y, -vector.z)) 
	//	&& bounds.Contains(pos + rot * Vector3(vector.x, -vector.y, vector.z)) 
	//	&& bounds.Contains(pos + rot * Vector3(-vector.x, -vector.y, vector.z));
}

bool DungeonGenerator::TestCollision(Room &room, Vector3 pos, Quaternion rot) {
	// If room is not entirely within zone, it cannot be placed (might intersect with another different zonedungeon)
	if (!this->IsInsideDungeon(room, pos, rot)) {
		return true;
	}

	// determine whether the room collides with any other room
	for (auto&& other : m_placedRooms) {
		Vector3 size2((float)room.m_size.x - 0.1f, (float)room.m_size.y - 0.1f, (float)room.m_size.z - 0.1f);

		if (VUtils::Physics::RectOverlapRect(room.m_size, pos, rot,
			size2, other.m_pos, other.m_rot))
			return true;
	}

	return false;

	//this->m_colliderA.size = Vector3((float)room.m_size.x - 0.1f, (float)room.m_size.y - 0.1f, (float)room.m_size.z - 0.1f);
	//for (auto&& room2 : m_placedRooms)
	//{
	//	this->m_colliderB.size = room2.m_size;
	//	Vector3 vector;
	//	float num;
	//
	//	// If the 
	//	if (Physics.ComputePenetration(this->m_colliderA, pos, rot, this->m_colliderB, room2.transform.position, room2.transform.rotation, out vector, out num))
	//	{
	//		return true;
	//	}
	//}
	//return false;
}

Room* DungeonGenerator::GetRandomWeightedRoom(VUtils::Random::State& state, bool perimeterRoom) {
	m_tempRooms.clear();
	float num = 0;
	for (auto&& roomData : m_dungeon->m_availableRooms)
	{
		if (!roomData->m_entrance && !roomData->m_endCap && !roomData->m_divider && roomData->m_perimeter == perimeterRoom)
		{
			num += roomData->m_weight;
			m_tempRooms.push_back(roomData);
		}
	}

	if (m_tempRooms.empty())
		return nullptr;

	float num2 = state.Range(0.f, num);
	float num3 = 0;
	for (auto&& roomData2 : m_tempRooms) {
		num3 += roomData2->m_weight;
		if (num2 <= num3)
			return roomData2;
	}
	return m_tempRooms[0];
}

Room* DungeonGenerator::GetRandomWeightedRoom(VUtils::Random::State& state, RoomConnectionInstance *connection) {
	m_tempRooms.clear();
	for (auto&& roomData : m_dungeon->m_availableRooms)
	{
		if (!roomData->m_entrance
			&& !roomData->m_endCap
			&& !roomData->m_divider
			&& (!connection || (roomData->HaveConnection(connection->m_connection) 
				&& connection->m_placeOrder >= roomData->m_minPlaceOrder)))
		{
			m_tempRooms.push_back(roomData);
		}
	}

	if (m_tempRooms.empty())
		return nullptr;

	return &this->GetWeightedRoom(state, m_tempRooms);
}

Room& DungeonGenerator::GetWeightedRoom(VUtils::Random::State& state, std::vector<Room*> &rooms) {
	float num = 0;
	for (auto&& roomData : rooms)
		num += roomData->m_weight;

	float num2 = state.Range(0.f, num);
	float num3 = 0;
	for (auto&& roomData2 : rooms) {
		num3 += roomData2->m_weight;
		if (num2 <= num3)
			return *roomData2;
	}

	return *m_tempRooms[0];
}

Room* DungeonGenerator::GetRandomRoom(VUtils::Random::State& state, RoomConnectionInstance *connection) {
	m_tempRooms.clear();
	for (auto&& roomData : m_dungeon->m_availableRooms) {
		if (!roomData->m_entrance 
			&& !roomData->m_endCap 
			&& !roomData->m_divider 
			&& (!connection 
				|| (roomData->HaveConnection(connection->m_connection) 
					&& connection->m_placeOrder >= roomData->m_minPlaceOrder)))
		{
			m_tempRooms.push_back(roomData);
		}
	}

	if (m_tempRooms.empty())
		return nullptr;

	return m_tempRooms[state.Range(0, m_tempRooms.size())];
}

RoomConnectionInstance* DungeonGenerator::GetOpenConnection(VUtils::Random::State& state) {
	if (m_openConnections.empty())
		return nullptr;

	return &m_openConnections[state.Range(0, m_openConnections.size())];
}

Room& DungeonGenerator::FindStartRoom(VUtils::Random::State& state) {
	m_tempRooms.clear();
	for (auto&& roomData : m_dungeon->m_availableRooms) {
		if (roomData->m_entrance)
			m_tempRooms.push_back(roomData);
	}

	return *m_tempRooms[state.Range(0, m_tempRooms.size())];
}

bool DungeonGenerator::CheckRequiredRooms() {
	if (this->m_dungeon->m_minRequiredRooms == 0 || this->m_dungeon->m_requiredRooms.empty())
		return false;

	int num = 0;
	for (auto&& room : m_placedRooms) {
		if (this->m_dungeon->m_requiredRooms.contains(room.m_room->m_name))
			num++;
	}

	return num >= this->m_dungeon->m_minRequiredRooms;
}
