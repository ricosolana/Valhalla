#include <optick.h>
#include <yaml-cpp/yaml.h>

#include <stdlib.h>
#include <utility>

#include "ValhallaServer.h"
#include "VUtilsResource.h"
#include "ServerSettings.h"
#include "NetManager.h"
#include "ZoneManager.h"
#include "ZDOManager.h"
#include "GeoManager.h"
#include "RouteManager.h"
#include "Hashes.h"
#include "HeightmapBuilder.h"
#include "ModManager.h"
#include "DungeonManager.h"

auto VALHALLA_INSTANCE(std::make_unique<IValhalla>());
IValhalla* Valhalla() {
    return VALHALLA_INSTANCE.get();
}



bool IValhalla::IsPeerAllowed(NetRpc* rpc) {
    //rpc
    return false;
}

void IValhalla::LoadFiles() {
    YAML::Node loadNode;
    bool createSettingsFile = false;
    {
        if (auto opt = VUtils::Resource::ReadFileString("server.yml")) {
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

    // TODO:
    // consider moving extraneous features into a separate lua mod group
    //  to not pollute the primary base server code
    //  crucial quality of life features are fine however to remain in c++  (zdos/send rates/...)

    m_settings.serverName = VUtils::String::ToAscii(loadNode["server-name"].as<std::string>(""));
    if (m_settings.serverName.empty()) m_settings.serverName = "Valhalla server";
    m_settings.serverPort = loadNode["server-port"].as<uint16_t>(2456);
    m_settings.serverPassword = loadNode["server-password"].as<std::string>("secret");
    m_settings.serverPublic = loadNode["server-public"].as<bool>(false);

    m_settings.worldName = VUtils::String::ToAscii(loadNode["world-name"].as<std::string>(""));
    if (m_settings.worldName.empty()) m_settings.worldName = "world";
    m_settings.worldSeed = loadNode["world-seed-name"].as<std::string>("");
    if (m_settings.worldSeed.empty()) m_settings.worldSeed = VUtils::Random::GenerateAlphaNum(10);
    //m_settings.worldSeed = VUtils::String::GetStableHashCode(m_settings.worldSeedName);
    m_settings.worldSave = loadNode["world-save"].as<bool>(false);
    m_settings.worldSaveInterval = seconds(std::max(loadNode["world-save-interval-s"].as<int>(1800), 60));
    m_settings.worldModern = loadNode["world-modern"].as<bool>(true);

    m_settings.playerAutoPassword = loadNode["player-auto-password"].as<bool>(true);
    m_settings.playerWhitelist = loadNode["player-whitelist"].as<bool>(false);          // enable whitelist
    m_settings.playerMax = std::max(loadNode["player-max"].as<int>(10), 1);                 // max allowed players
    m_settings.playerAuth = loadNode["player-auth"].as<bool>(true);                     // allow authed players only
    m_settings.playerList = loadNode["player-list"].as<bool>(true);                     // does not send player list to players
    //m_settings.playerArrivePing = loadNode["player-arrive-ping"].as<bool>(true);        // prevent player join ping
    m_settings.playerForceVisible = loadNode["player-map-visible"].as<bool>(false);   // force players to be visible on map

    m_settings.socketTimeout = milliseconds(loadNode["socket-timeout-ms"].as<unsigned int>(30000)); // player timeout in milliseconds

    m_settings.zdoMaxCongestion = loadNode["zdo-max-congestion"].as<int32_t>(10240);
    m_settings.zdoMinCongestion = loadNode["zdo-min-congestion"].as<int32_t>(2048);
    m_settings.zdoSendInterval = milliseconds(loadNode["zdo-send-interval-ms"].as<unsigned int>(50));
    m_settings.zdoAssignInterval = seconds(std::max(loadNode["zdo-assign-interval-s"].as<unsigned int>(2), 1U));
    m_settings.zdoSmartAssign = loadNode["zdo-smart-assign"].as<bool>(true);

    m_settings.spawningCreatures = loadNode["spawning-creatures"].as<bool>(true);
    m_settings.spawningLocations = loadNode["spawning-locations"].as<bool>(true);
    m_settings.spawningVegetation = loadNode["spawning-vegetation"].as<bool>(true);
    
    LOG(INFO) << "Server config loaded";

    if (createSettingsFile) {
        YAML::Node saveNode;
        
        saveNode["server-name"] = m_settings.serverName;
        saveNode["server-port"] = m_settings.serverPort;
        saveNode["server-password"] = m_settings.serverPassword;
        saveNode["server-public"] = m_settings.serverPublic;

        saveNode["world-name"] = m_settings.worldName;
        saveNode["world-seed"] = m_settings.worldSeed;
        saveNode["world-save"] = m_settings.worldSave;
        saveNode["world-save-interval-s"] = m_settings.worldSaveInterval.count();
        saveNode["world-modern"] = m_settings.worldModern;
        //saveNode["world-seed"] = m_settings.worldSeed;

        saveNode["player-auto-password"] = m_settings.playerAutoPassword;
        saveNode["player-whitelist"] = m_settings.playerWhitelist;
        saveNode["player-max"] = m_settings.playerMax;
        saveNode["player-auth"] = m_settings.playerAuth;
        saveNode["player-list"] = m_settings.playerList;
        saveNode["player-map-visible"] = m_settings.playerForceVisible;
        //saveNode["player-arrive-ping"] = m_settings.playerArrivePing;

        saveNode["socket-timeout-ms"] = m_settings.socketTimeout.count();

        saveNode["zdo-max-congestion"] = m_settings.zdoMaxCongestion;
        saveNode["zdo-min-congestion"] = m_settings.zdoMinCongestion;
        saveNode["zdo-send-interval-ms"] = m_settings.zdoSendInterval.count();
        saveNode["zdo-assign-interval-s"] = m_settings.zdoAssignInterval.count();
        saveNode["zdo-smart-assign"] = m_settings.zdoSmartAssign;

        saveNode["spawning-creatures"] = m_settings.spawningCreatures;
        saveNode["spawning-locations"] = m_settings.spawningLocations;
        saveNode["spawning-vegetation"] = m_settings.spawningVegetation;

        YAML::Emitter out;
        out.SetIndent(4);
        out << saveNode;

        VUtils::Resource::WriteFileString("server.yml", out.c_str());
    }

    if (auto opt = VUtils::Resource::ReadFileLines<decltype(m_blacklist)>("blacklist.txt")) {
        m_blacklist = *opt;
    }

    if (m_settings.playerWhitelist)
        if (auto opt = VUtils::Resource::ReadFileLines<decltype(m_whitelist)>("whitelist.txt")) {
            m_whitelist = *opt;
        }

    if (auto opt = VUtils::Resource::ReadFileLines<decltype(m_admin)>("admin.txt")) {
        m_admin = *opt;
    }

    if (m_settings.playerAutoPassword)
        if (auto opt = VUtils::Resource::ReadFileLines<decltype(m_bypass)>("bypass.txt")) {
            m_bypass = *opt;
        }


}

void IValhalla::Stop() {
    m_running = false;
}

void IValhalla::Start() {
    LOG(INFO) << "Starting Valhalla " << SERVER_VERSION << " (Valheim " << VConstants::GAME << ")";
    
    assert(!m_running);

    m_running = false;
    m_serverID = VUtils::Random::GenerateUID();
    m_startTime = steady_clock::now();

    this->LoadFiles();

    m_netTime = 2040;

    PrefabManager()->Init();
    ZDOManager()->Init();
    ZoneManager()->Init();
    WorldManager()->Init();
    GeoManager()->Init();
    ZoneManager()->GenerateLocations();
    DungeonManager()->Init();
    ModManager()->Init();

    HeightmapBuilder()->Init();
    NetManager()->Init();

    LOG(INFO) << "Server password is '" << m_settings.serverPassword << "'";

    // Basically, allow players who have already logged in before, unless the password has changed
    if (m_settings.playerAutoPassword) {
        const char* prevPassword = getenv("valhalla-prev-password");

        // Clear the allow list on password change
        if (prevPassword && m_settings.serverPassword != prevPassword) {
            m_bypass.clear();
        }

        // Put the env
        std::string kv = "valhalla-prev-password=" + m_settings.serverPassword;
        if (putenv(kv.c_str())) {
            char* msg = strerror(errno);
            LOG(ERROR) << "Failed to set env: " << msg;
        }
        else {
            LOG(INFO) << "Player auto password is enabled";
        }
    }

    m_prevUpdate = steady_clock::now();
    m_nowUpdate = steady_clock::now();

    LOG(INFO) << "Press ctrl+c to exit";
    signal(SIGINT, [](int) {
        LOG(WARNING) << "Interrupt caught, stopping server";
        Valhalla()->Stop();
    });

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
    NetManager()->Uninit();
    HeightmapBuilder()->Uninit();

    ModManager()->Uninit();

    WorldManager()->WriteFileWorldDB(true);

    VUtils::Resource::WriteFileLines("blacklist.txt", m_blacklist);
    VUtils::Resource::WriteFileLines("whitelist.txt", m_whitelist);
    VUtils::Resource::WriteFileLines("admin.txt", m_admin);
    VUtils::Resource::WriteFileLines("bypass.txt", m_bypass);
}



void IValhalla::Update() {
    // This is important to processing RPC remote invocations

    if (!NetManager()->GetPeers().empty())
        m_netTime += Delta();
    
    ModManager()->Update();
    NetManager()->Update();
    ZDOManager()->Update();
    ZoneManager()->Update();

    PERIODIC_NOW(180s, {
        LOG(INFO) << "There are a total of " << NetManager()->GetPeers().size() << " peers online";
    });

    PERIODIC_NOW(1s, {
        ModManager()->CallEvent("PeriodUpdate");
    })

    //PERIODIC_NOW(1s, {
    //    LOG(INFO) << "update";
    //});

    if (SERVER_SETTINGS.worldSave) {
        // save warming message
        PERIODIC_LATER(SERVER_SETTINGS.worldSaveInterval, SERVER_SETTINGS.worldSaveInterval, {
            LOG(INFO) << "World saving in 30s";
            Broadcast(MessageType::Center, "$msg_worldsavewarning 30s");
        });

        PERIODIC_LATER(SERVER_SETTINGS.worldSaveInterval, SERVER_SETTINGS.worldSaveInterval + 30s, {
            WorldManager()->WriteFileWorldDB(false);
        });
    }
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

void IValhalla::Broadcast(MessageType type, const std::string& text) {
    RouteManager()->InvokeAll(Hashes::Routed::ShowMessage, type, text);
}
