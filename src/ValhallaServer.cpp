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


            auto a = [](YAML::Node& node, std::string_view key, auto&& def, std::string_view comment = "") {
                auto&& mapping = node[key];

                try {
                    return mapping.as<decltype(def)>();
                }
                catch (const std::exception& e) {
                    mapping = def;
                }

                assert(node[key].IsDefined());

                return def;
            };

            //std::string k = a(loadNode, "mykey", "mydef");

            auto&& server = loadNode[VH_SETTING_KEY_SERVER];
            auto&& discord = loadNode[VH_SETTING_KEY_DISCORD];
            auto&& world = loadNode[VH_SETTING_KEY_WORLD];
            auto&& packet = loadNode[VH_SETTING_KEY_PACKET];
            auto&& player = loadNode[VH_SETTING_KEY_PLAYER];
            auto&& zdo = loadNode[VH_SETTING_KEY_ZDO];
            auto&& dungeons = loadNode[VH_SETTING_KEY_DUNGEONS];
            auto&& events = loadNode[VH_SETTING_KEY_EVENTS];

            //m_settings.serverName = VUtils::String::ToAscii(server[VH_SETTING_KEY_SERVER_NAME].as<std::string>(""));
            m_settings.serverName = server[VH_SETTING_KEY_SERVER_NAME].as<std::string>("");
            if (m_settings.serverName.empty() || m_settings.serverName.length() < 3 || m_settings.serverName.length() > 64) 
                m_settings.serverName = "Valhalla server";

            m_settings.serverPassword = server[VH_SETTING_KEY_SERVER_PASSWORD].as<std::string>("");
            if (!m_settings.serverPassword.empty() && (m_settings.serverPassword.length() < 5 || m_settings.serverPassword.length() > 11)) m_settings.serverPassword = VUtils::Random::GenerateAlphaNum(6);

            if (!reloading) {
                m_settings.serverPort = server[VH_SETTING_KEY_SERVER_PORT].as<uint16_t>(2456);
            
                m_settings.serverPublic = server[VH_SETTING_KEY_SERVER_PUBLIC].as<bool>(false);
                m_settings.serverDedicated = server[VH_SETTING_KEY_SERVER_DEDICATED].as<bool>(true);

                m_settings.worldName = VUtils::String::ToAscii(world[VH_SETTING_KEY_WORLD].as<std::string>(""));
                if (m_settings.worldName.empty() || m_settings.worldName.length() < 3) m_settings.worldName = "world";
                m_settings.worldSeed = world[VH_SETTING_KEY_WORLD_SEED].as<std::string>("");
                if (m_settings.worldSeed.empty()) m_settings.worldSeed = VUtils::Random::GenerateAlphaNum(10);
                m_settings.worldModern = world[VH_SETTING_KEY_WORLD_MODERN].as<bool>(true);

                m_settings.packetMode = (PacketMode) packet[VH_SETTING_KEY_PACKET_MODE].as<std::underlying_type_t<PacketMode>>(std::to_underlying(PacketMode::NORMAL));
                m_settings.packetFileUpperSize = std::clamp(world[VH_SETTING_KEY_PACKET_FILE_UPPER_SIZE].as<size_t>(256000ULL), 64000ULL, 256000000ULL);
                m_settings.packetCaptureSessionIndex = world[VH_SETTING_KEY_PACKET_CAPTURE_SESSION_INDEX].as<int>(-1);
                m_settings.packetPlaybackSessionIndex = world[VH_SETTING_KEY_PACKET_PLAYBACK_SESSION_INDEX].as<int>(-1);

                if (m_settings.packetMode == PacketMode::CAPTURE)
                    m_settings.packetCaptureSessionIndex++;
            }

            m_settings.discordWebhook = discord[VH_SETTING_KEY_DISCORD_WEBHOOK].as<std::string>("");

            m_settings.worldFeatures = world[VH_SETTING_KEY_WORLD_FEATURES].as<bool>(true);
            m_settings.worldVegetation = world[VH_SETTING_KEY_WORLD_VEGETATION].as<bool>(true);
            m_settings.worldCreatures = world[VH_SETTING_KEY_WORLD_CREATURES].as<bool>(true);           

            m_settings.worldSaveInterval = seconds(std::clamp(world[VH_SETTING_KEY_WORLD_SAVE_INTERVAL].as<int>(1800), 0, 60 * 60 * 24 * 7));

            m_settings.playerWhitelist = player[VH_SETTING_KEY_PLAYER_WHITELIST].as<bool>(true);          // enable whitelist
            m_settings.playerMax = std::max(player[VH_SETTING_KEY_PLAYER_MAX].as<int>(10), 1);     // max allowed players
            m_settings.playerOffline = player[VH_SETTING_KEY_PLAYER_OFFLINE].as<bool>(true);                     // allow authed players only
            m_settings.playerTimeout = seconds(std::clamp(player[VH_SETTING_KEY_PLAYER_TIMEOUT].as<int>(30), 1, 60*60));
            m_settings.playerListSendInterval = milliseconds(std::clamp(player[VH_SETTING_KEY_PLAYER_LIST_SEND_INTERVAL].as<int>(2000), 0, 1000*10));
            m_settings.playerListForceVisible = player[VH_SETTING_KEY_PLAYER_LIST_FORCE_VISIBLE].as<bool>(false);

            m_settings.zdoMaxCongestion = zdo[VH_SETTING_KEY_ZDO_MAX_CONGESTION].as<int>(10240);
            m_settings.zdoMinCongestion = zdo[VH_SETTING_KEY_ZDO_MIN_CONGESTION].as<int>(2048);
            m_settings.zdoSendInterval = milliseconds(zdo[VH_SETTING_KEY_ZDO_SEND_INTERVAL].as<int>(50));
            m_settings.zdoAssignInterval = seconds(std::clamp(zdo[VH_SETTING_KEY_ZDO_ASSIGN_INTERVAL].as<int>(2), 1, 60));
            m_settings.zdoAssignAlgorithm = (AssignAlgorithm) zdo[VH_SETTING_KEY_ZDO_ASSIGN_ALGORITHM].as<int>(std::to_underlying(AssignAlgorithm::NONE));
                        
            m_settings.dungeonsEnabled = dungeons[VH_SETTING_KEY_DUNGEONS_ENABLED].as<bool>(true);
            {
                auto&& endcaps = dungeons[VH_SETTING_KEY_DUNGEONS_ENDCAPS];
                m_settings.dungeonsEndcapsEnabled = endcaps[VH_SETTING_KEY_DUNGEONS_ENDCAPS_ENABLED].as<bool>(true);
                m_settings.dungeonsEndcapsInsetFrac = std::clamp(endcaps[VH_SETTING_KEY_DUNGEONS_ENDCAPS_INSETFRAC].as<float>(.5f), 0.f, 1.f);
            }

            m_settings.dungeonsDoors = dungeons[VH_SETTING_KEY_DUNGEONS_DOORS].as<bool>(true);

            {
                auto&& rooms = dungeons[VH_SETTING_KEY_DUNGEONS_ROOMS];
                m_settings.dungeonsRoomsFlipped = rooms[VH_SETTING_KEY_DUNGEONS_ROOMS_FLIPPED].as<int>(true);
                m_settings.dungeonsRoomsZoneBounded = rooms[VH_SETTING_KEY_DUNGEONS_ROOMS_ZONEBOUNDED].as<int>(true);
                m_settings.dungeonsRoomsInsetSize = std::max(rooms[VH_SETTING_KEY_DUNGEONS_ROOMS_INSETSIZE].as<float>(.1f), 0.f);
            }

            {
                auto&& regeneration = dungeons[VH_SETTING_KEY_DUNGEONS_REGENERATION];
                m_settings.dungeonsRegenerationInterval = seconds(std::clamp(regeneration[VH_SETTING_KEY_DUNGEONS_REGENERATION_INTERVAL].as<int64_t>(60LL*60LL*24LL*3LL), 0LL, 60LL*60LL*24LL*30LL));
                m_settings.dungeonsRegenerationMaxSteps = std::max(regeneration[VH_SETTING_KEY_DUNGEONS_REGENERATION_MAXSTEP].as<int>(3), 0);
            }
            m_settings.dungeonsSeeded = dungeons[VH_SETTING_KEY_DUNGEONS_SEEDED].as<bool>(true); // TODO rename seeded

            m_settings.eventsChance = std::clamp(events[VH_SETTING_KEY_EVENTS_CHANCE].as<float>(.2f), 0.f, 1.f);
            m_settings.eventsInterval = seconds(std::max(0, events[VH_SETTING_KEY_EVENTS_INTERVAL].as<int>(60 * 46)));
            m_settings.eventsRadius = std::clamp(events[VH_SETTING_KEY_EVENTS_RADIUS].as<float>(96), 1.f, 96.f * 4);
            m_settings.eventsRequireKeys = events[VH_SETTING_KEY_DUNGEONS_SEEDED].as<bool>(true);

            if (m_settings.serverPassword.empty())
                LOG(WARNING) << "Server does not have a password";
            else
                LOG(INFO) << "Server password is '" << m_settings.serverPassword << "'";

            if (m_settings.packetMode == PacketMode::CAPTURE) {
                LOG(WARNING) << "Experimental packet capture enabled";
            }
            else if (m_settings.packetMode == PacketMode::PLAYBACK) {
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
        world["capture-mode"] = std::to_underlying(m_settings.packetMode);
        world["capture-size-bytes"] = m_settings.packetFileUpperSize;
        world["capture-session"] = m_settings.packetCaptureSessionIndex;
        world["playback-session"] = m_settings.packetPlaybackSessionIndex;

        player["whitelist"] = m_settings.playerWhitelist;
        player["max"] = m_settings.playerMax;
        player["auth"] = m_settings.playerOffline;
        player["timeout-s"] = duration_cast<seconds>(m_settings.playerTimeout).count();
        player["player-list-send-interval-ms"] = duration_cast<milliseconds>(m_settings.playerListSendInterval).count();

        zdo["max-congestion"] = m_settings.zdoMaxCongestion;
        zdo["min-congestion"] = m_settings.zdoMinCongestion;
        zdo["send-interval-ms"] = duration_cast<milliseconds>(m_settings.zdoSendInterval).count();
        zdo["assign-interval-s"] = duration_cast<seconds>(m_settings.zdoAssignInterval).count();
        zdo["assign-algorithm"] = std::to_underlying(m_settings.zdoAssignAlgorithm);

        spawning["creatures"] = m_settings.worldCreatures;
        spawning["locations"] = m_settings.worldFeatures;
        spawning["vegetation"] = m_settings.worldVegetation;
        spawning["dungeons"] = m_settings.dungeonsEnabled;

        dungeons["end-caps"] = m_settings.dungeonsEndcapsEnabled;
        dungeons["doors"] = m_settings.dungeonsDoors;
        dungeons["flip-rooms"] = m_settings.dungeonsRoomsFlipped;
        dungeons["zone-limit"] = m_settings.dungeonsRoomsZoneBounded;
        dungeons["room-shrink"] = m_settings.dungeonsRoomsInsetSize;
        //dungeons["reset"] = m_settings.dungeonsRegenerationEnabled;
        dungeons["reset-time-s"] = duration_cast<seconds>(m_settings.dungeonsRegenerationInterval).count();
        //saveNode["dungeon-incremental-reset-time-s"] = m_settings.dungeonIncrementalResetTime.count();
        dungeons["incremental-reset-count"] = m_settings.dungeonsRegenerationMaxSteps;
        dungeons["seeded-random"] = m_settings.dungeonsSeeded;

        events["enabled"] = m_settings.eventsEnabled;
        events["chance"] = m_settings.eventsChance;
        events["interval-m"] = duration_cast<minutes>(m_settings.eventsInterval).count();
        events["range"] = m_settings.eventsRadius;

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

    VH_DISPATCH_WEBHOOK("Server started");

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

    VH_DISPATCH_WEBHOOK("Server stopping");
            
    LOG(INFO) << "Terminating server";

    // Cleanup 
    NetManager()->Uninit();
    HeightmapBuilder()->Uninit();

    ModManager()->Uninit();

    if (VH_SETTINGS.packetMode != WorldMode::PLAYBACK)
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

    if (m_settings.dungeonsRegenerationEnabled)
        DungeonManager()->TryRegenerateDungeons();
    
    std::error_code err;
    auto lastWriteTime = fs::last_write_time("server.yml", err);
    if (lastWriteTime != this->m_settingsLastTime) {
        // reload the file
        LoadFiles(true);
    }

    if (m_settings.packetMode == WorldMode::PLAYBACK) {
        PERIODIC_NOW(333ms, {
            char message[32];
            std::sprintf(message, "World playback %.2fs", (duration_cast<milliseconds>(Valhalla()->Nanos()).count() / 1000.f));
            Broadcast(UIMsgType::TopLeft, message);
        });
    }
    /*
    PERIODIC_NOW(5s, {
        DiscordManager()->SendSimpleMessage("Hello Valhalla!");
    });*/

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
