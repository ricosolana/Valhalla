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
#include "EventManager.h"

auto VALHALLA_INSTANCE(std::make_unique<IValhalla>());
IValhalla* Valhalla() {
    return VALHALLA_INSTANCE.get();
}



void IValhalla::LoadFiles(bool fresh) {
    bool fileError = false;
    
    {
        YAML::Node loadNode;
        {
            if (auto opt = VUtils::Resource::ReadFile<std::string>("server.yml")) {
                try {
                    loadNode = YAML::Load(opt.value());
                }
                catch (const YAML::ParserException& e) {
                    LOG(INFO) << e.what();
                    fileError = true;
                }
            }
            else {
                if (fresh) {
                    LOG(INFO) << "Server config not found, creating...";
                }
                fileError = true;
            }
        }

        // TODO:
        // consider moving extraneous features into a separate lua mod group
        //  to not pollute the primary base server code
        //  crucial quality of life features are fine however to remain in c++  (zdos/send rates/...)

        if (fresh || !fileError) {
            if (fresh) m_settings.serverName = VUtils::String::ToAscii(loadNode["server-name"].as<std::string>(""));
            if (m_settings.serverName.empty()
                || m_settings.serverName.length() < 3
                || m_settings.serverName.length() > 64) m_settings.serverName = "Valhalla server";
            if (fresh) m_settings.serverPort = loadNode["server-port"].as<uint16_t>(2456);
            //m_settings.serverPassword = loadNode["server-password"].as<std::string>("secret");
            if (fresh) m_settings.serverPassword = loadNode["server-password"].as<std::string>("s");
            if (fresh && !m_settings.serverPassword.empty()
                && (m_settings.serverPassword.length() < 5
                    || m_settings.serverPassword.length() > 11)) m_settings.serverPassword = VUtils::Random::GenerateAlphaNum(6);
            if (fresh) m_settings.serverPublic = loadNode["server-public"].as<bool>(false);

            if (fresh) m_settings.worldName = VUtils::String::ToAscii(loadNode["world-name"].as<std::string>(""));
            if (m_settings.worldName.empty() || m_settings.worldName.length() < 3) m_settings.worldName = "world";
            if (fresh) m_settings.worldSeed = loadNode["world-seed-name"].as<std::string>("");
            if (m_settings.worldSeed.empty()) m_settings.worldSeed = VUtils::Random::GenerateAlphaNum(10);
            if (fresh) m_settings.worldPregenerate = loadNode["world-pregenerate"].as<bool>(false);
            m_settings.worldSave = loadNode["world-save"].as<bool>(true);
            m_settings.worldSaveInterval = seconds(std::clamp(loadNode["world-save-interval-s"].as<int>(1800), 60, 60 * 60));
            if (fresh) m_settings.worldModern = loadNode["world-modern"].as<bool>(true);
            if (fresh) m_settings.worldMode = (WorldMode) loadNode["world-mode"].as<std::underlying_type_t<WorldMode>>(std::to_underlying(WorldMode::NORMAL));
            if (fresh) m_settings.worldCaptureDumpSize = std::clamp(loadNode["world-capture-dump-size"].as<size_t>(256000ULL), 64000ULL, 256000000ULL);

            //m_settings.playerAutoPassword = loadNode["player-auto-password"].as<bool>(true);
            m_settings.playerWhitelist = loadNode["player-whitelist"].as<bool>(false);          // enable whitelist
            m_settings.playerMax = std::clamp(loadNode["player-max"].as<int>(10), 1, 99999);     // max allowed players
            m_settings.playerAuth = loadNode["player-auth"].as<bool>(true);                     // allow authed players only
            //m_settings.playerList = loadNode["player-list"].as<bool>(true);                     // does not send player list to players
            //m_settings.playerArrivePing = loadNode["player-arrive-ping"].as<bool>(true);        // prevent player join ping
            //m_settings.playerForceVisible = loadNode["player-map-visible"].as<bool>(false);   // force players to be visible on map # TODO put this in lua?
            //m_settings.playerSleep = loadNode["player-sleep"].as<bool>(true);
            //m_settings.playerSleepSolo = loadNode["player-sleep-solo"].as<bool>(false);

            if (fresh) m_settings.socketTimeout = milliseconds(std::max(1000, loadNode["socket-timeout-ms"].as<int>(30000)));

            m_settings.zdoMaxCongestion = loadNode["zdo-max-congestion"].as<int>(10240);
            m_settings.zdoMinCongestion = loadNode["zdo-min-congestion"].as<int>(2048);
            m_settings.zdoSendInterval = milliseconds(loadNode["zdo-send-interval-ms"].as<int>(50));
            m_settings.zdoAssignInterval = seconds(std::clamp(loadNode["zdo-assign-interval-s"].as<int>(2), 1, 60));
            m_settings.zdoSmartAssign = loadNode["zdo-smart-assign"].as<bool>(false);

            m_settings.spawningCreatures = loadNode["spawning-creatures"].as<bool>(true);
            m_settings.spawningLocations = loadNode["spawning-locations"].as<bool>(true);
            m_settings.spawningVegetation = loadNode["spawning-vegetation"].as<bool>(true);
            m_settings.spawningDungeons = loadNode["spawning-dungeons"].as<bool>(true);

            m_settings.dungeonEndCaps = loadNode["dungeon-end-caps"].as<bool>(true);
            m_settings.dungeonDoors = loadNode["dungeon-doors"].as<bool>(true);
            m_settings.dungeonFlipRooms = loadNode["dungeon-flip-rooms"].as<bool>(true);
            m_settings.dungeonZoneLimit = loadNode["dungeon-zone-limit"].as<bool>(true);
            m_settings.dungeonRoomShrink = loadNode["dungeon-room-shrink"].as<bool>(true);
            m_settings.dungeonReset = loadNode["dungeon-reset"].as<bool>(true);
            m_settings.dungeonResetTime = seconds(std::max(60, loadNode["dungeon-reset-time-s"].as<int>(3600 * 72)));
            //m_settings.dungeonIncrementalResetTime = seconds(std::max(1, loadNode["dungeon-incremental-reset-time-s"].as<int>(5)));
            m_settings.dungeonIncrementalResetCount = std::min(20, loadNode["dungeon-incremental-reset-count"].as<int>(3));
            m_settings.dungeonRandomGeneration = loadNode["dungeon-random-generation"].as<bool>(true);

            m_settings.eventsEnabled = loadNode["events-enabled"].as<bool>(true);
            m_settings.eventsChance = std::clamp(loadNode["events-chance"].as<float>(.2f), 0.f, 1.f);
            m_settings.eventsInterval = seconds(std::max(60, loadNode["events-interval-m"].as<int>(46)) * 60);
            m_settings.eventsRange = std::clamp(loadNode["events-range"].as<float>(96), 1.f, 96.f * 4);
            //m_settings.eventsSendInterval = loadNode["events-enabled"].as<bool>(true);

            if (m_settings.serverPassword.empty())
                LOG(WARNING) << "Server does not have a password";
            else
                LOG(INFO) << "Server password is '" << m_settings.serverPassword << "'";

            if (m_settings.worldMode == WorldMode::CAPTURE) {
                LOG(WARNING) << "Experimental packet capture enabled";
            }
            else if (m_settings.worldMode == WorldMode::PLAYBACK) {
                LOG(WARNING) << "Experimental packet playback enabled";
            }
        }
    }
    
    LOG(INFO) << "Server config loaded";

    if (fresh && fileError) {
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

        //saveNode["player-auto-password"] = m_settings.playerAutoPassword;
        saveNode["player-whitelist"] = m_settings.playerWhitelist;
        saveNode["player-max"] = m_settings.playerMax;
        saveNode["player-auth"] = m_settings.playerAuth;
        //saveNode["player-list"] = m_settings.playerList;
        //saveNode["player-map-visible"] = m_settings.playerForceVisible;
        //saveNode["player-arrive-ping"] = m_settings.playerArrivePing;
        //saveNode["player-sleep"] = m_settings.playerSleep;
        //saveNode["player-sleep-solo"] = m_settings.playerSleepSolo;

        saveNode["socket-timeout-ms"] = m_settings.socketTimeout.count();

        saveNode["zdo-max-congestion"] = m_settings.zdoMaxCongestion;
        saveNode["zdo-min-congestion"] = m_settings.zdoMinCongestion;
        saveNode["zdo-send-interval-ms"] = m_settings.zdoSendInterval.count();
        saveNode["zdo-assign-interval-s"] = m_settings.zdoAssignInterval.count();
        saveNode["zdo-smart-assign"] = m_settings.zdoSmartAssign;

        saveNode["spawning-creatures"] = m_settings.spawningCreatures;
        saveNode["spawning-locations"] = m_settings.spawningLocations;
        saveNode["spawning-vegetation"] = m_settings.spawningVegetation;
        saveNode["spawning-dungeons"] = m_settings.spawningDungeons;

        saveNode["dungeon-end-caps"] = m_settings.dungeonEndCaps;
        saveNode["dungeon-doors"] = m_settings.dungeonDoors;
        saveNode["dungeon-flip-rooms"] = m_settings.dungeonFlipRooms;
        saveNode["dungeon-zone-limit"] = m_settings.dungeonZoneLimit;
        saveNode["dungeon-room-shrink"] = m_settings.dungeonRoomShrink;
        saveNode["dungeon-reset"] = m_settings.dungeonReset;
        saveNode["dungeon-reset-time-s"] = m_settings.dungeonResetTime.count();
        //saveNode["dungeon-incremental-reset-time-s"] = m_settings.dungeonIncrementalResetTime.count();
        saveNode["dungeon-incremental-reset-count"] = m_settings.dungeonIncrementalResetCount;
        saveNode["dungeon-random-generation"] = m_settings.dungeonRandomGeneration;

        saveNode["events-enabled"] = m_settings.eventsEnabled;
        saveNode["events-chance"] = m_settings.eventsChance;
        saveNode["events-interval-m"] = m_settings.eventsInterval.count() / 60;
        saveNode["events-range"] = m_settings.eventsRange;

        YAML::Emitter out;
        out.SetIndent(4);
        out << saveNode;

        VUtils::Resource::WriteFile("server.yml", out.c_str());
    }

    if (auto opt = VUtils::Resource::ReadFile<decltype(m_blacklist)>("blacklist.txt")) {
        m_blacklist = *opt;
    }

    if (m_settings.playerWhitelist)
        if (auto opt = VUtils::Resource::ReadFile<decltype(m_whitelist)>("whitelist.txt")) {
            m_whitelist = *opt;
        }

    if (auto opt = VUtils::Resource::ReadFile<decltype(m_admin)>("admin.txt")) {
        m_admin = *opt;
    }

    if (!fresh) {
        // then iterate players, settings active and inactive
        for (auto&& peer : NetManager()->GetPeers()) {
            peer->m_admin = m_admin.contains(peer->m_name);
        }
    }

    std::error_code err;
    this->m_settingsLastTime = fs::last_write_time("server.yml", err);

    //if (m_settings.playerAutoPassword)
    //    if (auto opt = VUtils::Resource::ReadFile<decltype(m_bypass)>("bypass.txt")) {
    //        m_bypass = *opt;
    //    }
}

void IValhalla::Stop() {
    m_terminate = true;

    // prevent deadlock
    if (el::Helpers::getThreadName() != "main")
        m_terminate.wait(true);
}

void IValhalla::Start() {
    LOG(INFO) << "Starting Valhalla " << VALHALLA_SERVER_VERSION << " (Valheim " << VConstants::GAME << ")";
    
    m_serverID = VUtils::Random::GenerateUID();
    m_startTime = steady_clock::now();

    this->LoadFiles(true);

    m_worldTime = 2040;

    ZDOManager()->Init();
    EventManager()->Init();
    PrefabManager()->Init();

    ZoneManager()->PostPrefabInit();
    DungeonManager()->PostPrefabInit();

    WorldManager()->PostZoneInit();
    GeoManager()->PostWorldInit();
    HeightmapBuilder()->PostGeoInit();
    ZoneManager()->PostGeoInit();

    NetManager()->PostInit();
    ModManager()->PostInit();

    /*
    if (SERVER_SETTINGS.worldRecording) {
        World* world = WorldManager()->GetWorld();
        VUtils::Resource::WriteFile(
            fs::path(VALHALLA_WORLD_RECORDING_PATH) / world->m_name / (world->m_name + ".db"),
            WorldManager()->SaveWorldDB());
    }*/

    m_prevUpdate = steady_clock::now();
    m_nowUpdate = steady_clock::now();

#ifdef _WIN32
    SetConsoleCtrlHandler([](DWORD dwCtrlType) {
#else // !_WIN32
    signal(SIGINT, [](int) {
#endif // !_WIN32
        el::Helpers::setThreadName("system");
        Valhalla()->Stop();
#ifdef _WIN32
        return TRUE;
    }, TRUE);
#else // !_WIN32
    });
#endif // !_WIN32

    m_terminate = false;
    while (!m_terminate) {
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

        PERIODIC_NOW(1s, {
            PeriodUpdate();
        });

        std::this_thread::sleep_for(1ms);
    }
            
    LOG(INFO) << "Terminating server";

    // Cleanup 
    NetManager()->Uninit();
    HeightmapBuilder()->Uninit();

    ModManager()->Uninit();

    WorldManager()->WriteFileWorldDB(true);

    VUtils::Resource::WriteFile("blacklist.txt", m_blacklist);
    VUtils::Resource::WriteFile("whitelist.txt", m_whitelist);
    VUtils::Resource::WriteFile("admin.txt", m_admin);
    //VUtils::Resource::WriteFile("bypass.txt", m_bypass);

    LOG(INFO) << "Server was gracefully terminated";

    // signal any other dummy thread to continue
    m_terminate = false;
}



void IValhalla::Update() {
    // This is important to processing RPC remote invocations

    if (!NetManager()->GetPeers().empty()) {
        m_worldTime += Delta() * m_worldTimeMultiplier;
    }
    
    ModManager()->Update();
    NetManager()->Update();
    ZDOManager()->Update();
    ZoneManager()->Update();
    EventManager()->Update();
    HeightmapBuilder()->Update();
}

void IValhalla::PeriodUpdate() {
    PERIODIC_NOW(180s, {
        LOG(INFO) << "There are a total of " << NetManager()->GetPeers().size() << " peers online";
    });

    ModManager()->CallEvent(IModManager::Events::PeriodicUpdate);

    if (m_settings.dungeonReset)
        DungeonManager()->TryRegenerateDungeons();
    
    std::error_code err;
    auto lastWriteTime = fs::last_write_time("server.yml", err);
    if (lastWriteTime != this->m_settingsLastTime) {
        // reload the file
        LoadFiles(false);
    }

    if (m_settings.worldMode == WorldMode::PLAYBACK) {
        PERIODIC_NOW(7s, {
            Broadcast(UIMsgType::TopLeft, std::string("World playback ") + std::to_string(duration_cast<seconds>(Valhalla()->Nanos()).count()) + "s");
        });
    }

    if (m_settings.worldSave) {
        // save warming message
        PERIODIC_LATER(m_settings.worldSaveInterval, m_settings.worldSaveInterval, {
            LOG(INFO) << "World saving in 30s";
            Broadcast(UIMsgType::Center, "$msg_worldsavewarning 30s");
            });

        PERIODIC_LATER(m_settings.worldSaveInterval, m_settings.worldSaveInterval + 30s, {
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

void IValhalla::Broadcast(UIMsgType type, const std::string& text) {
    RouteManager()->InvokeAll(Hashes::Routed::S2C_UIMessage, type, text);
}
