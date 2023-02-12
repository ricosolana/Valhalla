#include "DungeonGenerator.h"

#include "WorldManager.h"
#include "GeoManager.h"
#include "VUtilsMathf.h"

void DungeonGenerator::DungeonGenerator::Clear() {
	//while (base.transform.childCount > 0)
	//{
	//	UnityEngine.Object.DestroyImmediate(base.transform.GetChild(0).gameObject);
	//}
}

void DungeonGenerator::DungeonGenerator::Generate() {
	int seed = this->GetSeed();
	this->Generate(seed);
}

// TODO generate seed once, dont keep doing it
int DungeonGenerator::GetSeed() {
	this->m_generatedSeed = 0;

	if (this->m_hasGeneratedSeed) {
		return this->m_generatedSeed;
	}

	auto seed = GeoManager()->GetSeed();
	auto zone = IZoneManager::WorldToZonePos(m_pos);
	this->m_generatedSeed = seed + zone.x * 4271 + zone.y * -7187 + (int)m_pos.x * -4271 + (int)m_pos.y * 9187 + (int)m_pos.z * -2134;

	this->m_hasGeneratedSeed = true;
	return this->m_generatedSeed;
}

void DungeonGenerator::DungeonGenerator::Generate(int seed) {
	this->m_generatedSeed = seed;
	this->Clear();
	this->SetupColliders();
	this->SetupAvailableRooms();

	Vector2i zone = IZoneManager::WorldToZonePos(m_pos);
	this->m_zoneCenter = IZoneManager::ZoneToWorldPos(zone);
	this->m_zoneCenter.y = m_pos.y - this->m_originalPosition.y;

	//Bounds bounds = new Bounds(this->m_zoneCenter, this->m_zoneSize);
	//ZLog.Log(string.Format("Generating {0}, Seed: {1}, Bounds diff: {2} / {3}", new object[]
	//	{
	//		base.name,
	//		seed,
	//		bounds.min - base.transform.position,
	//		bounds.max - base.transform.position
	//	}));
	//ZLog.Log("Available rooms:" + DungeonGenerator.m_availableRooms.Count.ToString());
	//ZLog.Log("To place:" + this->m_maxRooms.ToString());
	m_placedRooms.clear();
	m_openConnections.clear();
	m_doorConnections.clear();
	//UnityEngine.Random.State state = UnityEngine.Random.state;
	//UnityEngine.Random.InitState(seed);

	VUtils::Random::State state(seed);

	this->GenerateRooms(state);
	this->Save();

	LOG(INFO) << "Placed " << m_placedRooms.size() << " rooms";

	std::string text;
	for (auto&& room : m_placedRooms)
	{
		text += room.name;
	}
	this->m_generatedHash = VUtils::String::GetStableHashCode(text); //.GetHashCode(); // C# instance-dependent hash

	LOG(INFO) << "Dungeon generated with seed " << seed << " and hash " << this->m_generatedHash;

	//SnapToGround.SnappAll();

	//for (auto&& room2 : m_placedRooms) {
	//	UnityEngine.Object.DestroyImmediate(room2.gameObject);
	//}

	m_placedRooms.clear();
	m_openConnections.clear();
	m_doorConnections.clear();

	//UnityEngine.Object.DestroyImmediate(this->m_colliderA);
	//UnityEngine.Object.DestroyImmediate(this->m_colliderB);
}

void DungeonGenerator::GenerateRooms(VUtils::Random::State& state) {
	switch (this->m_algorithm)
	{
	case Algorithm::Dungeon:
		this->GenerateDungeon(state);
		return;
	case Algorithm::CampGrid:
		this->GenerateCampGrid(state);
		return;
	case Algorithm::CampRadial:
		this->GenerateCampRadial(state);
		return;
	default:
		return;
	}
}

void DungeonGenerator::GenerateDungeon(VUtils::Random::State& state) {
	this->PlaceStartRoom();
	this->PlaceRooms();
	this->PlaceEndCaps();
	this->PlaceDoors(state);
}

void DungeonGenerator::GenerateCampGrid(VUtils::Random::State& state) {
	float num = std::cos(0.017453292f * this->m_maxTilt);
	Vector3 a = m_pos + Vector3((float)(-this->m_gridSize) * this->m_tileWidth * 0.5f, 0.f, (float)(-this->m_gridSize) * this->m_tileWidth * 0.5f);
	for (int i = 0; i < this->m_gridSize; i++)
	{
		for (int j = 0; j < this->m_gridSize; j++)
		{
			if (state.Value() <= this->m_spawnChance)
			{
				Vector3 pos = a + Vector3((float)j * this->m_tileWidth, 0.f, (float)i * this->m_tileWidth);
				auto randomWeightedRoom = this->GetRandomWeightedRoom(false);
				if (randomWeightedRoom)
				{
					Vector3 vector;
					Biome biome;
					BiomeArea biomeArea;
					ZoneManager()->GetGroundData(pos, vector, biome, biomeArea);
					if (vector.y < num)
						continue;

					Quaternion rot = Quaternion::Euler(0, (float)state.Range(0, 16) * 22.5f, 0.f);
					this->PlaceRoom(randomWeightedRoom, pos, rot, null);
				}
			}
		}
	}
}

void DungeonGenerator::GenerateCampRadial(VUtils::Random::State& state) {
	float num = state.Range(this->m_campRadiusMin, this->m_campRadiusMax);
	float num2 = std::cos(0.017453292f * this->m_maxTilt);
	int num3 = state.Range(this->m_minRooms, this->m_maxRooms);
	int num4 = num3 * 20;
	int num5 = 0;
	for (int i = 0; i < num4; i++)
	{
		Vector3 vector = m_pos + Quaternion::Euler(0.f, (float)state.Range(0, 360), 0.f) * Vector3::FORWARD * state.Range(0.f, num - this->m_perimeterBuffer);
		auto randomWeightedRoom = this->GetRandomWeightedRoom(false);
		if (randomWeightedRoom)
		{
			Vector3 vector2;
			Biome biome;
			BiomeArea biomeArea;
			ZoneManager()->GetGroundData(vector, vector2, biome, biomeArea);
			if (vector2.y < num2 || vector.y - IZoneManager::WATER_LEVEL < this->m_minAltitude)
				continue;

			Quaternion campRoomRotation = this->GetCampRoomRotation(randomWeightedRoom, vector);
			if (!this->TestCollision(randomWeightedRoom.m_room, vector, campRoomRotation))
			{
				this->PlaceRoom(randomWeightedRoom, vector, campRoomRotation, null);
				num5++;
				if (num5 >= num3)
				{
					break;
				}
			}
		}
	}
	if (this->m_perimeterSections > 0)
	{
		this->PlaceWall(state, num, this->m_perimeterSections);
	}
}

Quaternion DungeonGenerator::GetCampRoomRotation(VUtils::Random::State& state, RoomData* room, Vector3 pos) {
	if (room.m_room.m_faceCenter)
	{
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
	return Quaternion::Euler(0, (float)state.Range(0, 16) * 22.5f, 0);
}

void DungeonGenerator::PlaceWall(VUtils::Random::State& state, float radius, int sections)
{
	float num = std::cos(0.017453292f * this->m_maxTilt);
	int num2 = 0;
	int num3 = sections * 20;
	for (int i = 0; i < num3; i++)
	{
		RoomData* randomWeightedRoom = this->GetRandomWeightedRoom(true);
		if (randomWeightedRoom)
		{
			Vector3 vector = m_pos + Quaternion::Euler(0, (float)state.Range(0, 360), 0) * Vector3::FORWARD * radius;
			Quaternion campRoomRotation = this->GetCampRoomRotation(randomWeightedRoom, vector);

			Vector3 vector2;
			Biome biome;
			BiomeArea biomeArea;
			ZoneManager()->GetGroundData(vector, vector2, biome, biomeArea);
			if (vector2.y < num || vector.y - IZoneManager::WATER_LEVEL < this->m_minAltitude)
				continue;

			if (!this->TestCollision(randomWeightedRoom.m_room, vector, campRoomRotation))
			{
				this->PlaceRoom(randomWeightedRoom, vector, campRoomRotation, null);
				num2++;
				if (num2 >= sections)
				{
					break;
				}
			}
		}
	}
}

void DungeonGenerator::Save() {
	m_zdo->Set("rooms", (int)m_placedRooms.size());
	for (int i = 0; i < m_placedRooms.size(); i++)
	{
		Room room = DungeonGenerator.m_placedRooms[i];
		std::string text = "room" + std::to_string(i);
		zdo->Set(text, room.GetHash());
		zdo->Set(text + "_pos", room.transform.position);
		zdo->Set(text + "_rot", room.transform.rotation);
		zdo->Set(text + "_seed", room.m_seed);
	}
}

void DungeonGenerator::SetupAvailableRooms() {
	m_availableRooms.clear();
	for (auto&& roomData : DungeonDB.GetRooms())
	{
		if ((roomData.m_room.m_theme & this->m_themes) != (Room.Theme)0 && roomData.m_room.m_enabled)
		{
			m_availableRooms.Add(roomData);
		}
	}
}

DungeonGenerator::DoorDef DungeonGenerator::FindDoorType(VUtils::Random::State& state, std::string type)
{
	std::vector<DoorDef> list;
	for (auto&& doorDef : this->m_doorTypes)
	{
		if (doorDef.m_connectionType == type)
		{
			list.push_back(doorDef);
		}
	}
	if (list.empty()) {
		return nullptr;
	}
	return list[state.Range(0, list.size())];
}

void DungeonGenerator::PlaceDoors(VUtils::Random::State& state) {
	int num = 0;
	for (auto&& roomConnection : m_doorConnections) {
		auto&& doorDef = this->FindDoorType(state, roomConnection.m_type);
		if (!doorDef)
		{
			LOG(INFO) << "No door type for connection: " << roomConnection.m_type;
		}
		else if ((doorDef.m_chance <= 0 || state.Value() <= doorDef.m_chance) && (doorDef.m_chance > 0 || state.Value() <= this->m_doorChance))
		{
			// TODO impl
			//PrefabManager()->Instantiate(doorDef.m_prefab)

				//GameObject obj = UnityEngine.Object.Instantiate<GameObject>(doorDef.m_prefab, roomConnection.transform.position, roomConnection.transform.rotation);

			//UnityEngine.Object.Destroy(obj);

			num++;
		}
	}

	LOG(INFO) << "placed " << num << " doors";
}

void DungeonGenerator::PlaceEndCaps()
{
	for (int i = 0; i < m_openConnections.size(); i++)
	{
		RoomConnection roomConnection = m_openConnections[i];
		RoomConnection roomConnection2 = null;
		for (int j = 0; j < m_openConnections.size(); j++)
		{
			if (j != i && roomConnection.TestContact(m_openConnections[j]))
			{
				roomConnection2 = m_openConnections[j];
				break;
			}
		}
		if (roomConnection2 != null)
		{
			if (roomConnection.m_type != roomConnection2.m_type)
			{
				this->FindDividers(m_tempRooms);
				if (!m_tempRooms.empty())
				{
					RoomData* weightedRoom = this->GetWeightedRoom(m_tempRooms);
					RoomConnection[] connections = weightedRoom.m_room.GetConnections();
					Vector3 vector;
					Quaternion rot;
					this->CalculateRoomPosRot(connections[0], roomConnection.transform.position, roomConnection.transform.rotation, vector, rot);
					bool flag = false;
					for (auto&& room : m_placedRooms)
					{
						if (room.m_divider && room.transform.position.Distance(vector) < 0.5f)
						{
							flag = true;
							break;
						}
					}
					if (!flag)
					{
						LOG(WARNING) << "Cyclic detected; Door mismatch for cyclic room";
					}
				}
				else
				{
					LOG(WARNING) << "Cyclic detected. Door mismatch for cyclic room";
				}
			}
			else
			{
				LOG(INFO) << "cyclic detected and door types successfully match";
			}
		}
		else
		{
			this->FindEndCaps(roomConnection, m_tempRooms);
			bool flag2 = false;
			if (this->m_alternativeFunctionality)
			{
				for (int k = 0; k < 5; k++)
				{
					RoomData* weightedRoom2 = this->GetWeightedRoom(m_tempRooms);
					if (this->PlaceRoom(roomConnection, weightedRoom2, mode))
					{
						flag2 = true;
						break;
					}
				}
			}
			IOrderedEnumerable<RoomData*> orderedEnumerable = from item in DungeonGenerator.m_tempRooms
				orderby item.m_room.m_endCapPrio descending
				select item;
			if (!flag2)
			{
				for (auto&& roomData : orderedEnumerable)
				{
					if (this->PlaceRoom(roomConnection, roomData))
					{
						flag2 = true;
						break;
					}
				}
			}
			if (!flag2)
			{
				LOG(WARNING) << "Failed to place end cap";
				//LOG(WARNING) << "Failed to place end cap " << roomConnection.name << " " << roomConnection.transform.parent.gameObject.name;
			}
		}
	}
}

void DungeonGenerator::FindDividers(VUtils::Random::State& state, std::vector<RoomData*>& rooms) {
	rooms.clear();
	for (auto&& roomData : m_availableRooms)
	{
		if (roomData.m_room.m_divider)
		{
			rooms.push_back(roomData);
		}
	}

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

void DungeonGenerator::FindEndCaps(VUtils::Random::State& state, RoomConnection connection, std::vector<RoomData*>& rooms) {
	rooms.clear();
	for (auto&& roomData : m_availableRooms)
	{
		if (roomData.m_room.m_endCap && roomData.m_room.HaveConnection(connection))
		{
			rooms.push_back(roomData);
		}
	}

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

RoomData* DungeonGenerator::FindEndCap(VUtils::Random::State& state, RoomConnection connection) {
	m_tempRooms.clear();
	for (auto&& roomData : m_availableRooms)
	{
		if (roomData.m_room.m_endCap && roomData.m_room.HaveConnection(connection))
		{
			m_tempRooms.push_back(roomData);
		}
	}
	if (m_tempRooms.empty())
	{
		return null;
	}
	return m_tempRooms[state.Range(0, m_tempRooms.size())];
}

void DungeonGenerator::PlaceRooms() {
	for (int i = 0; i < this->m_maxRooms; i++)
	{
		this->PlaceOneRoom();
		if (this->CheckRequiredRooms() && m_placedRooms.size() > this->m_minRooms)
		{
			LOG(INFO) << "All required rooms have been placed, stopping generation";
			return;
		}
	}
}

void DungeonGenerator::PlaceStartRoom() {
	RoomData* roomData = this->FindStartRoom();
	RoomConnection entrance = roomData.m_room.GetEntrance();
	Quaternion rotation = base.transform.rotation;

	Vector3 pos;
	Quaternion rot = Quaternion::IDENTITY;
	this->CalculateRoomPosRot(entrance, base.transform.position, rotation, pos, rot);
	this->PlaceRoom(roomData, pos, rot, entrance, mode);
}

bool DungeonGenerator::PlaceOneRoom() {
	RoomConnection openConnection = this->GetOpenConnection();
	if (!openConnection)
		return false;

	for (int i = 0; i < 10; i++)
	{
		RoomData* roomData = this->m_alternativeFunctionality ? this->GetRandomWeightedRoom(openConnection) : this->GetRandomRoom(openConnection);
		if (roomData == null)
		{
			break;
		}
		if (this->PlaceRoom(openConnection, roomData, mode))
		{
			return true;
		}
	}
	return false;
}

void DungeonGenerator::CalculateRoomPosRot(RoomConnection roomCon, Vector3 exitPos, Quaternion exitRot, Vector3 &pos, Quaternion &rot) {
	Quaternion rhs = Quaternion.Inverse(roomCon.transform.localRotation);
	rot = exitRot * rhs;
	Vector3 localPosition = roomCon.transform.localPosition;
	pos = exitPos - rot * localPosition;
}

bool DungeonGenerator::PlaceRoom(RoomConnection connection, RoomData* roomData) {
	Room room = roomData.m_room;
	Quaternion quaternion = connection.transform.rotation;
	quaternion *= Quaternion::Euler(0, 180, 0);
	RoomConnection connection2 = room.GetConnection(connection);
	if (connection2.transform.parent.gameObject != room.gameObject)
	{
		LOG(WARNING) << "Connection is not placed as child of room";
	}
	Vector3 pos;
	Quaternion rot;
	this->CalculateRoomPosRot(connection2, connection.transform.position, quaternion, out pos, out rot);
	if (room.m_size.x != 0 && room.m_size.z != 0 && this->TestCollision(room, pos, rot))
	{
		return false;
	}
	this->PlaceRoom(roomData, pos, rot, connection, mode);
	if (!room.m_endCap)
	{
		if (connection.m_allowDoor && (!connection.m_doorOnlyIfOtherAlsoAllowsDoor || connection2.m_allowDoor))
		{
			m_doorConnections.push_back(connection);
		}
		m_openConnections.Remove(connection);
	}
	return true;
}

void DungeonGenerator::PlaceRoom(RoomData* room, Vector3 pos, Quaternion rot, RoomConnection fromConnection)
{
	Vector3 vector = pos;
	if (this->m_useCustomInteriorTransform)
	{
		vector = pos - base.transform.position;
	}
	int seed = (int)vector.x * 4271 + (int)vector.y * 9187 + (int)vector.z * 2134;
	//UnityEngine.Random.State state = UnityEngine.Random.state;
	//UnityEngine.Random.InitState(seed);

	VUtils::Random::State state(seed);

	//for (auto&& znetView in room.m_netViews)
	//{
	//	znetView.gameObject.SetActive(true);
	//}

	UnityEngine.Random.InitState(seed);
	foreach(RandomSpawn randomSpawn in room.m_randomSpawns)
	{
		randomSpawn.Randomize();
	}
	Vector3 position = room.m_room.transform.position;
	Quaternion quaternion = Quaternion.Inverse(room.m_room.transform.rotation);
	using (std::vector<ZNetView>.Enumerator enumerator = room.m_netViews.GetEnumerator())
	{
		while (enumerator.MoveNext())
		{
			ZNetView znetView2 = enumerator.Current;
			if (znetView2.gameObject.activeSelf)
			{
				Vector3 point = quaternion * (znetView2.gameObject.transform.position - position);
				Vector3 position2 = pos + rot * point;
				Quaternion rhs = quaternion * znetView2.gameObject.transform.rotation;
				Quaternion rotation = rot * rhs;
				GameObject gameObject = UnityEngine.Object.Instantiate<GameObject>(znetView2.gameObject, position2, rotation);
				ZNetView component = gameObject.GetComponent<ZNetView>();
				if (component.GetZDO() != null)
				{
					component.GetZDO().SetPGWVersion(ZoneSystem.instance.m_pgwVersion);
				}
				if (mode == ZoneSystem.SpawnMode.Ghost)
				{
					UnityEngine.Object.Destroy(gameObject);
				}
			}
		}
	}
	
	//for (ZNetView znetView3 in room.m_netViews)
	//{
	//	znetView3.gameObject.SetActive(false);
	//}

	Room component2 = UnityEngine.Object.Instantiate<GameObject>(room.m_room.gameObject, pos, rot, base.transform).GetComponent<Room>();
	component2.gameObject.name = room.m_room.gameObject.name;
	if (mode != ZoneSystem.SpawnMode.Client)
	{
		component2.m_placeOrder = (fromConnection ? (fromConnection.m_placeOrder + 1) : 0);
		component2.m_seed = seed;
		DungeonGenerator.m_placedRooms.Add(component2);
		this->AddOpenConnections(component2, fromConnection);
	}
	UnityEngine.Random.state = state;
}

void DungeonGenerator::AddOpenConnections(Room newRoom, RoomConnection skipConnection) {
	RoomConnection[] connections = newRoom.GetConnections();
	if (skipConnection != null)
	{
		foreach(RoomConnection roomConnection in connections)
		{
			if (!roomConnection.m_entrance && Vector3.Distance(roomConnection.transform.position, skipConnection.transform.position) >= 0.1f)
			{
				roomConnection.m_placeOrder = newRoom.m_placeOrder;
				DungeonGenerator.m_openConnections.Add(roomConnection);
			}
		}
		return;
	}
	RoomConnection[] array = connections;
	for (int i = 0; i < array.Length; i++)
	{
		array[i].m_placeOrder = newRoom.m_placeOrder;
	}
	DungeonGenerator.m_openConnections.AddRange(connections);
}

void DungeonGenerator::SetupColliders() {
	if (this->m_colliderA != null)
	{
		return;
	}
	BoxCollider[] componentsInChildren = base.gameObject.GetComponentsInChildren<BoxCollider>();
	for (int i = 0; i < componentsInChildren.Length; i++)
	{
		UnityEngine.Object.DestroyImmediate(componentsInChildren[i]);
	}
	this->m_colliderA = base.gameObject.AddComponent<BoxCollider>();
	this->m_colliderB = base.gameObject.AddComponent<BoxCollider>();
}

bool DungeonGenerator::IsInsideDungeon(Room room, Vector3 pos, Quaternion rot)
{
	Bounds bounds = new Bounds(this->m_zoneCenter, this->m_zoneSize);
	Vector3 vector = room.m_size;
	vector *= 0.5f;
	return bounds.Contains(pos + rot * Vector3(vector.x, vector.y, -vector.z)) && bounds.Contains(pos + rot * new Vector3(-vector.x, vector.y, -vector.z)) && bounds.Contains(pos + rot * new Vector3(vector.x, vector.y, vector.z)) && bounds.Contains(pos + rot * new Vector3(-vector.x, vector.y, vector.z)) && bounds.Contains(pos + rot * new Vector3(vector.x, -vector.y, -vector.z)) && bounds.Contains(pos + rot * new Vector3(-vector.x, -vector.y, -vector.z)) && bounds.Contains(pos + rot * new Vector3(vector.x, -vector.y, vector.z)) && bounds.Contains(pos + rot * new Vector3(-vector.x, -vector.y, vector.z));
}

bool DungeonGenerator::TestCollision(Room room, Vector3 pos, Quaternion rot)
{
	if (!this->IsInsideDungeon(room, pos, rot))
	{
		return true;
	}
	this->m_colliderA.size = new Vector3((float)room.m_size.x - 0.1f, (float)room.m_size.y - 0.1f, (float)room.m_size.z - 0.1f);
	for (auto&& room2 : m_placedRooms)
	{
		this->m_colliderB.size = room2.m_size;
		Vector3 vector;
		float num;
		if (Physics.ComputePenetration(this->m_colliderA, pos, rot, this->m_colliderB, room2.transform.position, room2.transform.rotation, out vector, out num))
		{
			return true;
		}
	}
	return false;
}

RoomData* DungeonGenerator::GetRandomWeightedRoom(VUtils::Random::State& state, bool perimeterRoom)
{
	m_tempRooms.clear();
	float num = 0;
	for (auto&& roomData : m_availableRooms)
	{
		if (!roomData.m_room.m_entrance && !roomData.m_room.m_endCap && !roomData.m_room.m_divider && roomData.m_room.m_perimeter == perimeterRoom)
		{
			num += roomData.m_room.m_weight;
			m_tempRooms.Add(roomData);
		}
	}
	if (m_tempRooms.empty())
	{
		return null;
	}
	float num2 = state.Range(0.f, num);
	float num3 = 0;
	for (auto&& roomData2 : m_tempRooms)
	{
		num3 += roomData2.m_room.m_weight;
		if (num2 <= num3)
		{
			return roomData2;
		}
	}
	return m_tempRooms[0];
}

RoomData* DungeonGenerator::GetRandomWeightedRoom(RoomConnection connection)
{
	m_tempRooms.clear();
	for (auto&& roomData : m_availableRooms)
	{
		if (!roomData.m_room.m_entrance 
			&& !roomData.m_room.m_endCap 
			&& !roomData.m_room.m_divider 
			&& (!connection || (roomData.m_room.HaveConnection(connection) && connection.m_placeOrder >= roomData.m_room.m_minPlaceOrder)))
		{
			m_tempRooms.Add(roomData);
		}
	}
	if (m_tempRooms.empty())
	{
		return null;
	}
	return this->GetWeightedRoom(m_tempRooms);
}

RoomData* DungeonGenerator::GetWeightedRoom(VUtils::Random::State& state, std::vector<RoomData*> rooms)
{
	float num = 0;
	for (auto&& roomData : rooms)
	{
		num += roomData.m_room.m_weight;
	}
	float num2 = state.Range(0.f, num);
	float num3 = 0;
	for (auto&& roomData2 : rooms)
	{
		num3 += roomData2.m_room.m_weight;
		if (num2 <= num3)
		{
			return roomData2;
		}
	}
	return m_tempRooms[0];
}

RoomData* DungeonGenerator::GetRandomRoom(VUtils::Random::State& state, RoomConnection connection)
{
	DungeonGenerator.m_tempRooms.Clear();
	for (auto&& roomData : m_availableRooms)
	{
		if (!roomData.m_room.m_entrance 
			&& !roomData.m_room.m_endCap 
			&& !roomData.m_room.m_divider 
			&& (!connection || (roomData.m_room.HaveConnection(connection) && connection.m_placeOrder >= roomData.m_room.m_minPlaceOrder)))
		{
			m_tempRooms.push_back(roomData);
		}
	}
	if (m_tempRooms.empty())
	{
		return null;
	}
	return m_tempRooms[state.Range(0, m_tempRooms.size())];
}

RoomConnection DungeonGenerator::GetOpenConnection(VUtils::Random::State& state)
{
	if (m_openConnections.empty())
	{
		return null;
	}
	return m_openConnections[state.Range(0, m_openConnections.size())];
}

RoomData* DungeonGenerator::FindStartRoom(VUtils::Random::State& state)
{
	m_tempRooms.clear();
	for (auto&& roomData : m_availableRooms)
	{
		if (roomData.m_room.m_entrance)
		{
			m_tempRooms.push_back(roomData);
		}
	}
	return m_tempRooms[state.Range(0, m_tempRooms.size())];
}

bool DungeonGenerator::CheckRequiredRooms()
{
	if (this->m_minRequiredRooms == 0 || this->m_requiredRooms.empty())
	{
		return false;
	}
	int num = 0;
	for (auto&& room : m_placedRooms)
	{
		if (this->m_requiredRooms.Contains(room.gameObject.name))
		{
			num++;
		}
	}
	return num >= this->m_minRequiredRooms;
}
