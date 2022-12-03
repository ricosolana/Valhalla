#pragma once

#include <thread>
#include "Task.h"
#include "ServerSettings.h"
#include "NetAcceptor.h"
#include "VUtilsRandom.h"

#define SERVER_ID Valhalla()->ID()
#define SERVER_SETTINGS Valhalla()->Settings()

class NetRpc;

class ValhallaServer {
private:
	static std::atomic_bool m_running;

	static std::list<std::unique_ptr<Task>> m_tasks;
	static std::recursive_mutex m_taskMutex;

    static ServerSettings m_settings;
	static const OWNER_t m_serverID; // generated at start

    static std::unique_ptr<RCONAcceptor> m_rcon;
	static std::list<std::shared_ptr<RCONSocket>> m_rconSockets;

	static const steady_clock::time_point m_startTime;
	static steady_clock::time_point m_prevUpdate;
	static steady_clock::time_point m_nowUpdate;

	static double m_netTime;

private:
	static void LoadFiles();

public:

	static OWNER_t ID() const {
		return m_serverID;
	}

	static const ServerSettings& Settings() const {
        return m_settings;
    }

	robin_hood::unordered_set<std::string> m_banned;

	void Init();
	void UnInit();

	// Get the time in nanoseconds
	auto Nanos() {
		return m_startTime - m_nowUpdate;
	}

	// Get the time in Ticks (C# DateTime.Ticks)
	int64_t Ticks() {
		return Nanos().count() / 100;
	}

	// Get the time in seconds (Unity Time.time)
	float Time() {
		return float((double)Nanos().count() / (double)duration_cast<nanoseconds>(1s).count());
	}

	// Get the elapsed time since last frame in seconds
	float Delta() {
		auto elapsed = m_nowUpdate - m_prevUpdate;
		return (double)elapsed.count() / (double)duration_cast<decltype(elapsed)>(1s).count();
	}

	double NetTime() {
		// returns active server time
		// server is frozen as long as no players are online
		return m_netTime;
	}
	
	static bool IsPeerAllowed(NetRpc* rpc);

	Task& RunTask(Task::F f);
	Task& RunTaskLater(Task::F f, std::chrono::milliseconds after);
	Task& RunTaskAt(Task::F f, std::chrono::steady_clock::time_point at);
	Task& RunTaskRepeat(Task::F f, std::chrono::milliseconds period);
	Task& RunTaskLaterRepeat(Task::F f, std::chrono::milliseconds after, std::chrono::milliseconds period);
	Task& RunTaskAtRepeat(Task::F f, std::chrono::steady_clock::time_point at, std::chrono::milliseconds period);

private:
	void Update();
};

VServer* Valhalla();
