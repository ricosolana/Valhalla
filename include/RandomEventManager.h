#pragma once

#include "VUtils.h"

#if VH_IS_ON(VH_RANDOM_EVENTS)
#include "VUtilsString.h"
#include "DataReader.h"
#include "DataWriter.h"
#include "Types.h"
#include "RandomEventManager.h"
#include "HashUtils.h"

class IRandomEventManager {
	class Event {
	public:
		std::string m_name = "";
		float m_duration = 60;
		bool m_nearBaseOnly = true;
		bool m_pauseIfNoPlayerInArea = true;
		Biome m_biome;

		UNORDERED_SET_t<std::string, ankerl::unordered_dense::string_hash, std::equal_to<>> m_presentGlobalKeys;
		UNORDERED_SET_t<std::string, ankerl::unordered_dense::string_hash, std::equal_to<>> m_absentGlobalKeys;
	};

public:
	UNORDERED_MAP_t<HASH_t, std::unique_ptr<Event>> m_events;

	// Event timer for chance of next event
	float m_eventIntervalTimer = 0;

private:
	// The current random active event in the world
	//	can be null if no event is active
	const Event *m_activeEvent = nullptr;
	Vector3f m_activeEventPos;
	float m_activeEventTimer = 0;

private:
	void SendCurrentRandomEvent();

	std::optional<std::pair<std::reference_wrapper<const Event>, Vector3f>> GetPossibleRandomEvent();

	// Checks whether the server has or doesnt have
	//	the global keys requested of the event
	bool CheckGlobalKeys(const Event& e);

	// Get an event by name
	//	Returns null if not found
	const Event* GetEvent(std::string_view name) {
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

IRandomEventManager *RandomEventManager();
#endif