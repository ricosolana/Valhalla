#include <yaml-cpp/yaml.h>

#include <stdlib.h>
#include <utility>
#include <charconv>
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

namespace YAML {
    template<>
    struct convert<PacketMode> {
        static Node encode(const PacketMode& rhs) {
            return Node(std::to_underlying(rhs));
        }

        static bool decode(const Node& node, PacketMode& rhs) {
            if (!node.IsScalar())
                return false;

            rhs = PacketMode(node.as<std::underlying_type_t<PacketMode>>());

            return true;
        }
    };

    template<>
    struct convert<AssignAlgorithm> {
        static Node encode(const AssignAlgorithm& rhs) {
            return Node(std::to_underlying(rhs));
        }

        static bool decode(const Node& node, AssignAlgorithm& rhs) {
            if (!node.IsScalar())
                return false;

            rhs = AssignAlgorithm(node.as<std::underlying_type_t<AssignAlgorithm>>());

            return true;
        }
    };





    template<typename T>
    static bool parseDuration(const std::string& s, T& out) {
        int64_t dur = 0;
        size_t index = 0;
        for (; index < s.length(); index++) {
            const int64_t ch = (int64_t)s[index];
            if (ch >= '0' && ch <= '9')
                dur += (ch - '0') * (index + 1) * 10;
            else if (index > 0) {
                switch (ch) {
                case 'n': out = duration_cast<T>(nanoseconds(dur)); return true;
                case 'u': out = duration_cast<T>(microseconds(dur)); return true;
                case 'm': out = duration_cast<T>(milliseconds(dur)); return true;
                case 's': out = duration_cast<T>(seconds(dur)); return true;
                case 'M': out = duration_cast<T>(minutes(dur)); return true;
                case 'h': out = duration_cast<T>(hours(dur)); return true;
                case 'd': out = duration_cast<T>(days(dur)); return true;
                case 'w': out = duration_cast<T>(weeks(dur)); return true;
                case 'o': out = duration_cast<T>(months(dur)); return true;
                case 'y': out = duration_cast<T>(years(dur)); return true;
                }
                break;
            }
        }
        out = T(dur);
        return false;
    };

    template<>
    struct convert<nanoseconds> {
        static Node encode(const nanoseconds& rhs) {
            return Node(std::to_string(rhs.count()) + "ns");
        }

        static bool decode(const Node& node, nanoseconds& rhs) {
            if (!node.IsScalar())
                return false;

            auto&& s = node.Scalar();

            return parseDuration(node.Scalar(), rhs);
        }
    };

    template<>
    struct convert<microseconds> {
        static Node encode(const microseconds& rhs) {
            return Node(std::to_string(rhs.count()) + "us");
        }

        static bool decode(const Node& node, microseconds& rhs) {
            if (!node.IsScalar())
                return false;

            auto&& s = node.Scalar();

            return parseDuration(node.Scalar(), rhs);
        }
    };

    template<>
    struct convert<milliseconds> {
        static Node encode(const milliseconds& rhs) {
            return Node(std::to_string(rhs.count()) + "ms");
        }

        static bool decode(const Node& node, milliseconds& rhs) {
            if (!node.IsScalar())
                return false;

            auto&& s = node.Scalar();

            return parseDuration(node.Scalar(), rhs);
        }
    };

    template<>
    struct convert<seconds> {
        static Node encode(const seconds& rhs) {
            return Node(std::to_string(rhs.count()) + "s");
        }

        static bool decode(const Node& node, seconds& rhs) {
            if (!node.IsScalar())
                return false;

            auto&& s = node.Scalar();

            return parseDuration(node.Scalar(), rhs);
        }
    };

    template<>
    struct convert<minutes> {
        static Node encode(const minutes& rhs) {
            return Node(std::to_string(rhs.count()) + "Min");
        }

        static bool decode(const Node& node, minutes& rhs) {
            if (!node.IsScalar())
                return false;

            auto&& s = node.Scalar();

            return parseDuration(node.Scalar(), rhs);
        }
    };

    template<>
    struct convert<hours> {
        static Node encode(const hours& rhs) {
            return Node(std::to_string(rhs.count()) + "hours");
        }

        static bool decode(const Node& node, hours& rhs) {
            if (!node.IsScalar())
                return false;

            auto&& s = node.Scalar();

            return parseDuration(node.Scalar(), rhs);
        }
    };

    template<>
    struct convert<days> {
        static Node encode(const days& rhs) {
            return Node(std::to_string(rhs.count()) + "days");
        }

        static bool decode(const Node& node, days& rhs) {
            if (!node.IsScalar())
                return false;

            auto&& s = node.Scalar();

            return parseDuration(node.Scalar(), rhs);
        }
    };

    template<>
    struct convert<weeks> {
        static Node encode(const weeks& rhs) {
            return Node(std::to_string(rhs.count()) + "weeks");
        }

        static bool decode(const Node& node, weeks& rhs) {
            if (!node.IsScalar())
                return false;

            auto&& s = node.Scalar();

            return parseDuration(node.Scalar(), rhs);
        }
    };

    template<>
    struct convert<months> {
        static Node encode(const months& rhs) {
            return Node(std::to_string(rhs.count()) + "onths");
        }

        static bool decode(const Node& node, months& rhs) {
            if (!node.IsScalar())
                return false;

            auto&& s = node.Scalar();

            return parseDuration(node.Scalar(), rhs);
        }
    };

    template<>
    struct convert<years> {
        static Node encode(const years& rhs) {
            return Node(std::to_string(rhs.count()) + "years");
        }

        static bool decode(const Node& node, years& rhs) {
            if (!node.IsScalar())
                return false;

            auto&& s = node.Scalar();

            return parseDuration(node.Scalar(), rhs);
        }
    };
}

template<class T>
struct is_duration : std::false_type {};

template<class Rep, class Period>
struct is_duration<duration<Rep, Period>> : std::true_type {};

// Retrieve a config value
//  Returns the value or the default
//  The key will be set in the config
//  Accepts an optional predicate for whether to use the default value
//template<typename T, typename Func = decltype([](const T&) -> bool {})>



template<typename T, typename Def, typename Func = std::nullptr_t>
    requires (std::is_same_v<Func, std::nullptr_t> 
    //|| (std::tuple_size<typename VUtils::Traits::func_traits<Func>::args_type>{} == 1 
        //&& is_duration<typename std::tuple_element_t<0, typename VUtils::Traits::func_traits<Func>::args_type>>::value == is_duration<T>::value))

    || ((is_duration<typename std::tuple_element_t<0, typename VUtils::Traits::func_traits<Func>::args_type>>::value && is_duration<T>::value) == is_duration<Def>::value))
    //|| (is_duration<typename std::tuple_element_t<0, typename VUtils::Traits::func_traits<Func>::args_type>>::value == is_duration<T>::value))
void a(T& set, YAML::Node node, const std::string& key, Def def, Func defPred = nullptr, bool reloading = false, std::string comment = "") {
    static constexpr auto T_IS_DUR = is_duration<T>::value;
        
    if (reloading)
        return;

    auto&& mapping = node[key];
    
    try {
        auto&& val = mapping.as<T>();

        if constexpr (!std::is_same_v<Func, std::nullptr_t>) {
            using Param0 = std::tuple_element_t<0, typename VUtils::Traits::func_traits<Func>::args_type>;

            if constexpr (T_IS_DUR) {
                if (!defPred || !defPred(duration_cast<Param0>(val))) {
                    set = val;
                    //if (!comment.empty()) emitter << YAML::Comment(std::string(comment));
                    //emitter << YAML::
                    return;
                }
            }
            else {
                if (!defPred || !defPred(static_cast<Param0>(val))) {
                    set = val;
                    return;
                }
            }
        }
        else {
            set = val;
            return;
        }
    }
    catch (const YAML::Exception&) {}

    mapping = def;
    
    assert(node[key].IsDefined());
    
    if constexpr (T_IS_DUR) {
        set = duration_cast<T>(def);
    }
    else
        set = T(def);
};

void IValhalla::LoadFiles(bool reloading) {
    bool fileError = false;
    
    {
        YAML::Node node;
        {
            if (auto opt = VUtils::Resource::ReadFile<std::string>("server.yml")) {
                try {
                    node = YAML::Load(opt.value());
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

        if (!reloading || !fileError) {
            auto&& server = node["server"];
            auto&& player = node["players"];
            auto&& world = node["world"];
            auto&& zdo = node["zdos"];
            auto&& dungeons = node["dungeons"];
            auto&& events = node["events"];
            auto&& packet = node["packets"];
            auto&& discord = node["discord"];

            a(m_settings.serverName, server, "name", "Valhalla server", [](const std::string& val) { return val.empty() || val.length() < 3 || val.length() > 64; });
            a(m_settings.serverPassword, server, "password", "secret", [](const std::string& val) { return !val.empty() && (val.length() < 5 || val.length() > 11); });
            a(m_settings.serverPort, server, "port", 2456, nullptr, reloading);
            a(m_settings.serverPublic, server, "public", false, nullptr, reloading);
            a(m_settings.serverDedicated, server, "dedicated", true, nullptr, reloading);

            a(m_settings.playerWhitelist, player, "whitelist", true);
            a(m_settings.playerMax, player, "max", 10, [](int val) { return val < 1; });
            a(m_settings.playerOnline, player, "offline", true);
            a(m_settings.playerTimeout, player, "timeout", 30s, [](seconds val) { return val < 0s || val > 1h; });
            a(m_settings.playerListSendInterval, player, "list-send-interval", 2s, [](seconds val) { return val < 0s; });
            a(m_settings.playerListForceVisible, player, "list-force-visible", false);

            a(m_settings.worldName, world, "world", "world", [](const std::string& val) { return val.empty() || val.length() < 3; }, reloading);
            a(m_settings.worldSeed, world, "seed", VUtils::Random::GenerateAlphaNum(10), [](const std::string& val) { return val.empty(); }, reloading);
            a(m_settings.worldPregenerate, world, "pregenerate", false, nullptr, reloading);
            a(m_settings.worldSaveInterval, world, "save-interval", 30min, [](seconds val) { return val < 0s; });
            a(m_settings.worldModern, world, "modern", true, nullptr, reloading);
            a(m_settings.worldFeatures, world, "features", true);
            a(m_settings.worldVegetation, world, "vegetation", true);
            a(m_settings.worldCreatures, world, "creatures", true);

            if (m_settings.packetMode == PacketMode::CAPTURE)
                m_settings.packetCaptureSessionIndex++;           
            
            a(m_settings.zdoSendInterval, zdo, "send-interval", 50ms, [](seconds val) { return val <= 0s || val > 1s; });
            a(m_settings.zdoMaxCongestion, zdo, "max-send-threshold", 10240, [](int val) { return val < 1000; });
            a(m_settings.zdoMinCongestion, zdo, "min-send-threshold", 2048, [](int val) { return val < 1000; });
            a(m_settings.zdoAssignInterval, zdo, "assign-interval", 2s, [](seconds val) { return val <= 0s || val > 10s; });
            a(m_settings.zdoAssignAlgorithm, zdo, "assign-algorithm", AssignAlgorithm::NONE);
            
            a(m_settings.dungeonsEnabled, dungeons, "enabled", true);
            {
                auto&& endcaps = dungeons["endcaps"];
                a(m_settings.dungeonsEndcapsEnabled, endcaps, "enabled", true);
                a(m_settings.dungeonsEndcapsInsetFrac, endcaps, "inset-ratio", .5f, [](float val) { return val < 0.f || val > 1.f; });
            }

            a(m_settings.dungeonsDoors, dungeons, "doors", true);

            {
                auto&& rooms = dungeons["rooms"];
                a(m_settings.dungeonsRoomsFlipped, rooms, "flipped", true);
                a(m_settings.dungeonsRoomsZoneBounded, rooms, "zone-bounded", true);
                a(m_settings.dungeonsRoomsInsetSize, rooms, "inset-size", .1f, [](float val) { return val < 0; });
                a(m_settings.dungeonsRoomsFurnishing, rooms, "furnishing", true);
            }

            {
                auto&& regeneration = dungeons["regeneration"];
                a(m_settings.dungeonsRegenerationInterval, regeneration, "interval", days(3), [](minutes val) { return val < 1min; });
                a(m_settings.dungeonsRegenerationMaxSteps, regeneration, "steps", 3, [](int val) { return val < 1; });
            }

            a(m_settings.dungeonsSeeded, dungeons, "seeded", true);

            a(m_settings.eventsChance, events, "chance", .2f, [](float val) { return val < 0 || val > 1; });
            a(m_settings.eventsInterval, events, "interval", 46min, [](seconds val) { return val < 0s; });
            a(m_settings.eventsRadius, events, "activation-radius", 96, [](float val) { return val < 1 || val > 96 * 4; });
            a(m_settings.eventsRequireKeys, events, "require-keys", true);

            a(m_settings.packetMode, packet, "mode", PacketMode::NORMAL, nullptr, reloading);
            a(m_settings.packetFileUpperSize, packet, "file-size", 256000ULL, [](size_t val) { return val < 0 || val > 256000000ULL; }, reloading);
            a(m_settings.packetCaptureSessionIndex, packet, "capture-session", -1, nullptr, reloading);
            a(m_settings.packetPlaybackSessionIndex, packet, "playback-session", -1, nullptr, reloading);

            a(m_settings.discordWebhook, discord, "webhook", "");
            a(m_settings.discordToken, discord, "token", "", nullptr, reloading);
            a(m_settings.discordGuild, discord, "guild", 0, nullptr, reloading);
            //a(m_settings.discordDeleteCommands, discord, "delete-commands", false, nullptr, reloading);

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

        if (!reloading) {
            YAML::Emitter out;
            out.SetIndent(2);
            out << node;

            VUtils::Resource::WriteFile("server.yml", out.c_str());
        }

    }
    
    LOG(INFO) << "Server config loaded";

    if (!reloading) {

    }

    /*
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
        player["auth"] = m_settings.playerOnline;
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
    }*/

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

    DiscordManager()->Init();

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

    if (VH_SETTINGS.packetMode != PacketMode::PLAYBACK)
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

    if (m_settings.dungeonsRegenerationInterval > 0s)
        DungeonManager()->TryRegenerateDungeons();
    
    std::error_code err;
    auto lastWriteTime = fs::last_write_time("server.yml", err);
    if (lastWriteTime != this->m_settingsLastTime) {
        // reload the file
        LoadFiles(true);
    }

    if (m_settings.packetMode == PacketMode::PLAYBACK) {
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
