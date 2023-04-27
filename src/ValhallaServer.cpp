#include <yaml-cpp/yaml.h>

#include <stdlib.h>
#include <utility>
#ifdef _WIN32
#include <winstring.h>
#endif

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
#include "RandomEventManager.h"
#include "DiscordManager.h"

auto VALHALLA_INSTANCE(std::make_unique<IValhalla>());
IValhalla* Valhalla() {
    return VALHALLA_INSTANCE.get();
}



void IValhalla::LoadFiles(bool reloading) {
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
                if (!reloading) {
                    LOG(INFO) << "Server config not found, creating...";
                }
                fileError = true;
            }
        }

        // TODO:
        // consider moving extraneous features into a separate lua mod group
        //  to not pollute the primary base server code
        //  crucial quality of life features are fine however to remain in c++  (zdos/send rates/...)

        if (!reloading || !fileError) {
            auto&& server = loadNode["server"];
            auto&& discord = loadNode["discord"];
            auto&& world = loadNode["world"];
            auto&& player = loadNode["player"];
            auto&& zdo = loadNode["zdo"];
            auto&& spawning = loadNode["spawning"];
            auto&& dungeons = loadNode["dungeons"];
            auto&& events = loadNode["events"];

            m_settings.serverName = VUtils::String::ToAscii(server["name"].as<std::string>(""));
            if (m_settings.serverName.empty()
                || m_settings.serverName.length() < 3
                || m_settings.serverName.length() > 64) m_settings.serverName = "Valhalla server";

            m_settings.serverPassword = server["password"].as<std::string>("s");
            if (!m_settings.serverPassword.empty()
                && (m_settings.serverPassword.length() < 5
                    || m_settings.serverPassword.length() > 11)) m_settings.serverPassword = VUtils::Random::GenerateAlphaNum(6);

            if (!reloading) {
                m_settings.serverPort = server["port"].as<uint16_t>(2456);
            
                m_settings.serverPublic = server["public"].as<bool>(false);
                m_settings.serverDedicated = server["dedicated"].as<bool>(true);

                m_settings.worldName = VUtils::String::ToAscii(world["name"].as<std::string>(""));
                if (m_settings.worldName.empty() || m_settings.worldName.length() < 3) m_settings.worldName = "world";
                m_settings.worldSeed = world["seed"].as<std::string>("");
                if (m_settings.worldSeed.empty()) m_settings.worldSeed = VUtils::Random::GenerateAlphaNum(10);
                m_settings.worldModern = world["modern"].as<bool>(true);
                m_settings.worldCaptureMode = (WorldMode) world["capture-mode"].as<std::underlying_type_t<WorldMode>>(std::to_underlying(WorldMode::NORMAL));
                m_settings.worldCaptureSize = std::clamp(world["capture-size-bytes"].as<size_t>(256000ULL), 64000ULL, 256000000ULL);
                m_settings.worldCaptureSession = world["capture-session"].as<int>(-1);
                m_settings.worldPlaybackSession = world["playback-session"].as<int>(-1);

                if (m_settings.worldCaptureMode == WorldMode::CAPTURE)
                    m_settings.worldCaptureSession++;
            }

            m_settings.discordWebhook = discord["webhook"].as<std::string>("");

            m_settings.worldSaveInterval = minutes(std::clamp(world["save-interval-m"].as<int>(1800), 0, 60 * 24 * 7));

            m_settings.playerWhitelist = player["whitelist"].as<bool>(false);          // enable whitelist
            m_settings.playerMax = std::clamp(player["max"].as<int>(10), 1, 99999);     // max allowed players
            m_settings.playerAuth = player["auth"].as<bool>(true);                     // allow authed players only
            m_settings.playerTimeout = seconds(player["timeout-s"].as<int>(30));
            m_settings.playerListSendInterval = milliseconds(std::clamp(player["player-list-send-interval-ms"].as<int>(2000), 0, 1000*10));

            m_settings.zdoMaxCongestion = zdo["max-congestion"].as<int>(10240);
            m_settings.zdoMinCongestion = zdo["min-congestion"].as<int>(2048);
            m_settings.zdoSendInterval = milliseconds(zdo["send-interval-ms"].as<int>(50));
            m_settings.zdoAssignInterval = seconds(std::clamp(zdo["assign-interval-s"].as<int>(2), 1, 60));
            m_settings.zdoAssignAlgorithm = (AssignAlgorithm) zdo["assign-algorithm"].as<int>(std::to_underlying(AssignAlgorithm::NONE));

            m_settings.spawningCreatures = spawning["creatures"].as<bool>(true);
            m_settings.spawningLocations = spawning["locations"].as<bool>(true);
            m_settings.spawningVegetation = spawning["vegetation"].as<bool>(true);
            m_settings.spawningDungeons = spawning["dungeons"].as<bool>(true);

            m_settings.dungeonEndCaps = dungeons["end-caps"].as<bool>(true);
            m_settings.dungeonDoors = dungeons["doors"].as<bool>(true);
            m_settings.dungeonFlipRooms = dungeons["flip-rooms"].as<bool>(true);
            m_settings.dungeonZoneLimit = dungeons["zone-limit"].as<bool>(true);
            m_settings.dungeonRoomShrink = dungeons["room-shrink"].as<bool>(true);
            m_settings.dungeonReset = dungeons["reset"].as<bool>(true);
            m_settings.dungeonResetTime = seconds(std::max(60, dungeons["reset-time-s"].as<int>(3600 * 72)));
            //m_settings.dungeonIncrementalResetTime = seconds(std::max(1, loadNode["dungeon-incremental-reset-time-s"].as<int>(5)));
            m_settings.dungeonIncrementalResetCount = std::min(20, dungeons["incremental-reset-count"].as<int>(3));
            m_settings.dungeonSeededRandom = dungeons["seeded-random"].as<bool>(true); // TODO rename seeded

            m_settings.eventsEnabled = events["enabled"].as<bool>(true);
            m_settings.eventsChance = std::clamp(events["chance"].as<float>(.2f), 0.f, 1.f);
            m_settings.eventsInterval = minutes(std::max(1, events["interval-m"].as<int>(46)));
            m_settings.eventsRange = std::clamp(events["range"].as<float>(96), 1.f, 96.f * 4);

            if (m_settings.serverPassword.empty())
                LOG(WARNING) << "Server does not have a password";
            else
                LOG(INFO) << "Server password is '" << m_settings.serverPassword << "'";

            if (m_settings.worldCaptureMode == WorldMode::CAPTURE) {
                LOG(WARNING) << "Experimental packet capture enabled";
            }
            else if (m_settings.worldCaptureMode == WorldMode::PLAYBACK) {
                LOG(WARNING) << "Experimental packet playback enabled";
            }
        }
    }
    
    LOG(INFO) << "Server config loaded";

    //if (!reloading && fileError) {
    if (!reloading) {
        YAML::Node saveNode;

        auto&& server = saveNode["server"];
        auto&& discord = saveNode["discord"];
        auto&& world = saveNode["world"];
        auto&& player = saveNode["player"];
        auto&& zdo = saveNode["zdo"];
        auto&& spawning = saveNode["spawning"];
        auto&& dungeons = saveNode["dungeons"];
        auto&& events = saveNode["events"];
        
        server["name"] = m_settings.serverName;
        server["port"] = m_settings.serverPort;
        server["password"] = m_settings.serverPassword;
        server["public"] = m_settings.serverPublic;
        server["dedicated"] = m_settings.serverDedicated;

        discord["webhook"] = m_settings.discordWebhook;

        world["name"] = m_settings.worldName;
        world["seed"] = m_settings.worldSeed;
        world["save-interval-m"] = duration_cast<minutes>(m_settings.worldSaveInterval).count();
        world["modern"] = m_settings.worldModern;
        world["capture-mode"] = std::to_underlying(m_settings.worldCaptureMode);
        world["capture-size-bytes"] = m_settings.worldCaptureSize;
        world["capture-session"] = m_settings.worldCaptureSession;
        world["playback-session"] = m_settings.worldPlaybackSession;

        player["whitelist"] = m_settings.playerWhitelist;
        player["max"] = m_settings.playerMax;
        player["auth"] = m_settings.playerAuth;
        player["timeout-s"] = duration_cast<seconds>(m_settings.playerTimeout).count();
        player["player-list-send-interval-ms"] = duration_cast<milliseconds>(m_settings.playerListSendInterval).count();

        zdo["max-congestion"] = m_settings.zdoMaxCongestion;
        zdo["min-congestion"] = m_settings.zdoMinCongestion;
        zdo["send-interval-ms"] = duration_cast<milliseconds>(m_settings.zdoSendInterval).count();
        zdo["assign-interval-s"] = duration_cast<seconds>(m_settings.zdoAssignInterval).count();
        zdo["assign-algorithm"] = std::to_underlying(m_settings.zdoAssignAlgorithm);

        spawning["creatures"] = m_settings.spawningCreatures;
        spawning["locations"] = m_settings.spawningLocations;
        spawning["vegetation"] = m_settings.spawningVegetation;
        spawning["dungeons"] = m_settings.spawningDungeons;

        dungeons["end-caps"] = m_settings.dungeonEndCaps;
        dungeons["doors"] = m_settings.dungeonDoors;
        dungeons["flip-rooms"] = m_settings.dungeonFlipRooms;
        dungeons["zone-limit"] = m_settings.dungeonZoneLimit;
        dungeons["room-shrink"] = m_settings.dungeonRoomShrink;
        dungeons["reset"] = m_settings.dungeonReset;
        dungeons["reset-time-s"] = duration_cast<seconds>(m_settings.dungeonResetTime).count();
        //saveNode["dungeon-incremental-reset-time-s"] = m_settings.dungeonIncrementalResetTime.count();
        dungeons["incremental-reset-count"] = m_settings.dungeonIncrementalResetCount;
        dungeons["seeded-random"] = m_settings.dungeonSeededRandom;

        events["enabled"] = m_settings.eventsEnabled;
        events["chance"] = m_settings.eventsChance;
        events["interval-m"] = duration_cast<minutes>(m_settings.eventsInterval).count();
        events["range"] = m_settings.eventsRange;

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

    if (reloading) {
        // then iterate players, settings active and inactive
        for (auto&& peer : NetManager()->GetPeers()) {
            peer->m_admin = m_admin.contains(peer->m_name);
        }
    }

    NetManager()->OnConfigLoad(reloading);

#ifdef _WIN32
    {
        //std::string title = m_settings.serverName + " - " + VConstants::GAME;
        std::string title = "Valhalla " + std::string(VH_SERVER_VERSION) + " - Valheim " + std::string(VConstants::GAME);
        SetConsoleTitle(title.c_str());
    }
#endif

    std::error_code err;
    this->m_settingsLastTime = fs::last_write_time("server.yml", err);
}

void IValhalla::Stop() {
    m_terminate = true;

    // prevent deadlock
    if (el::Helpers::getThreadName() != "main")
        m_terminate.wait(true);
}

void IValhalla::Start() {
    LOG(INFO) << "Starting Valhalla " << VH_SERVER_VERSION << " (Valheim " << VConstants::GAME << ")";
    
    m_serverID = VUtils::Random::GenerateUID();
    m_startTime = steady_clock::now();

    this->LoadFiles(false);

    //m_worldTime = 2040;
    m_worldTime = GetMorning(1);

    m_serverTimeMultiplier = 1;

    ZDOManager()->Init();
    RandomEventManager()->Init();
    PrefabManager()->Init();

    ZoneManager()->PostPrefabInit();
    DungeonManager()->PostPrefabInit();

    WorldManager()->PostZoneInit();
    GeoManager()->PostWorldInit();
    HeightmapBuilder()->PostGeoInit();
    ZoneManager()->PostGeoInit();

    WorldManager()->PostInit();
    NetManager()->PostInit();
    ModManager()->PostInit();

    /*
    if (VH_SETTINGS.worldRecording) {
        World* world = WorldManager()->GetWorld();
        VUtils::Resource::WriteFile(
            fs::path(VH_CAPTURE_PATH) / world->m_name / (world->m_name + ".db"),
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
        //FrameMarkStart("mainloop");

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

        //FrameMarkEnd("mainloop");

        std::this_thread::sleep_for(1ms);

        FrameMark;
    }
            
    LOG(INFO) << "Terminating server";

    // Cleanup 
    NetManager()->Uninit();
    HeightmapBuilder()->Uninit();

    ModManager()->Uninit();

    if (VH_SETTINGS.worldCaptureMode != WorldMode::PLAYBACK)
        WorldManager()->GetWorld()->WriteFiles();

    VUtils::Resource::WriteFile("blacklist.txt", m_blacklist);
    VUtils::Resource::WriteFile("whitelist.txt", m_whitelist);
    VUtils::Resource::WriteFile("admin.txt", m_admin);
    //VUtils::Resource::WriteFile("bypass.txt", m_bypass);

    LOG(INFO) << "Server was gracefully terminated";

    // signal any other dummy thread to continue
    m_terminate = false;
}



void IValhalla::Update() {
    ZoneScoped;

    // This is important to processing RPC remote invocations

    if (!NetManager()->GetPeers().empty()) {
        m_worldTime += Delta() * m_worldTimeMultiplier;
    }
    
    VH_DISPATCH_MOD_EVENT(IModManager::Events::Update);

    NetManager()->Update();
    ZDOManager()->Update();
    ZoneManager()->Update();
    RandomEventManager()->Update();
    HeightmapBuilder()->Update();
}

void IValhalla::PeriodUpdate() {
    PERIODIC_NOW(180s, {
        LOG(INFO) << "There are a total of " << NetManager()->GetPeers().size() << " peers online";
    });

    VH_DISPATCH_MOD_EVENT(IModManager::Events::PeriodicUpdate);

    if (m_settings.dungeonReset)
        DungeonManager()->TryRegenerateDungeons();
    
    std::error_code err;
    auto lastWriteTime = fs::last_write_time("server.yml", err);
    if (lastWriteTime != this->m_settingsLastTime) {
        // reload the file
        LoadFiles(true);
    }

    if (m_settings.worldCaptureMode == WorldMode::PLAYBACK) {
        PERIODIC_NOW(333ms, {
            char message[32];
            std::sprintf(message, "World playback %.2fs", (duration_cast<milliseconds>(Valhalla()->Nanos()).count() / 1000.f));
            Broadcast(UIMsgType::TopLeft, message);
        });
    }
   
    PERIODIC_NOW(5s, {
        DiscordManager()->SendSimpleMessage("Hello Valhalla!");
    });

    if (m_settings.worldSaveInterval > 0s) {
        // save warming message
        PERIODIC_LATER(m_settings.worldSaveInterval, m_settings.worldSaveInterval, {
            LOG(INFO) << "World saving in 30s";
            Broadcast(UIMsgType::Center, "$msg_worldsavewarning 30s");
        });

        PERIODIC_LATER(m_settings.worldSaveInterval, m_settings.worldSaveInterval + 30s, {
            WorldManager()->GetWorld()->WriteFiles();
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
    RouteManager()->InvokeAll(Hashes::Routed::S2C_UIMessage, type, std::string_view(text));
}
