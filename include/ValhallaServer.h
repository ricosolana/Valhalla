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

public:
	std::string m_serverName;
	std::string m_serverPassword;
	UUID_t m_serverUuid;
	int m_maxPeers;
	robin_hood::unordered_set<std::string> m_banned;

	void Launch();
	void Terminate();

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
