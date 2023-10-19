#pragma once

#include "VUtils.h"
#include "DataReader.h"
#include "DataWriter.h"
#include "Types.h"
#include "RandomEventManager.h"

class IRandomEventManager {
public:
	class Event {
	public:
		std::string m_name;
		//float m_duration = 60;
		nanoseconds m_duration{};
		bool m_nearBaseOnly{};
		bool m_pauseIfNoPlayerInArea{};
		Biome m_biome{};

		UNORDERED_SET_t<std::string, ankerl::unordered_dense::string_hash, std::equal_to<>> m_presentGlobalKeys;
		UNORDERED_SET_t<std::string, ankerl::unordered_dense::string_hash, std::equal_to<>> m_absentGlobalKeys;

		Event() {}
	};

public:
	UNORDERED_MAP_t<std::string_view, std::unique_ptr<Event>> m_events;

	// Event timer for next event attempt
	float m_eventIntervalTimer = 0;

private:
	// The current random active event in the world
	//	null means no event is active
	const Event* m_activeEvent = nullptr;
	Vector3f m_activeEventPos;
	nanoseconds m_activeEventRemaining;
	nanoseconds m_activeEventInitialDuration;

private:
	void SendCurrentRandomEvent();

	std::optional<std::pair<std::reference_wrapper<const Event>, Vector3f>> GetPossibleRandomEvent();

	// Checks whether the server has or doesnt have
	//	the global keys requested of the event
	bool CheckGlobalKeys(const Event& e);

public:
	void init();
	void Update();

	void SetCurrentRandomEvent(const Event& e, Vector3f pos, nanoseconds ns);

	// Get an event by name
	//	Returns null if not found
	const Event* GetEvent(std::string_view name) {
		auto&& find = m_events.find(name);
		if (find != m_events.end())
			return find->second.get();
		return nullptr;
	}

	void save(DataWriter& writer);
	void Load(DataReader& reader, int version);
};

IRandomEventManager* RandomEventManager();
