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

quill::Logger *LOGGER = nullptr;

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

    // TODO see
    // ...\vcpkg\installed\x64-windows\include\yaml-cpp\binary.h
    // some ideas for datareader/datawriter buffer ownership
    //  basically use to have a single simple class for reading and another for writing, instead of 2 for ownership/observing
    //  it will *really* simplify the flow
    //  just use throws when doing an illegal operation?
    //      i like performance though, not sure how much of a difference it makes
    //      

    // also see
    // ...\vcpkg\installed\x64-windows\include\yaml-cpp\convert.h
    // some ideas for more specific generics within a known type
    //  basically use to remove a bunch of std::chrono::duration template overloads below



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
                case 't': out = duration_cast<T>(TICKS_t(dur)); return true;
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

    template<typename Rep, typename Period>
    struct convert<duration<Rep, Period>> {
        static Node encode(const duration<Rep, Period>& rhs) {
            //using D = std::remove_reference_t<std::remove_const_t<decltype(rhs)>>;
            using D = std::remove_cvref_t<decltype(rhs)>;

            if constexpr (std::is_same_v<D, nanoseconds>)
                return Node(std::to_string(rhs.count()) + "ns");
            else if constexpr (std::is_same_v<D, TICKS_t>)
                return Node(std::to_string(rhs.count()) + "ticks");
            else if constexpr (std::is_same_v<D, microseconds>)
                return Node(std::to_string(rhs.count()) + "us");
            else if constexpr (std::is_same_v<D, milliseconds>)
                return Node(std::to_string(rhs.count()) + "ms");
            else if constexpr (std::is_same_v<D, seconds>)
                return Node(std::to_string(rhs.count()) + "s");
            else if constexpr (std::is_same_v<D, minutes>)
                return Node(std::to_string(rhs.count()) + "Minutes");
            else if constexpr (std::is_same_v<D, hours>)
                return Node(std::to_string(rhs.count()) + "hours");
            else if constexpr (std::is_same_v<D, days>)
                return Node(std::to_string(rhs.count()) + "days");
            else if constexpr (std::is_same_v<D, weeks>)
                return Node(std::to_string(rhs.count()) + "weeks");
            else if constexpr (std::is_same_v<D, months>)
                return Node(std::to_string(rhs.count()) + "onths");
            else if constexpr (std::is_same_v<D, years>)
                return Node(std::to_string(rhs.count()) + "years");

            assert(false);
            return Node(std::to_string(rhs.count()) + "?durationtype");
            //else if constexpr (true)
                //static_assert(false, "Unsupported type provided to convert");
        }

        static bool decode(const Node& node, duration<Rep, Period>& rhs) {
            if (!node.IsScalar())
                return false;

            auto&& s = node.Scalar();

            return parseDuration(node.Scalar(), rhs);
        }
    };

    template<>
    struct convert<dpp::snowflake> {
        static Node encode(const dpp::snowflake& rhs) {
            return Node(std::to_string((uint64_t)rhs));
        }

        static bool decode(const Node& node, dpp::snowflake& rhs) {
            if (!node.IsScalar())
                return false;

            rhs = node.as<int64_t>();
            return true;
        }
    };

    template<typename K, typename V, typename Hash, typename Eq, typename Alloc, typename Bucket> // = ankerl::unordered_dense::hash<K>>
    struct convert<ankerl::unordered_dense::map<K, V, Hash, Eq, Alloc, Bucket>> {
        static Node encode(const ankerl::unordered_dense::map<K, V, Hash, Eq, Alloc, Bucket>& rhs) {
            Node node(NodeType::Map);
            for (const auto& element : rhs)
                node.force_insert(element.first, element.second);
            return node;
        }

        static bool decode(const Node& node, ankerl::unordered_dense::map<K, V, Hash, Eq, Alloc, Bucket>& rhs) {
            if (!node.IsMap())
                return false;

            rhs.clear();
            for (const auto& element : node)
#if defined(__GNUC__) && __GNUC__ < 4
                // workaround for GCC 3:
                rhs[element.first.template as<K>()] = element.second.template as<V>();
#else
                rhs[element.first.as<K>()] = element.second.as<V>();
#endif
            return true;
        }
    };

    template<typename K, typename Hash, typename Eq, typename Alloc, typename Bucket> // = ankerl::unordered_dense::hash<K>>
    struct convert<ankerl::unordered_dense::set<K, Hash, Eq, Alloc, Bucket>> {
        static Node encode(const ankerl::unordered_dense::set<K, Hash, Eq, Alloc, Bucket>& rhs) {
            Node node(NodeType::Sequence);
            for (const auto& element : rhs)
                node.push_back(element);
            return node;
        }

        static bool decode(const Node& node, ankerl::unordered_dense::set<K, Hash, Eq, Alloc, Bucket>& rhs) {
            if (!node.IsSequence())
                return false;

            rhs.clear();
            for (const auto& element : node)
#if defined(__GNUC__) && __GNUC__ < 4
                // workaround for GCC 3:

                rhs.insert(element.template as<K>());
#else
                rhs.insert(element.as<K>());
#endif
            return true;
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
                    LOG_INFO(LOGGER, "{}", e.what());
                    fileError = true;
                }
            }
            else {
                if (!reloading) {
                    LOG_INFO(LOGGER, "Server config not found, creating...");
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
            
#ifdef VH_OPTION_ENABLE_CAPTURE
            a(m_settings.packetMode, packet, "mode", PacketMode::NORMAL, nullptr, reloading);
            a(m_settings.packetFileUpperSize, packet, "file-size", 256000ULL, [](size_t val) { return val < 0 || val > 256000000ULL; }, reloading);
            a(m_settings.packetCaptureSessionIndex, packet, "capture-session", -1, nullptr, reloading);
            a(m_settings.packetPlaybackSessionIndex, packet, "playback-session", -1, nullptr, reloading);

            if (m_settings.packetMode == PacketMode::CAPTURE)
                m_settings.packetCaptureSessionIndex++;
#endif

            a(m_settings.discordWebhook, discord, "webhook", "");
            a(m_settings.discordToken, discord, "token", "", nullptr, reloading);
            a(m_settings.discordGuild, discord, "guild", 0, nullptr, reloading);
            a(m_settings.discordAccountLinking, discord, "account-linking", false, nullptr, reloading);
            //a(m_settings.discordDeleteCommands, discord, "delete-commands", false, nullptr, reloading);
             
            //a(m_settings.discordDevAccount, discord, "dev-account", UNORDERED_SET_t<std::string>());

            //a(m_settings.discordEnableDevCommands, discord, "enable-dev-commands", true);

            if (m_settings.serverPassword.empty()) {
                LOG_WARNING(LOGGER, "Server does not have a password");
            }
            else {
                LOG_INFO(LOGGER, "Server password is '{}'", m_settings.serverPassword);
            }

#ifdef VH_OPTION_ENABLE_CAPTURE
            if (m_settings.packetMode == PacketMode::CAPTURE) {
                LOG_WARNING(LOGGER, "Experimental packet capture enabled");
            }
            else if (m_settings.packetMode == PacketMode::PLAYBACK) {
                LOG_WARNING(LOGGER, "Experimental packet playback enabled");
            }
#endif
        }

        if (!reloading) {
            YAML::Emitter out;
            out.SetIndent(2);
            out << node;

            VUtils::Resource::WriteFile("server.yml", out.c_str());
        }
    }
    
    if (auto&& opt = VUtils::Resource::ReadFile<std::string>("blacklist.yml")) {
        try {
            auto node = YAML::Load(*opt);
            m_blacklist = node.as<decltype(m_blacklist)>();
        }
        catch (const YAML::Exception& e) {
            LOG_ERROR(LOGGER, "{}", e.what());
        }
    }

    if (auto&& opt = VUtils::Resource::ReadFile<std::string>("whitelist.yml")) {
        try {
            auto node = YAML::Load(*opt);
            m_whitelist = node.as<decltype(m_whitelist)>();
        }
        catch (const YAML::Exception& e) {
            LOG_ERROR(LOGGER, "{}", e.what());
        }
    }

    if (auto&& opt = VUtils::Resource::ReadFile<std::string>("admin.yml")) {
        try {
            auto node = YAML::Load(*opt);
            m_admin = node.as<decltype(m_admin)>();
        }
        catch (const YAML::Exception& e) {
            LOG_ERROR(LOGGER, "{}", e.what());
        }
    }

    if (m_settings.discordAccountLinking) {
        if (auto&& opt = VUtils::Resource::ReadFile<std::string>("linked.yml")) {
            try {
                auto node = YAML::Load(*opt);
                DiscordManager()->m_linkedAccounts = node.as<decltype(IDiscordManager::m_linkedAccounts)>();
            }
            catch (const YAML::Exception& e) {
                LOG_ERROR(LOGGER, "{}", e.what());
            }
        }
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
        std::string title = "Valhalla " + std::string(VH_VERSION) + " - Valheim " + std::string(VConstants::GAME);
        SetConsoleTitle(title.c_str());
    }
#endif

    std::error_code err;
    this->m_settingsLastTime = fs::last_write_time("server.yml", err);
}

std::thread::id MAIN_THREAD;

void IValhalla::Stop() {
    m_terminate = true;

    // prevent deadlock
    if (std::this_thread::get_id() != MAIN_THREAD)
        m_terminate.wait(true);
}

void IValhalla::Start() {    
    MAIN_THREAD = std::this_thread::get_id();
    


    LOG_INFO(LOGGER, "Starting Valhalla {} (Valheim {})", VH_VERSION, VConstants::GAME);

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
        tracy::SetThreadName("system");

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

        FrameMark;
    }

    VH_DISPATCH_WEBHOOK("Server stopping");
            
    LOG_INFO(LOGGER, "Terminating server");

    // Cleanup 
    NetManager()->Uninit();
    HeightmapBuilder()->Uninit();

    ModManager()->Uninit();

#ifdef VH_OPTION_ENABLE_CAPTURE
    if (VH_SETTINGS.packetMode != PacketMode::PLAYBACK)
#endif
        WorldManager()->GetWorld()->WriteFiles();

    {
        YAML::Node node(m_blacklist);

        YAML::Emitter emit;
        emit.SetIndent(2);
        emit << node;

        VUtils::Resource::WriteFile("blacklist.yml", emit.c_str());
    }

    {
        YAML::Node node(m_whitelist);

        YAML::Emitter emit;
        emit.SetIndent(2);
        emit << node;

        VUtils::Resource::WriteFile("whitelist.yml", emit.c_str());
    }

    {
        YAML::Node node(m_admin);
        
        YAML::Emitter emit;
        emit.SetIndent(2);
        emit << node;

        VUtils::Resource::WriteFile("admin.yml", emit.c_str());
    }

    {
        YAML::Node node(DiscordManager()->m_linkedAccounts);

        YAML::Emitter emit;
        emit.SetIndent(2);

        emit << YAML::Comment("Discord-linked accounts") 
             << YAML::Newline
             << YAML::Comment("Entries are in the form of 'steam-id: discord-id'")
             << node;

        VUtils::Resource::WriteFile("linked.yml", emit.c_str());
    }

    LOG_INFO(LOGGER, "Server was gracefully terminated");

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
        LOG_INFO(LOGGER, "There are a total of {} peers online", NetManager()->GetPeers().size());
    });

    VH_DISPATCH_MOD_EVENT(IModManager::Events::PeriodicUpdate);

    DiscordManager()->PeriodUpdate();

    if (m_settings.dungeonsRegenerationInterval > 0s)
        DungeonManager()->TryRegenerateDungeons();
    
    std::error_code err;
    auto lastWriteTime = fs::last_write_time("server.yml", err);
    if (lastWriteTime != this->m_settingsLastTime) {
        // reload the file
        LoadFiles(true);
    }

#ifdef VH_OPTION_ENABLE_CAPTURE
    if (m_settings.packetMode == PacketMode::PLAYBACK) {
        PERIODIC_NOW(333ms, {
            char message[32];
            std::sprintf(message, "World playback %.2fs", (duration_cast<milliseconds>(Valhalla()->Nanos()).count() / 1000.f));
            Broadcast(UIMsgType::TopLeft, message);
        });
    }
#endif

    if (m_settings.worldSaveInterval > 0s) {
        // save warming message
        PERIODIC_LATER(m_settings.worldSaveInterval, m_settings.worldSaveInterval, {
            LOG_INFO(LOGGER, "World saving in 30s");
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

void IValhalla::Broadcast(UIMsgType type, std::string_view text) {
    RouteManager()->InvokeAll(Hashes::Routed::S2C_UIMessage, type, text);
}
