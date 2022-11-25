#include <optick.h>
#include <yaml-cpp/yaml.h>

#include <utility>

#include "VServer.h"
#include "ModManager.h"
#include "VUtilsResource.h"
#include "ServerSettings.h"
#include "NetManager.h"
#include "ChatManager.h"
#include "NetSyncManager.h"
#include "WorldGenerator.h"

using namespace std::chrono;

std::unique_ptr<VServer> VServer_INSTANCE(std::make_unique<VServer>());
VServer* Valhalla() {
	return VServer_INSTANCE.get();
}



void VServer::LoadFiles() {
    {
        auto opt = VUtils::Resource::ReadFileLines("banned.txt");
        if (opt) {
            auto&& banned = opt.value();
            for (auto &&s: banned)
                m_banned.insert(std::move(s));
        }
    }

    YAML::Node loadNode;
    bool createSettingsFile = false;
    {
        if (auto&& opt = VUtils::Resource::ReadFileString("server.yml")) {
            try {
                loadNode = YAML::Load(opt.value());
            }
            catch (const YAML::ParserException& e) {
                LOG(INFO) << e.what();
                createSettingsFile = true;
            }
        }
        else {
            LOG(INFO) << "Server config not found, creating...";
            createSettingsFile = true;
        }
    }

    m_settings.serverName = loadNode["server-name"].as<std::string>("My server");
    m_settings.serverPort = loadNode["server-port"].as<uint16_t>(2456);
    m_settings.serverPassword = loadNode["server-password"].as<std::string>("secret");
    m_settings.serverPublic = loadNode["server-public"].as<bool>(false);

    m_settings.worldName = loadNode["world-name"].as<std::string>("Dedicated world");
    m_settings.worldSeedName = loadNode["world-seed-name"].as<std::string>("Some special seed");
    m_settings.worldSeed = VUtils::String::GetStableHashCode(m_settings.worldSeedName);

    m_settings.playerWhitelist = loadNode["player-whitelist"].as<bool>(false);		// enable whitelist
    m_settings.playerMax = loadNode["player-max"].as<unsigned int>(64);		// max allowed players
    m_settings.playerAuth = loadNode["player-auth"].as<bool>(true);				// allow authed players only
    m_settings.playerList = loadNode["player-list"].as<bool>(true);				// does not send player list to players
    m_settings.playerArrivePing = loadNode["player-arrive-ping"].as<bool>(true);		// prevent player join ping

    m_settings.socketTimeout = milliseconds(loadNode["socket-timeout"].as<unsigned int>(30000)); // player timeout in milliseconds
    m_settings.zdoMaxCongestion = loadNode["zdo-max-congestion"].as<int32_t>(10240);
    m_settings.zdoMinCongestion = loadNode["zdo-min-congestion"].as<int32_t>(2048);
    m_settings.zdoSendInterval = milliseconds(loadNode["zdo-send-interval"].as<unsigned int>(50)); // player timeout in milliseconds

    m_settings.rconEnabled = loadNode["rcon-enabled"].as<bool>(false);
    m_settings.rconPort = loadNode["rcon-port"].as<uint16_t>(25575);
    m_settings.rconPassword = loadNode["rcon-password"].as<std::string>("");
    m_settings.rconKeys = loadNode["rcon-keys"].as<std::vector<std::string>>(std::vector<std::string>());

    LOG(INFO) << "Server config loaded";

    if (createSettingsFile) {
        YAML::Node saveNode;

        saveNode["server-name"] = m_settings.serverName;
        saveNode["server-port"] = m_settings.serverPort;
        saveNode["server-password"] = m_settings.serverPassword;
        saveNode["server-public"] = m_settings.serverPublic;

        saveNode["world-name"] = m_settings.worldName;
        saveNode["world-seed"] = m_settings.worldSeed;

        saveNode["player-whitelist"] = m_settings.playerWhitelist;
        saveNode["player-max"] = m_settings.playerMax;
        saveNode["player-auth"] = m_settings.playerAuth;
        saveNode["player-list"] = m_settings.playerList;
        saveNode["player-arrive-ping"] = m_settings.playerArrivePing;

        saveNode["socket-timeout"] = m_settings.socketTimeout.count();
        saveNode["zdo-max-congestion"] = m_settings.zdoMaxCongestion;
        saveNode["zdo-min-congestion"] = m_settings.zdoMinCongestion;
        saveNode["zdo-send-interval"] = m_settings.zdoSendInterval.count();

        saveNode["rcon-enabled"] = m_settings.rconEnabled;
        saveNode["rcon-port"] = m_settings.rconPort;
        saveNode["rcon-password"] = m_settings.rconPassword;
        saveNode["rcon-keys"] = m_settings.rconKeys;

        YAML::Emitter out;
        out.SetIndent(4);
        out << saveNode;

        VUtils::Resource::WriteFileString("server.yml", out.c_str());
    }
}



void VServer::Launch() {
    this->LoadFiles();

    ModManager()->Init();
    WorldManager::Init();
    WorldGenerator::Init();
    NetManager::Init();
    ChatManager::Init();

    LOG(INFO) << "Server password is '" << m_settings.serverPassword << "'";

    // Initialize rcon server
    if (m_settings.rconEnabled) {
        if (m_settings.rconPassword.empty())
            m_settings.rconPassword = "mysecret";
        LOG(INFO) << "Enabling RCON on port " << m_settings.rconPort;
        LOG(INFO) << "RCON password is '" << m_settings.rconPassword << "'";
        m_rcon = std::make_unique<RCONAcceptor>();
        m_rcon->Listen();
    }

	m_prevUpdate = steady_clock::now();
	m_nowUpdate = steady_clock::now();

	m_running = true;
	while (m_running) {
		OPTICK_FRAME("main");

		auto now = steady_clock::now();
		auto elapsed = duration_cast<nanoseconds>(m_nowUpdate - m_prevUpdate);

		m_prevUpdate = m_nowUpdate; // old state
		m_nowUpdate = now; // new state

		// Mutex is scoped
		{
			std::scoped_lock lock(m_taskMutex);
			for (auto itr = m_tasks.begin(); itr != m_tasks.end();) {
				auto ptr = itr->get();
				if (ptr->at < now) {
					if (ptr->period < 0ms) {
						itr = m_tasks.erase(itr);
					}
					else {
						ptr->function(*ptr);
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
		
		Update(Delta());

		// TODO adjust based on workload intensity
		std::this_thread::sleep_for(1ms);
	}

    // Cleanup 
    ModManager()->UnInit();
    NetManager::Close();

    {
        std::vector<std::string> banned;
        for (auto&& s : m_banned)
            banned.push_back(std::move(s));
        VUtils::Resource::WriteFileLines("banned.txt", banned);
    }
}

void VServer::Terminate() {
	LOG(INFO) << "Terminating server";

    m_running = false;
}



void VServer::Update(float delta) {
	// This is important to processing RPC remote invocations

    if (m_rcon) {
        OPTICK_EVENT("Rcon");
        // TODO add cleanup
        //  also consider making socket cleaner easier to look at and understand; too many iterator / loops
        while (auto opt = m_rcon->Accept()) {
            auto&& rconSocket = std::static_pointer_cast<RCONSocket>(opt.value());

            m_rconSockets.push_back(rconSocket);
            LOG(INFO) << "Rcon accepted " << rconSocket->GetAddress();
            CALL_EVENT(EVENT_HASH_RconConnect, rconSocket);
        }

        // Authed sockets
        for (auto&& itr = m_rconSockets.begin(); itr != m_rconSockets.end();) {
            auto &&rconSocket = *itr;
            if (!rconSocket->Connected()) {
                LOG(INFO) << "Rcon disconnected " << rconSocket->GetAddress();
                CALL_EVENT(EVENT_HASH_RconDisconnect, rconSocket);
                itr = m_rconSockets.erase(itr);
            } else {
                rconSocket->Update();

                // receive the msg not pkg
                while (auto opt = rconSocket->RecvMsg()) {
                    auto &&msg = opt.value();
                    auto args = VUtils::String::Split(msg.msg, " ");
                    if (args.empty())
                        continue;

                    LOG(INFO) << "Got command " << msg.msg;

                    HASH_t cmd = __H(args[0]);

                    // Lua global handler
                    CALL_EVENT(EVENT_HASH_RconIn, rconSocket, args);

                    // Lua specific handler
                    CALL_EVENT(EVENT_HASH_RconIn ^ cmd, rconSocket, args);



                    if ("ban" == args[0] && args.size() >= 2) {
                        auto&& pair = m_banned.insert(std::string(args[1]));
                        if (pair.second)
                            rconSocket->SendMsg("Banned " + std::string(args[1]));
                        else
                            rconSocket->SendMsg(std::string(args[1]) + " is already banned");
                    } else if ("kick" == args[0] && args.size() >= 2) {
                        auto id = (OWNER_t) std::stoll(std::string(args[1]));
                        auto peer = NetManager::GetPeer(id);
                        if (peer) {
                            peer->Kick();
                            rconSocket->SendMsg("Kicked " + peer->m_name);
                        }
                        else {
                            rconSocket->SendMsg(peer->m_name + " is not online");
                        }
                    } else if ("list" == args[0]) {
                        auto&& peers = NetManager::GetPeers();
                        if (!peers.empty()) {
                            for (auto&& peer : peers) {
                                rconSocket->SendMsg(peer->m_name + " " + peer->m_rpc->m_socket->GetAddress()
                                    + " " + peer->m_rpc->m_socket->GetHostName());
                            }
                        }
                        else {
                            rconSocket->SendMsg("There are no peers online");
                        }
                    } else if ("stop" == args[0]) {
                        rconSocket->SendMsg("Stopping server...");
                        RunTaskLater([this](Task &) {
                            Terminate();
                        }, 5s);
                    } else if ("quit" == args[0] || "exit" == args[0]) {
                        rconSocket->SendMsg("Closing Rcon connection...");
                        rconSocket->Close(true);
                    } else {
                        rconSocket->SendMsg("Unknown command");
                    }
                }

                ++itr;
            }
        }
    }

	NetManager::Update(delta);
    NetSyncManager::Update(delta);
    CALL_EVENT(EVENT_HASH_Update, delta);
	//VModManager::Event::OnUpdate(delta);
}



Task& VServer::RunTask(Task::F f) {
	return RunTaskLater(std::move(f), 0ms);
}

Task& VServer::RunTaskLater(Task::F f, milliseconds after) {
	return RunTaskLaterRepeat(std::move(f), after, 0ms);
}

Task& VServer::RunTaskAt(Task::F f, steady_clock::time_point at) {
	return RunTaskAtRepeat(std::move(f), at, 0ms);
}

Task& VServer::RunTaskRepeat(Task::F f, milliseconds period) {
	return RunTaskLaterRepeat(std::move(f), 0ms, period);
}

Task& VServer::RunTaskLaterRepeat(Task::F f, milliseconds after, milliseconds period) {
	return RunTaskAtRepeat(std::move(f), steady_clock::now() + after, period);
}

Task& VServer::RunTaskAtRepeat(Task::F f, steady_clock::time_point at, milliseconds period) {
	std::scoped_lock lock(m_taskMutex);
	Task* task = new Task{std::move(f), at, period};
	m_tasks.push_back(std::unique_ptr<Task>(task));
	return *task;
}
