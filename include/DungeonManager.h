#pragma once

#include <robin_hood.h>

#include "VUtils.h"
#include "Prefab.h"
#include "Room.h"

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
	std::vector<DoorDef> m_doorTypes; // Serialized

	float m_doorChance = 0.5f;

	float m_maxTilt = 10;

	float m_tileWidth = 8;

	int m_gridSize = 4;

	float m_spawnChance = 1;

	float m_campRadiusMin = 15;

	float m_campRadiusMax = 30;

	float m_minAltitude = 1;

	int m_perimeterSections;

	float m_perimeterBuffer = 2;

	bool m_useCustomInteriorTransform;

	Vector3 m_originalPosition;

	// Order is significant (polled with Seeded Random)
	std::vector<std::unique_ptr<Room>> m_availableRooms;
};

class IDungeonManager {
	robin_hood::unordered_map<HASH_t, std::unique_ptr<Dungeon>> m_dungeons;

public:
    void Init();

};

IDungeonManager* DungeonManager();
