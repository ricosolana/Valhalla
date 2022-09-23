#include "Game.hpp"
#include "ScriptManager.hpp"

std::unique_ptr<Game> GAME;
Game *Game::Get() {
	return GAME.get();
}

void Game::Run() {
	GAME = std::make_unique<Game>();
	GAME->Start();
}



void Game::Start() {
	ScriptManager::Init();
	m_znet = std::make_unique<ZNet>();

	m_running = true;

	while (m_running) {
		auto now = std::chrono::steady_clock::now();
		static auto last_tick = now; // Initialized to this once
		auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - last_tick).count();
		last_tick = now;

		{
			std::scoped_lock lock(m_taskMutex);
			const auto now = std::chrono::steady_clock::now();
			for (auto itr = m_tasks.begin(); itr != m_tasks.end();) {
				auto ptr = itr->get();
				if (ptr->at < now) {
					if (ptr->period < 0ms) {
						itr = m_tasks.erase(itr);
					}
					else {
						ptr->function(ptr);
						if (ptr->Repeats()) {
							ptr->at += ptr->period;
							++itr;
						}
						else
							itr = m_tasks.erase(itr);
					}
				}
				else
					++itr;
			}
		}

		// UPDATE
		Update(elapsed / 1000000.f);

		// could instead create a performance analyzer that will
		// not delay when tps is low
		// Removing this will make cpu usage go to a lot more
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void Game::Stop() {
	m_znet->Disconnect();
	m_running = false;
	ScriptManager::Uninit();
}

void Game::Update(float delta) {
	// This is important to processing RPC remote invocations

	m_znet->Update();

	ScriptManager::Event::OnUpdate(delta);
}



Task* Game::RunTask(Task::F f) {
	return RunTaskLater(f, 0ms);
}

Task* Game::RunTaskLater(Task::F f, std::chrono::milliseconds after) {
	return RunTaskLaterRepeat(f, after, 0ms);
}

Task* Game::RunTaskAt(Task::F f, std::chrono::steady_clock::time_point at) {
	return RunTaskAtRepeat(f, at, 0ms);
}

Task* Game::RunTaskLaterRepeat(Task::F f, std::chrono::milliseconds after, std::chrono::milliseconds period) {
	return RunTaskAtRepeat(f, std::chrono::steady_clock::now() + after, period);
}

Task* Game::RunTaskAtRepeat(Task::F f, std::chrono::steady_clock::time_point at, std::chrono::milliseconds period) {
	std::scoped_lock lock(m_taskMutex);
	Task* task = new Task{f, at, period};
	m_tasks.push_back(std::unique_ptr<Task>(task));
	return task;
}
