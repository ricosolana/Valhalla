#include <optick.h>
#include <yaml-cpp/yaml.h>

#include <utility>

#include "VServer.h"
#include "ModManager.h"
#include "VUtilsResource.h"
#include "ServerSettings.h"
#include "NetManager.h"
#include "ChatManager.h"

using namespace std::chrono;

std::unique_ptr<VServer> VServer_INSTANCE(std::make_unique<VServer>());
VServer* Valhalla() {
	return VServer_INSTANCE.get();
}



void VServer::LoadFiles() {
    {
        std::vector<std::string> banned;
        VUtils::Resource::ReadFileLines("banned.txt", banned);
        for (auto&& s : banned)
            m_banned.insert(std::move(s));
    }

    YAML::Node loadNode;
    {
        std::string buf;
        if (VUtils::Resource::ReadFileBytes("server.yml", buf)) {
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

    VUtils::Resource::WriteFileBytes("server.yml", out.c_str());

    LOG(INFO) << "Server config loaded";
}



void VServer::Launch() {
    this->LoadFiles();

    ModManager()->Init();
	//VModManager::Init();
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

void VServer::Terminate() {
	LOG(INFO) << "Terminating server";

	m_running = false;
    ModManager()->UnInit();
	NetManager::Close();

    {
        std::vector<std::string> banned;
        for (auto &&s: m_banned)
            banned.push_back(std::move(s));
        VUtils::Resource::WriteFileLines("banned.txt", banned);
    }
}





void VServer::Update(float delta) {
	// This is important to processing RPC remote invocations

    if (m_rcon) {
        OPTICK_EVENT("Rcon");
        // TODO add cleanup
        //  also consider making socket cleaner easier to look at and understand; too many iterator / loops
        while (auto opt = m_rcon->Accept()) {
            auto&& rconSocket = std::static_pointer_cast<RCONSocket>(opt.value());

            m_unAuthRconSockets.push_back(rconSocket);
            rconSocket->Start();
            LOG(INFO) << "Rcon connecting " << rconSocket->GetAddress();
        }

        static constexpr int32_t RCON_S2C_RESPONSE = 0;
        static constexpr int32_t RCON_COMMAND = 2;
        static constexpr int32_t RCON_C2S_LOGIN = 3;

        static auto send_response = [](std::shared_ptr<RCONSocket>& socket, int32_t client_id, const std::string &msg) {
            static NetPackage pkg;
            pkg.Write(client_id);
            pkg.Write(RCON_S2C_RESPONSE);
            pkg.m_stream.Write((BYTE_t *) msg.data(), msg.size() + 1);

            socket->Send(std::move(pkg));
        };

        // Un authed RCON sockets polling
        for (auto&& itr = m_unAuthRconSockets.begin(); itr != m_unAuthRconSockets.end();) {
            auto &&rconSocket = (*itr);
            if (!(*itr)->Connected()) {
                LOG(INFO) << "Unauthorized Rcon disconnected " << rconSocket->GetAddress();
                itr = m_unAuthRconSockets.erase(itr);
            } else {
                rconSocket->Update();
                auto opt = rconSocket->Recv();
                // first packet must be the login packet
                if (opt) {
                    auto &&pkg = opt.value();

                    auto client_id = pkg.Read<int32_t>();
                    auto msg_type = pkg.Read<int32_t>();
                    auto in_msg = std::string((char *) pkg.m_stream.Remaining().data());
                    VUtils::String::FormatAscii(in_msg);

                    if (in_msg == m_settings.rconPassword && !m_authRconSockets.contains(client_id)) {
                        //if (CALL_EVENT("RconConnect", rconSocket) == EVENT_CANCEL)
                            //continue;

                        send_response(rconSocket, client_id, " ");
                        m_authRconSockets[client_id] = rconSocket;
                        LOG(INFO) << "Rcon authorized " << rconSocket->GetAddress();
                    } else {
                        send_response(rconSocket, -1, " ");
                        rconSocket->Close(true);
                        LOG(INFO) << "Rcon failed authorization " << rconSocket->GetAddress();
                    }
                    itr = m_unAuthRconSockets.erase(itr);
                } else
                    ++itr;
            }
        }

        // Authed sockets
        for (auto&& itr = m_authRconSockets.begin(); itr != m_authRconSockets.end();) {
            auto &&rconSocket = itr->second;
            if (!itr->second->Connected()) {
                LOG(INFO) << "Authorized Rcon disconnected " << rconSocket->GetAddress();
                itr = m_authRconSockets.erase(itr);
            } else {
                rconSocket->Update();
                while (auto opt = rconSocket->Recv()) {
                    auto &&pkg = opt.value();

                    pkg.Read<int32_t>(); // dummy client id
                    pkg.Read<int32_t>(); // dummy type
                    auto in_msg = std::string((char *) pkg.m_stream.Remaining().data());
                    VUtils::String::FormatAscii(in_msg);

                    std::string out_msg = " ";

                    // Ideas for dynamic high-performance command parser:
                    // robin hood map to contain lowercase commands as keys
                    // map will contain functions with args ( client id )
                    // rcon is not secure anyway... not much should be inputted here

                    LOG(INFO) << "Got command " << in_msg;

                    auto args(VUtils::String::Split(in_msg, " "));

                    if ("ban" == args[0] && args.size() == 2) {
                        m_banned.insert(std::string(args[1]));
                        out_msg = "Banned " + std::string(args[1]);
                    } else if ("kick" == args[0] && args.size() == 2) {
                        auto id = (OWNER_t) std::stoll(std::string(args[1]));
                        auto peer = NetManager::GetPeer(id);
                        if (peer) {
                            peer->Kick();
                            out_msg = "Kicked " + peer->m_name;
                        } else
                            out_msg = "Peer is not online";
                    } else if ("list" == args[0]) {
                        out_msg = "Player-list not yet supported";
                    } else if ("stop" == args[0]) {
                        out_msg = "Stopping server...";
                        RunTaskLater([this](Task &) {
                            Terminate();
                        }, 5s);
                    } else if ("quit" == args[0] || "exit" == args[0]) {
                        out_msg = "Closing RCON connection...";
                        rconSocket->Close(true);
                    } else {
                        out_msg = "Unknown command";
                    }

                    send_response(rconSocket, itr->first, out_msg);
                }

                ++itr;
            }
        }
    }

	NetManager::Update(delta);
    CALL_EVENT("Update", delta);
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
