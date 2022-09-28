#include "ValhallaServer.h"
#include "ScriptManager.h"
#include "ResourceManager.h"

std::unique_ptr<ValhallaServer> VALHALLA_SERVER_INSTANCE(std::make_unique<ValhallaServer>());
ValhallaServer* Valhalla() {
	//if (!VALHALLA_SERVER_INSTANCE)
		//VALHALLA_SERVER_INSTANCE = std::make_unique<ValhallaServer>();
	return VALHALLA_SERVER_INSTANCE.get();
}



void ValhallaServer::Launch() {
	//assert(!VALHALLA_SERVER_INSTANCE && "Tried launching another server instance!");
	assert(!m_running && "Tried calling Launch() twice!");
	assert(m_serverPassword.empty() && "Must implement password salting feature (reminder)");

	ResourceManager::SetRoot("./data/");
	//ScriptManager::Init();
	m_znet = std::make_unique<ZNet>(2456);
	m_znet->Listen();

	m_running = true;
	while (m_running) {
		auto now = std::chrono::steady_clock::now();
		static auto last_tick = now; // Initialized to this once
		auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - last_tick).count();
		last_tick = now;

		// mutex is carefully scoped in this micro-scope
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

		// This is temporary to not spin lock
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void ValhallaServer::Terminate() {
	//m_znet->Disconnect();
	m_running = false;
	ScriptManager::Uninit();
}

void ValhallaServer::Update(float delta) {
	// This is important to processing RPC remote invocations

	m_znet->Update();

	//ScriptManager::Event::OnUpdate(delta);
}



Task* ValhallaServer::RunTask(Task::F f) {
	return RunTaskLater(f, 0ms);
}

Task* ValhallaServer::RunTaskLater(Task::F f, std::chrono::milliseconds after) {
	return RunTaskLaterRepeat(f, after, 0ms);
}

Task* ValhallaServer::RunTaskAt(Task::F f, std::chrono::steady_clock::time_point at) {
	return RunTaskAtRepeat(f, at, 0ms);
}

Task* ValhallaServer::RunTaskLaterRepeat(Task::F f, std::chrono::milliseconds after, std::chrono::milliseconds period) {
	return RunTaskAtRepeat(f, std::chrono::steady_clock::now() + after, period);
}

Task* ValhallaServer::RunTaskAtRepeat(Task::F f, std::chrono::steady_clock::time_point at, std::chrono::milliseconds period) {
	std::scoped_lock lock(m_taskMutex);
	Task* task = new Task{f, at, period};
	m_tasks.push_back(std::unique_ptr<Task>(task));
	return task;
}
