#include <optick.h>
#include <yaml-cpp/yaml.h>

#include <utility>

#include "ValhallaServer.h"
#include "VUtilsResource.h"
#include "ServerSettings.h"
#include "NetManager.h"
#include "ZoneManager.h"
#include "ZDOManager.h"

auto VALHALLA_INSTANCE(std::make_unique<IValhalla>());
IValhalla* Valhalla() {
    return VALHALLA_INSTANCE.get();
}



//std::atomic_bool ValhallaServer::m_running = false;
//std::list<std::unique_ptr<Task>> ValhallaServer::m_tasks;
//std::recursive_mutex ValhallaServer::m_taskMutex;
//ServerSettings ValhallaServer::m_settings;
//const OWNER_t ValhallaServer::m_serverID = VUtils::Random::GenerateUID();
//std::unique_ptr<RCONAcceptor> ValhallaServer::m_rcon;
//std::list<std::shared_ptr<RCONSocket>> ValhallaServer::m_rconSockets;
//const steady_clock::time_point ValhallaServer::m_startTime = steady_clock::now();
//steady_clock::time_point ValhallaServer::m_prevUpdate;
//steady_clock::time_point ValhallaServer::m_nowUpdate;
//double ValhallaServer::m_netTime = 0;



bool IValhalla::IsPeerAllowed(NetRpc* rpc) {
    //rpc
    return false;
}

void IValhalla::LoadFiles() {
    {
        auto opt = VUtils::Resource::ReadFileLines("banned.txt");
        if (opt) {
            auto&& banned = opt.value();
            for (auto &&s: banned)
                m_banned.insert(std::move(s));
        }
    }

    {
        auto opt = VUtils::Resource::ReadFileLines("admin.txt");
        if (opt) {
            auto&& admin = opt.value();
            for (auto&& s : admin)
                m_admin.insert(std::move(s));
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

    m_settings.playerWhitelist = loadNode["player-whitelist"].as<bool>(false);        // enable whitelist
    m_settings.playerMax = loadNode["player-max"].as<unsigned int>(64);        // max allowed players
    m_settings.playerAuth = loadNode["player-auth"].as<bool>(true);                // allow authed players only
    m_settings.playerList = loadNode["player-list"].as<bool>(true);                // does not send player list to players
    m_settings.playerArrivePing = loadNode["player-arrive-ping"].as<bool>(true);        // prevent player join ping

    m_settings.socketTimeout = milliseconds(loadNode["socket-timeout"].as<unsigned int>(30000)); // player timeout in milliseconds
    m_settings.zdoMaxCongestion = loadNode["zdo-max-congestion"].as<int32_t>(10240);
    m_settings.zdoMinCongestion = loadNode["zdo-min-congestion"].as<int32_t>(2048);
    m_settings.zdoSendInterval = milliseconds(loadNode["zdo-send-interval"].as<unsigned int>(50)); // player timeout in milliseconds

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

        YAML::Emitter out;
        out.SetIndent(4);
        out << saveNode;

        VUtils::Resource::WriteFileString("server.yml", out.c_str());
    }
}

void IValhalla::Stop() {
    m_running = false;
}

void IValhalla::Start() {
    LOG(INFO) << "Starting Valhalla";

    assert(!m_running);

    // Does not work properly in some circumstances
    signal(SIGINT, [](int) {
        Valhalla()->Stop();
    });

    m_running = false;
    m_serverID = VUtils::Random::GenerateUID();
    m_startTime = steady_clock::now();
    m_netTime = 0;
    
    this->LoadFiles();

    PrefabManager()->Init();
    ZDOManager()->Init();
    ZoneManager()->Init();
    WorldManager()->Init();
    NetManager()->Init();

    LOG(INFO) << "Server password is '" << m_settings.serverPassword << "'";

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
                    if (ptr->period == milliseconds::min()) { // if task cancelled
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
        
        Update();

        // TODO adjust based on workload intensity
        std::this_thread::sleep_for(1ms);
    }

    // Cleanup 
    NetManager()->Close();
    {
        std::vector<std::string> banned;
        for (auto&& s : m_banned)
            banned.push_back(std::move(s));
        VUtils::Resource::WriteFileLines("banned.txt", banned);
    }
}



void IValhalla::Update() {
    // This is important to processing RPC remote invocations

    if (!NetManager()->GetPeers().empty())
        m_netTime += Delta();
    
    NetManager()->Update();
}



Task& IValhalla::RunTask(Task::F f) {
    return RunTaskLater(std::move(f), 0ms);
}

Task& IValhalla::RunTaskLater(Task::F f, milliseconds after) {
    return RunTaskLaterRepeat(std::move(f), after, 0ms);
}

Task& IValhalla::RunTaskAt(Task::F f, steady_clock::time_point at) {
    return RunTaskAtRepeat(std::move(f), at, 0ms);
}

Task& IValhalla::RunTaskRepeat(Task::F f, milliseconds period) {
    return RunTaskLaterRepeat(std::move(f), 0ms, period);
}

Task& IValhalla::RunTaskLaterRepeat(Task::F f, milliseconds after, milliseconds period) {
    return RunTaskAtRepeat(std::move(f), steady_clock::now() + after, period);
}

Task& IValhalla::RunTaskAtRepeat(Task::F f, steady_clock::time_point at, milliseconds period) {
    std::scoped_lock lock(m_taskMutex);
    Task* task = new Task{std::move(f), at, period};
    m_tasks.push_back(std::unique_ptr<Task>(task));
    return *task;
}
