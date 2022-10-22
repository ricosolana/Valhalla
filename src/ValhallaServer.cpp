#include <optick.h>
//#include <ryml/ryml.hpp>
//#include <c4/format.hpp>
//#include <ryml/ryml_std.hpp>
#include <yaml-cpp/yaml.h>

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
	//assert(m_serverPassword.empty() && "Must implement password salting feature (reminder)");

	YAML::Node loadNode;
	{
		std::string buf;
		if (ResourceManager::ReadFileBytes("server.yml", buf)) {
			try {
				loadNode = YAML::Load(buf);
			}
			catch (YAML::ParserException& e) {
				LOG(INFO) << e.what();
			}
		} 
		else
			LOG(INFO) << "Server config not found, creating...";
	}

	auto serverName =					loadNode["server-name"].as<std::string>("My server");
	auto serverPort =					loadNode["server-port"].as<uint16_t>(2456);
	auto serverPassword =				loadNode["server-password"].as<std::string>("secret");
	auto serverPublic =					loadNode["server-public"].as<bool>(false);

	auto worldName =					loadNode["world-name"].as<std::string>("Dedicated world");
	auto worldSeed =					loadNode["world-seed"].as<int32_t>(123456789);

	auto playerWhitelist =				loadNode["player-whitelist"].as<bool>(false);		// enable whitelist
	auto playerTimeout =				loadNode["player-timeout"].as<float>(30000);		// player timeout in milliseconds
	auto playerMax =					loadNode["player-max"].as<unsigned int>(64);		// max allowed players
	auto playerAuth =					loadNode["player-auth"].as<bool>(true);				// allow authed players only
	auto playerList =					loadNode["player-list"].as<bool>(true);				// does not send playerlist to players
	auto playerArrivePing =				loadNode["player-arrive-ping"].as<bool>(true);		// prevent player join ping
	
	YAML::Node saveNode;

	saveNode["server-name"] =			serverName;
	saveNode["server-port"] =			serverPort;
	saveNode["server-password"] =		serverPassword;
	saveNode["server-public"] =			serverPublic;

	saveNode["world-name"] =			worldName;
	saveNode["world-seed"] =			worldSeed;

	saveNode["player-whitelist"] =		playerWhitelist;
	saveNode["player-timeout"] =		playerTimeout;
	saveNode["player-max"] =			playerMax;
	saveNode["player-auth"] =			playerAuth;
	saveNode["player-list"] =			playerList;
	saveNode["player-arrive-ping"] =	playerArrivePing;

	if (saveNode != loadNode) {
		YAML::Emitter out;
		out.SetIndent(4);
		out << saveNode;

		assert(out.good());

		ResourceManager::WriteFileBytes("server.yml", out.c_str());

		LOG(INFO) << "Patched server config";
	}
	else {
		LOG(INFO) << "Server config loaded";
	}



	// have other settings like chat spam prevention, fly prevention, advanced permission settings
	
	m_serverUuid = Utils::GenerateUID();

	ModManager::Init();
	NetManager::Start(serverName, serverPassword, serverPort, serverPublic, playerTimeout);



	m_running = true;
	while (m_running) {
		OPTICK_FRAME("main");

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

		// Prevent spin lock
		// TODO
		//	this could be adjusted in order to accomodate 
		//	more resource intensive workloads
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

Task* ValhallaServer::RunTaskRepeat(Task::F f, std::chrono::milliseconds period) {
	return RunTaskLaterRepeat(f, 0ms, period);
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
