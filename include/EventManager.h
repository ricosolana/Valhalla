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
		//bool m_random = true;

		float m_duration = 60;

		bool m_nearBaseOnly = true;

		bool m_pauseIfNoPlayerInArea = true;

		//[BitMask(typeof(Heightmap.Biome))]
		Biome m_biome;

		//[Header("( Keys required to be TRUE )")]
		// could use hashes
		robin_hood::unordered_set<std::string> m_presentGlobalKeys;

		//[Header("( Keys required to be FALSE )")]
		// could use hashes
		robin_hood::unordered_set<std::string> m_absentGlobalKeys;

		//[Space(20f)]
		//std::string m_startMessage = "";

		//std::string m_endMessage = "";

		//std::string m_forceMusic = "";

		//std::string m_forceEnvironment = "";

		// Used by client ZoneCtrl.SpawnSystem
		//std::vector<SpawnSystem.SpawnData> m_spawn;
	};

public:
	robin_hood::unordered_map<HASH_t, std::unique_ptr<Event>> m_events;

	// Event timer for chance of next event
	float m_eventIntervalTimer = 0;

private:
	// The current random active event in the world
	//	can be null if no event is active
	const Event *m_activeEvent = nullptr;
	Vector3 m_activeEventPos;
	float m_activeEventTimer = 0;

private:
	void SendCurrentRandomEvent();

	std::optional<std::pair<std::reference_wrapper<const Event>, Vector3>> GetPossibleRandomEvent();

	// Checks whether the server has or doesnt have
	//	the global keys requested of the event
	bool CheckGlobalKeys(const Event& e);

	// Get an event by name
	//	Returns null if not found
	const Event* GetEvent(const std::string& name) {
		auto&& find = m_events.find(VUtils::String::GetStableHashCode(name));
		if (find != m_events.end())
			return find->second.get();
		return nullptr;
	}

public:
	void Init();
    void Update();

    void Save(DataWriter& writer);
    void Load(DataReader& reader, int version);
};

IEventManager *EventManager();
