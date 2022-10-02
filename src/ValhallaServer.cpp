#include "ValhallaServer.h"
#include "ModManager.h"
#include "ResourceManager.h"

using namespace std::chrono;

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

	m_serverName = "lorem ipsum";
	m_serverUuid = Utils::GenerateUID();

	ResourceManager::SetRoot("./data/");
	ModManager::Init();
	//m_znet = std::make_unique<ZNet>(2456);
	//m_znet->Listen();
	NetManager::Listen(2456);



	m_running = true;
	while (m_running) {
		auto now = steady_clock::now();
		static auto last_tick = now; // Initialized to this once
		auto elapsed = std::chrono::duration_cast<microseconds>(now - last_tick).count();
		last_tick = now;

		// mutex is carefully scoped in this micro-scope
		{
			std::scoped_lock lock(m_taskMutex);
			const auto now = steady_clock::now();
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
		Update(elapsed / (double)duration_cast<microseconds>(1s).count());

		// Spin lock prevention
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void ValhallaServer::Terminate() {
	LOG(INFO) << "Terminating server";

	m_running = false;
	ModManager::Uninit();
	NetManager::Close();
}

void ValhallaServer::Update(float delta) {
	// This is important to processing RPC remote invocations

	NetManager::Update(delta);

	ModManager::Event::OnUpdate(delta);
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
