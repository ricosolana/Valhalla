#pragma once

#include <thread>
#include "Task.h"
#include "NetManager.h"

class ValhallaServer {
	std::atomic_bool m_running = false;

	// perfect structure for this job
	// https://stackoverflow.com/questions/2209224/vector-vs-list-in-stl
	std::list<std::unique_ptr<Task>> m_tasks;

	std::recursive_mutex m_taskMutex;

	std::string m_serverName;
	int m_maxPeers;
	UUID_t m_serverUuid;
	uint64_t m_nS = 0; // time in uS
	//std::chrono::steady_clock::time_point start;
	//float m_time = 0;

public:
	UUID_t Uuid() const {
		return m_serverUuid;
	}

	//std::string m_serverPassword;
	robin_hood::unordered_set<std::string> m_banned;

	void Launch();
	void Terminate();

	// Return the time in nanoseconds
	uint64_t Nanos() {
		return m_nS;
	}

	// According to C# DateTime.Ticks
	uint64_t Ticks() {
		return m_nS / 100;
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
