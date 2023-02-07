#pragma once

#include "VUtils.h"
#include "DataReader.h"
#include "DataWriter.h"
#include "Biome.h"
#include "robin_hood.h"

class IEventManager {
	class Event {
	private:
		bool m_firstActivation = true;

		bool m_active;
	public:
		std::string m_name = "";

		// dumb
		//bool m_enabled = true;

		// redundant? arent all events random?
		bool m_random = true;

		float m_duration = 60;

		bool m_nearBaseOnly = true;

		bool m_pauseIfNoPlayerInArea = true;

		//[BitMask(typeof(Heightmap.Biome))]
		Biome m_biome;

		//[Header("( Keys required to be TRUE )")]
		// could use hashes
		robin_hood::unordered_set<std::string> m_requiredGlobalKeys;

		//[Header("( Keys required to be FALSE )")]
		// could use hashes
		robin_hood::unordered_set<std::string> m_notRequiredGlobalKeys;

		//[Space(20f)]
		//std::string m_startMessage = "";

		//std::string m_endMessage = "";

		//std::string m_forceMusic = "";

		//std::string m_forceEnvironment = "";

		// Used by client ZoneCtrl.SpawnSystem
		//std::vector<SpawnSystem.SpawnData> m_spawn;
	};

public:
	// add configuration in server settings

	float m_eventIntervalMin = 1;

	float m_eventChance = 25;

	float m_randomEventRange = 200;

	float m_eventTimer = 0;

	float m_sendTimer = 0;

	std::vector<std::unique_ptr<Event>> m_events;

private:
	Event *m_randomEvent = nullptr;
	float m_forcedEventUpdateTimer = 0;
	Event *m_forcedEvent = nullptr;
	Event *m_activeEvent = nullptr;

	//float m_tempSaveEventTimer;
	//std::string m_tempSaveRandomEvent;
	//float m_tempSaveRandomEventTime;
	//Vector3 m_tempSaveRandomEventPos;

	Vector3 m_activeEventPos; // location of current event
	float m_activeEventTimer = 0; //  running time of current event

private:
	void SendCurrentRandomEvent();

public:
    void Update();

    void Save(DataWriter& writer);
    void Load(DataReader& reader, int version);
};

IEventManager EventManager();
