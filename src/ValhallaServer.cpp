#include <optick.h>
#include <yaml-cpp/yaml.h>

#include "ValhallaServer.h"
#include "ModManager.h"
#include "ResourceManager.h"
#include "ServerSettings.h"
#include "NetManager.h"
#include "ChatManager.h"

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

	m_settings.serverName =				loadNode["server-name"].as<std::string>("My server");
	m_settings.serverPort =				loadNode["server-port"].as<uint16_t>(2456);
	m_settings.serverPassword =			loadNode["server-password"].as<std::string>("secret");
	m_settings.serverPublic =			loadNode["server-public"].as<bool>(false);

	m_settings.worldName =				loadNode["world-name"].as<std::string>("Dedicated world");
    m_settings.worldSeedName =			loadNode["world-seed-name"].as<std::string>("Some special seed");
	m_settings.worldSeed =				Utils::GetStableHashCode(m_settings.worldSeedName);

	m_settings.playerWhitelist =		loadNode["player-whitelist"].as<bool>(false);		// enable whitelist
	m_settings.playerMax =				loadNode["player-max"].as<unsigned int>(64);		// max allowed players
	m_settings.playerAuth =				loadNode["player-auth"].as<bool>(true);				// allow authed players only
	m_settings.playerList =				loadNode["player-list"].as<bool>(true);				// does not send player list to players
	m_settings.playerArrivePing =		loadNode["player-arrive-ping"].as<bool>(true);		// prevent player join ping

	m_settings.socketTimeout =		    milliseconds(loadNode["socket-timeout"].as<unsigned int>(30000)); // player timeout in milliseconds
	m_settings.socketMaxCongestion =	loadNode["socket-max-congestion"].as<int32_t>(10240);
    m_settings.socketMinCongestion =	loadNode["socket-min-congestion"].as<int32_t>(2048);

    m_settings.rconEnabled =            loadNode["rcon-enabled"].as<bool>(false);
    m_settings.rconPort =               loadNode["rcon-port"].as<uint16_t>(25575);
    m_settings.rconPassword =           loadNode["rcon-password"].as<std::string>("");
    m_settings.rconKeys =               loadNode["rcon-keys"].as<std::vector<std::string>>(std::vector<std::string>());



	YAML::Node saveNode;

	saveNode["server-name"] =			    m_settings.serverName;
	saveNode["server-port"] =			    m_settings.serverPort;
	saveNode["server-password"] =		    m_settings.serverPassword;
	saveNode["server-public"] =			    m_settings.serverPublic;

	saveNode["world-name"] =			    m_settings.worldName;
	saveNode["world-seed"] =			    m_settings.worldSeed;

	saveNode["player-whitelist"] =		    m_settings.playerWhitelist;
	saveNode["player-max"] =			    m_settings.playerMax;
	saveNode["player-auth"] =			    m_settings.playerAuth;
	saveNode["player-list"] =			    m_settings.playerList;
	saveNode["player-arrive-ping"] =	    m_settings.playerArrivePing;

	saveNode["socket-timeout"] =		    m_settings.socketTimeout.count();
	saveNode["socket-max-congestion"] =		m_settings.socketMaxCongestion;
    saveNode["socket-min-congestion"] =		m_settings.socketMinCongestion;

    saveNode["rcon-enabled"] =              m_settings.rconEnabled;
    saveNode["rcon-port"] =                 m_settings.rconPort;
    saveNode["rcon-password"] =             m_settings.rconPassword;
    saveNode["rcon-keys"] =                 m_settings.rconKeys;


    YAML::Emitter out;
	out.SetIndent(4);
	out << saveNode;

	ResourceManager::WriteFileBytes("server.yml", out.c_str());

	LOG(INFO) << "Server config loaded";
	
	// have other settings like chat spam prevention, fly prevention, advanced permission settings
	


	ModManager::Init();
    NetManager::Init();
    ChatManager::Init();
    if (m_settings.rconEnabled) {
        if (m_settings.rconPassword.empty())
            m_settings.rconPassword = "mysecret";
        LOG(INFO) << "Enabling RCON on port " << m_settings.rconPort;
        LOG(INFO) << "RCON password is '" << m_settings.rconPassword << "'";
        m_rcon = std::make_unique<RCONAcceptor>();
        m_rcon->Listen();
    }

	m_startTime = steady_clock::now(); // const during server run
	m_prevUpdate = steady_clock::now();
	m_nowUpdate = steady_clock::now();

	m_running = true;
	while (m_running) {
		OPTICK_FRAME("main");

		auto now = steady_clock::now();
		auto elapsed = duration_cast<nanoseconds>(m_nowUpdate - m_prevUpdate);

		m_prevUpdate = m_nowUpdate; // old state
		m_nowUpdate = now; // new state

		// mutex is carefully scoped
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
				
		// UPDATE
		//float delta = elapsed.count() / (double)duration_cast<decltype(elapsed)>(1s).count();
		Update(Delta());

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
    ModManager::UnInit();
	NetManager::Close();
}





void ValhallaServer::Update(float delta) {
	// This is important to processing RPC remote invocations

    if (m_rcon) {
        while (auto rconSocket = m_rcon->Accept()) {
            m_rconSockets.insert({ 0, static_cast<RCONSocket*>(rconSocket) });
            rconSocket->Start();
        }

        for (auto&& pair : m_rconSockets) {
            static constexpr int32_t RCON_COMMAND_RESPONSE = 0;
            static constexpr int32_t RCON_COMMAND = 2;
            static constexpr int32_t RCON_LOGIN = 3;

            auto&& rconSocket = pair.second;
            rconSocket->Update();
            while (auto opt = rconSocket->Recv()) {
                auto &&pkg = opt.value();

                auto id = pkg.Read<int32_t>();
                auto cmd = pkg.Read<int32_t>();
                auto payload = pkg.m_stream.Remaining();

                auto message = std::string((char*)payload.data());

                if (!pair.first) {
                    if (cmd == RCON_LOGIN) {
                        if (!m_rconSockets.contains(id) && m_settings.rconPassword == message) {
                            pair.first = id;

                            std::string spacedMsg = " ";

                            NetPackage authPkg;
                            authPkg.Write(id);
                            authPkg.Write(RCON_COMMAND_RESPONSE);
                            authPkg.m_stream.Write((BYTE_t*)spacedMsg.data(), spacedMsg.size());
                            authPkg.Write((BYTE_t)0);

                            rconSocket->Send(authPkg);

                            continue;
                        }
                    }
                } else {
                    // run the command
                    LOG(INFO) << "Got command " << message;

                    std::string replyMsg = "You executed the command " + message;

                    NetPackage replyPkg;
                    replyPkg.Write(id);
                    replyPkg.Write(RCON_COMMAND_RESPONSE);
                    replyPkg.m_stream.Write((BYTE_t*)replyMsg.data(), replyMsg.size());
                    replyPkg.Write((BYTE_t)0);

                    rconSocket->Send(replyPkg);
                    continue;
                }

                rconSocket->Close();
                break;
            }
        }
    }

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
