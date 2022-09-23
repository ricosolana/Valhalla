#pragma once

#include <thread>
#include <asio.hpp>
#include "Task.hpp"
#include "ZNetPeer.hpp"
#include "ZNet.hpp"
#include "Settings.hpp"
#include "PlayerProfile.hpp"

using namespace asio::ip;

class Game {
	bool m_running = false;

	// perfect structure for this job
	// https://stackoverflow.com/questions/2209224/vector-vs-list-in-stl
	std::list<std::unique_ptr<Task>> m_tasks;

	std::recursive_mutex m_taskMutex;

	Settings m_settings;

public:
	static constexpr const char* VERSION = "0.202.19";
	std::unique_ptr<ZNet> m_znet;
	std::unique_ptr<PlayerProfile> m_playerProfile;

	static Game *Get();
	static void Run();

	void Stop();

	Task* RunTask(Task::F f);
	Task* RunTaskLater(Task::F f, std::chrono::milliseconds after);
	Task* RunTaskAt(Task::F f, std::chrono::steady_clock::time_point at);
	Task* RunTaskLaterRepeat(Task::F f, std::chrono::milliseconds after, std::chrono::milliseconds period);
	Task* RunTaskAtRepeat(Task::F f, std::chrono::steady_clock::time_point at, std::chrono::milliseconds period);

private:
	void Start();

	void Update(float delta);
};
