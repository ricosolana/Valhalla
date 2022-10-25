#include <optick.h>
//#include <ryml/ryml.hpp>
//#include <c4/format.hpp>
//#include <ryml/ryml_std.hpp>
#include <yaml-cpp/yaml.h>

#include "ValhallaServer.h"
#include "ModManager.h"
#include "ResourceManager.h"
#include "ServerSettings.h"

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

	//start = steady_clock::now();

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

	ServerSettings settings;

	settings.serverName =				loadNode["server-name"].as<std::string>("My server");
	settings.serverPort =				loadNode["server-port"].as<uint16_t>(2456);
	settings.serverPassword =			loadNode["server-password"].as<std::string>("secret");
	settings.serverPublic =				loadNode["server-public"].as<bool>(false);

	settings.worldName =				loadNode["world-name"].as<std::string>("Dedicated world");
	settings.worldName =				loadNode["world-seed-name"].as<std::string>("Some special seed");
	settings.worldSeed =				Utils::GetStableHashCode(settings.worldSeedName);

	settings.playerWhitelist =			loadNode["player-whitelist"].as<bool>(false);		// enable whitelist
	settings.playerMax =				loadNode["player-max"].as<unsigned int>(64);		// max allowed players
	settings.playerAuth =				loadNode["player-auth"].as<bool>(true);				// allow authed players only
	settings.playerList =				loadNode["player-list"].as<bool>(true);				// does not send playerlist to players
	settings.playerArrivePing =			loadNode["player-arrive-ping"].as<bool>(true);		// prevent player join ping

	settings.socketTimeout =			loadNode["socket-timeout"].as<float>(30000);		// player timeout in milliseconds
	settings.socketCongestion =			loadNode["socket-congestion"].as<int32_t>(10240);



	YAML::Node saveNode;

	saveNode["server-name"] =			settings.serverName;
	saveNode["server-port"] =			settings.serverPort;
	saveNode["server-password"] =		settings.serverPassword;
	saveNode["server-public"] =			settings.serverPublic;

	saveNode["world-name"] =			settings.worldName;
	saveNode["world-seed"] =			settings.worldSeed;

	saveNode["player-whitelist"] =		settings.playerWhitelist;
	saveNode["player-max"] =			settings.playerMax;
	saveNode["player-auth"] =			settings.playerAuth;
	saveNode["player-list"] =			settings.playerList;
	saveNode["player-arrive-ping"] =	settings.playerArrivePing;

	saveNode["socket-timeout"] =		settings.socketTimeout;
	saveNode["socket-congestion"] =		settings.socketCongestion;

	YAML::Emitter out;
	out.SetIndent(4);
	out << saveNode;

	ResourceManager::WriteFileBytes("server.yml", out.c_str());

	LOG(INFO) << "Server config loaded";
	
	// have other settings like chat spam prevention, fly prevention, advanced permission settings
	
	m_serverUuid = Utils::GenerateUID();

	ModManager::Init();
	NetManager::Start(settings);



	m_running = true;
	while (m_running) {
		OPTICK_FRAME("main");

		auto now = steady_clock::now();
		static auto last_tick = now; // Initialized to this once
		auto elapsed = duration_cast<nanoseconds>(now - last_tick); // .count();
		last_tick = now;

		// mutex is carefully scoped in this micro-scope
		{
			std::scoped_lock lock(m_taskMutex);
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

		// now inc uS
		m_nS += elapsed.count();
		
		// UPDATE
		float delta = elapsed.count() / (double)duration_cast<decltype(elapsed)>(1s).count();
		Update(delta);

		// Prevent spin lock
		// TODO
		//	this could be adjusted in order to accomodate 
		//	more resource intensive workloads
		std::this_thread::sleep_for(1ms);
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

Task* ValhallaServer::RunTaskLater(Task::F f, milliseconds after) {
	return RunTaskLaterRepeat(f, after, 0ms);
}

Task* ValhallaServer::RunTaskAt(Task::F f, steady_clock::time_point at) {
	return RunTaskAtRepeat(f, at, 0ms);
}

Task* ValhallaServer::RunTaskRepeat(Task::F f, milliseconds period) {
	return RunTaskLaterRepeat(f, 0ms, period);
}

Task* ValhallaServer::RunTaskLaterRepeat(Task::F f, milliseconds after, milliseconds period) {
	return RunTaskAtRepeat(f, steady_clock::now() + after, period);
}

Task* ValhallaServer::RunTaskAtRepeat(Task::F f, steady_clock::time_point at, milliseconds period) {
	std::scoped_lock lock(m_taskMutex);
	Task* task = new Task{f, at, period};
	m_tasks.push_back(std::unique_ptr<Task>(task));
	return task;
}
