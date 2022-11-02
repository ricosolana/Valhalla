#pragma once

#include <thread>
#include "Task.h"
#include "ServerSettings.h"
#include "NetAcceptor.h"

#define SERVER_ID Valhalla()->ID()

class ValhallaServer {
	std::atomic_bool m_running = false;

	// perfect structure for this job
	// https://stackoverflow.com/questions/2209224/vector-vs-list-in-stl
	std::list<std::unique_ptr<Task>> m_tasks;

	std::recursive_mutex m_taskMutex;

    ServerSettings m_settings;
	const OWNER_t m_serverId; // generated at start
    std::unique_ptr<RCONAcceptor> m_rcon;
    robin_hood::unordered_map<int32_t, RCONSocket*> m_rconSockets;

	steady_clock::time_point m_startTime;
	steady_clock::time_point m_prevUpdate;
	steady_clock::time_point m_nowUpdate;

public:
    ValhallaServer() : m_serverId(Utils::GenerateUID()) {}

	OWNER_t ID() const {
		return m_serverId;
	}

    const ServerSettings& Settings() const {
        return m_settings;
    }

	robin_hood::unordered_set<std::string> m_banned;

	void Launch();
	void Terminate();

	// Return the time in nanoseconds
	auto Nanos() {
		//return m_nanos;
		return m_startTime - m_nowUpdate;
	}

	// Returns the time in Ticks (C# DateTime.Ticks)
	int64_t Ticks() {
		return Nanos().count() / 100;
	}

	// Returns the time in seconds (Unity Time.time)
	float Time() {
		return float((double)Nanos().count() / (double)duration_cast<nanoseconds>(1s).count());
	}

	float Delta() {
		auto elapsed = m_nowUpdate - m_prevUpdate;
		return (double)elapsed.count() / (double)duration_cast<decltype(elapsed)>(1s).count();
	}

	Task* RunTask(Task::F f);
	Task* RunTaskLater(Task::F f, std::chrono::milliseconds after);
	Task* RunTaskAt(Task::F f, std::chrono::steady_clock::time_point at);
	Task* RunTaskRepeat(Task::F f, std::chrono::milliseconds period);
	Task* RunTaskLaterRepeat(Task::F f, std::chrono::milliseconds after, std::chrono::milliseconds period);
	Task* RunTaskAtRepeat(Task::F f, std::chrono::steady_clock::time_point at, std::chrono::milliseconds period);

private:
	void Update(float delta);
};

ValhallaServer* Valhalla();
