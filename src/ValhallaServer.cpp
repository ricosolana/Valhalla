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
                dur += (ch - '0') * (index + 1);
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
            return Node(std::to_string(rhs.count()) + "s");
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
            return Node(std::to_string(rhs.count()) + "s");
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
            return Node(std::to_string(rhs.count()) + "s");
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
            return Node(std::to_string(rhs.count()) + "s");
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
            return Node(std::to_string(rhs.count()) + "s");
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
            return Node(std::to_string(rhs.count()) + "s");
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
            return Node(std::to_string(rhs.count()) + "s");
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
            return Node(std::to_string(rhs.count()) + "s");
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
            return Node(std::to_string(rhs.count()) + "s");
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
    || is_duration<typename std::tuple_element_t<0, typename VUtils::Traits::func_traits<Func>::args_type>>::value == is_duration<T>::value)
void a(T& set, YAML::Node node, const std::string& key, Def def, Func defPred = nullptr, bool reloading = false, std::string_view comment = "") {
    if (reloading)
        return;
    
    auto&& mapping = node[key];

    try {
        auto&& val = mapping.as<T>();

        if constexpr (!std::is_same_v<Func, std::nullptr_t>) {
            using Param0 = std::tuple_element_t<0, typename VUtils::Traits::func_traits<Func>::args_type>;

            static constexpr auto T_IS_DUR = is_duration<T>::value;
            static constexpr auto PARAM0_IS_DUR = is_duration<Param0>::value;

            //static_assert(T_IS_DUR == PARAM0_IS_DUR && "Both types must be durations or something else");
            /*
            if (!defPred || !defPred(duration_cast<Param0>(val))) {
                set = val;
                return;
            }*/

            if constexpr (T_IS_DUR && PARAM0_IS_DUR) {
                if (!defPred || !defPred(duration_cast<Param0>(val))) {
                    set = val;
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
        /*
        if (!defPred || !defPred(val)) {
            set = val;
            return;
            //return val;
        }*/
    }
    catch (const YAML::Exception&) {}

    mapping = def;
    
    assert(node[key].IsDefined());
    
    if constexpr (is_duration<T>::value && is_duration<Def>::value) {
        //duration_cast<seconds>(1d);
        set = duration_cast<T>(def);
    }
    else
        set = T(def);

    //set = static_cast<decltype(set)>(def);
    //set = static_cast<T>(def);
    //set = static_cast<decltype(T)>(def);
    //set = static_cast<T>(decltype(def){ def });
    //set = def;
};

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
            /*
            auto a = []<typename T>(YAML::Node & node, std::string_view key, const T & def = T{}, bool skip = false, std::string_view comment = "") {
                auto&& mapping = node[key];
                
                try {
                    return mapping.as<T>();
                    //return mapping.as<YAML::Node>();
                }
                //catch (const std::exception&) {
                catch (const YAML::InvalidNode&) {
                    mapping = def;
                }

                assert(node[key].IsDefined());

                return def;
            };*/

            //std::string k = a(loadNode, "mykey", "mydef");

            //auto&& server = a(loadNode, VH_SETTING_KEY_SERVER, YAML::Node());
            auto&& server = loadNode[VH_SETTING_KEY_SERVER];
            auto&& discord = loadNode[VH_SETTING_KEY_DISCORD];
            auto&& world = loadNode[VH_SETTING_KEY_WORLD];
            auto&& packet = loadNode[VH_SETTING_KEY_PACKET];
            auto&& player = loadNode[VH_SETTING_KEY_PLAYER];
            auto&& zdo = loadNode[VH_SETTING_KEY_ZDO];
            auto&& dungeons = loadNode[VH_SETTING_KEY_DUNGEONS];
            auto&& events = loadNode[VH_SETTING_KEY_EVENTS];

            //m_settings.serverName = VUtils::String::ToAscii(server[VH_SETTING_KEY_SERVER_NAME].as<std::string>(""));
            a(m_settings.serverName, server, VH_SETTING_KEY_SERVER_NAME, "Valhalla server"); //, [](const std::string& val) { return val.empty() || val.length() < 3 || val.length() > 64; });
            a(m_settings.serverPassword, server, VH_SETTING_KEY_SERVER_PASSWORD, "secret", [](const std::string& val) { return !val.empty() && (val.length() < 5 || val.length() > 11); });
            a(m_settings.serverPort, server, VH_SETTING_KEY_SERVER_PORT, 2456, nullptr, reloading);
            a(m_settings.serverPublic, server, VH_SETTING_KEY_SERVER_PUBLIC, false, nullptr, reloading);
            a(m_settings.serverDedicated, server, VH_SETTING_KEY_SERVER_DEDICATED, true, nullptr, reloading);

            a(m_settings.worldName, world, VH_SETTING_KEY_WORLD, "world", [](const std::string& val) { return val.empty() || val.length() < 3; });
            a(m_settings.worldSeed, world, VH_SETTING_KEY_WORLD_SEED, VUtils::Random::GenerateAlphaNum(10), [](const std::string& val) { return val.empty(); });
            a(m_settings.worldModern, world, VH_SETTING_KEY_WORLD_MODERN, true);

            a(m_settings.packetMode, packet, VH_SETTING_KEY_PACKET_MODE, PacketMode::NORMAL, nullptr, reloading);
            a(m_settings.packetFileUpperSize, packet, VH_SETTING_KEY_PACKET_FILE_UPPER_SIZE, 256000ULL, [](size_t val) { return val < 0 || val > 256000000ULL; }, reloading);
            a(m_settings.packetCaptureSessionIndex, packet, VH_SETTING_KEY_PACKET_CAPTURE_SESSION_INDEX, -1);
            a(m_settings.packetPlaybackSessionIndex, packet, VH_SETTING_KEY_PACKET_PLAYBACK_SESSION_INDEX, -1);

            //a(m_settings.packetFileUpperSize, packet, VH_SETTING_KEY_PACKET_FILE_UPPER_SIZE, 256000ULL, [](milliseconds val) { return val < 0s || val > 25600000s; }, reloading);

            if (m_settings.packetMode == PacketMode::CAPTURE)
                m_settings.packetCaptureSessionIndex++;

            /*
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
            }*/

            a(m_settings.discordWebhook, discord, VH_SETTING_KEY_DISCORD_WEBHOOK, "");

            a(m_settings.worldFeatures, world, VH_SETTING_KEY_WORLD_FEATURES, true);
            a(m_settings.worldVegetation, world, VH_SETTING_KEY_WORLD_VEGETATION, true);
            a(m_settings.worldCreatures, world, VH_SETTING_KEY_WORLD_CREATURES, true);
            a(m_settings.worldSaveInterval, world, VH_SETTING_KEY_WORLD_SAVE_INTERVAL, 30min, [](seconds val) { return val < 0s || val > seconds(60 * 60 * 24 * 7); });

            a(m_settings.playerWhitelist, player, VH_SETTING_KEY_PLAYER_WHITELIST, true);
            a(m_settings.playerMax, player, VH_SETTING_KEY_PLAYER_MAX, 10, [](int val) { return val < 1; });
            a(m_settings.playerOnline, player, VH_SETTING_KEY_PLAYER_OFFLINE, true);
            a(m_settings.playerTimeout, player, VH_SETTING_KEY_PLAYER_TIMEOUT, 1h);
            a(m_settings.playerListSendInterval, player, VH_SETTING_KEY_PLAYER_LIST_SEND_INTERVAL, 2s, [](milliseconds val) { return val < 0s || val > seconds(1000 * 10); });
            a(m_settings.playerListForceVisible, player, VH_SETTING_KEY_PLAYER_LIST_FORCE_VISIBLE, false);
            
            a(m_settings.zdoMaxCongestion, zdo, VH_SETTING_KEY_ZDO_MAX_CONGESTION, 10240, [](int val) { return val < 1000; });
            a(m_settings.zdoMinCongestion, zdo, VH_SETTING_KEY_ZDO_MIN_CONGESTION, 2048, [](int val) { return val < 1000; });
            a(m_settings.zdoSendInterval, zdo, VH_SETTING_KEY_ZDO_SEND_INTERVAL, 50ms, [](milliseconds val) { return val <= 0ms || val >= 1s; });
            a(m_settings.zdoAssignInterval, zdo, VH_SETTING_KEY_ZDO_ASSIGN_INTERVAL, 2s, [](seconds val) { return val <= 0s || val > 10s; });
            a(m_settings.zdoAssignAlgorithm, zdo, VH_SETTING_KEY_ZDO_ASSIGN_ALGORITHM, AssignAlgorithm::NONE);
            

            /*
            m_settings.discordWebhook = discord[VH_SETTING_KEY_DISCORD_WEBHOOK].as<std::string>("");

            m_settings.worldFeatures = world[VH_SETTING_KEY_WORLD_FEATURES].as<bool>(true);
            m_settings.worldVegetation = world[VH_SETTING_KEY_WORLD_VEGETATION].as<bool>(true);
            m_settings.worldCreatures = world[VH_SETTING_KEY_WORLD_CREATURES].as<bool>(true);           

            m_settings.worldSaveInterval = seconds(std::clamp(world[VH_SETTING_KEY_WORLD_SAVE_INTERVAL].as<int>(1800), 0, 60 * 60 * 24 * 7));

            m_settings.playerWhitelist = player[VH_SETTING_KEY_PLAYER_WHITELIST].as<bool>(true);          // enable whitelist
            m_settings.playerMax = std::max(player[VH_SETTING_KEY_PLAYER_MAX].as<int>(10), 1);     // max allowed players
            m_settings.playerOnline = player[VH_SETTING_KEY_PLAYER_OFFLINE].as<bool>(true);                     // allow authed players only
            m_settings.playerTimeout = seconds(std::clamp(player[VH_SETTING_KEY_PLAYER_TIMEOUT].as<int>(30), 1, 60*60));
            m_settings.playerListSendInterval = milliseconds(std::clamp(player[VH_SETTING_KEY_PLAYER_LIST_SEND_INTERVAL].as<int>(2000), 0, 1000*10));
            m_settings.playerListForceVisible = player[VH_SETTING_KEY_PLAYER_LIST_FORCE_VISIBLE].as<bool>(false);

            m_settings.zdoMaxCongestion = zdo[VH_SETTING_KEY_ZDO_MAX_CONGESTION].as<int>(10240);
            m_settings.zdoMinCongestion = zdo[VH_SETTING_KEY_ZDO_MIN_CONGESTION].as<int>(2048);
            m_settings.zdoSendInterval = milliseconds(zdo[VH_SETTING_KEY_ZDO_SEND_INTERVAL].as<int>(50));
            m_settings.zdoAssignInterval = seconds(std::clamp(zdo[VH_SETTING_KEY_ZDO_ASSIGN_INTERVAL].as<int>(2), 1, 60));
            m_settings.zdoAssignAlgorithm = (AssignAlgorithm) zdo[VH_SETTING_KEY_ZDO_ASSIGN_ALGORITHM].as<int>(std::to_underlying(AssignAlgorithm::NONE));
            */

            a(m_settings.dungeonsEnabled, dungeons, VH_SETTING_KEY_DUNGEONS_ENABLED, true);
            {
                auto&& endcaps = dungeons[VH_SETTING_KEY_DUNGEONS_ENDCAPS];
                a(m_settings.dungeonsEndcapsEnabled, endcaps, VH_SETTING_KEY_DUNGEONS_ENDCAPS_ENABLED, true);
                a(m_settings.dungeonsEndcapsInsetFrac, endcaps, VH_SETTING_KEY_DUNGEONS_ENDCAPS_INSETFRAC, .5f, [](float val) { return val < 0.f || val > 1.f; });
            }
            /*
            m_settings.dungeonsEnabled = dungeons[VH_SETTING_KEY_DUNGEONS_ENABLED].as<bool>(true);
            {
                auto&& endcaps = dungeons[VH_SETTING_KEY_DUNGEONS_ENDCAPS];
                m_settings.dungeonsEndcapsEnabled = endcaps[VH_SETTING_KEY_DUNGEONS_ENDCAPS_ENABLED].as<bool>(true);
                m_settings.dungeonsEndcapsInsetFrac = std::clamp(endcaps[VH_SETTING_KEY_DUNGEONS_ENDCAPS_INSETFRAC].as<float>(.5f), 0.f, 1.f);
            }*/

            a(m_settings.dungeonsDoors, dungeons, VH_SETTING_KEY_DUNGEONS_DOORS, true);

            //m_settings.dungeonsDoors = dungeons[VH_SETTING_KEY_DUNGEONS_DOORS].as<bool>(true);

            {
                auto&& rooms = dungeons[VH_SETTING_KEY_DUNGEONS_ROOMS];
                a(m_settings.dungeonsRoomsFlipped, rooms, VH_SETTING_KEY_DUNGEONS_ROOMS_FLIPPED, true);
                a(m_settings.dungeonsRoomsZoneBounded, rooms, VH_SETTING_KEY_DUNGEONS_ROOMS_FLIPPED, true);
                a(m_settings.dungeonsRoomsInsetSize, rooms, VH_SETTING_KEY_DUNGEONS_ROOMS_FLIPPED, .1f, [](float val) { return val < 0; });
            }

            /*
            {
                auto&& rooms = dungeons[VH_SETTING_KEY_DUNGEONS_ROOMS];
                m_settings.dungeonsRoomsFlipped = rooms[VH_SETTING_KEY_DUNGEONS_ROOMS_FLIPPED].as<int>(true);
                m_settings.dungeonsRoomsZoneBounded = rooms[VH_SETTING_KEY_DUNGEONS_ROOMS_ZONEBOUNDED].as<int>(true);
                m_settings.dungeonsRoomsInsetSize = std::max(rooms[VH_SETTING_KEY_DUNGEONS_ROOMS_INSETSIZE].as<float>(.1f), 0.f);
            }*/

            {
                auto&& regeneration = dungeons[VH_SETTING_KEY_DUNGEONS_REGENERATION];
                a(m_settings.dungeonsRegenerationInterval, regeneration, VH_SETTING_KEY_DUNGEONS_REGENERATION_INTERVAL, days(3), [](minutes val) { return val < 1min; });
                a(m_settings.dungeonsRegenerationMaxSteps, regeneration, VH_SETTING_KEY_DUNGEONS_REGENERATION_MAXSTEP, 3, [](int val) { return val < 1; });
            }

            /*
            {
                auto&& regeneration = dungeons[VH_SETTING_KEY_DUNGEONS_REGENERATION];
                m_settings.dungeonsRegenerationInterval = seconds(std::clamp(regeneration[VH_SETTING_KEY_DUNGEONS_REGENERATION_INTERVAL].as<int64_t>(60LL*60LL*24LL*3LL), 0LL, 60LL*60LL*24LL*30LL));
                m_settings.dungeonsRegenerationMaxSteps = std::max(regeneration[VH_SETTING_KEY_DUNGEONS_REGENERATION_MAXSTEP].as<int>(3), 0);
            }*/

            a(m_settings.dungeonsSeeded, dungeons, VH_SETTING_KEY_DUNGEONS_SEEDED, true);

            //m_settings.dungeonsSeeded = dungeons[VH_SETTING_KEY_DUNGEONS_SEEDED].as<bool>(true); // TODO rename seeded

            a(m_settings.eventsChance, events, VH_SETTING_KEY_EVENTS_CHANCE, .2f, [](float val) { return val < 0 || val > 1; });
            a(m_settings.eventsInterval, events, VH_SETTING_KEY_EVENTS_INTERVAL, 46min, [](seconds val) { return val < 0s; });
            a(m_settings.eventsRadius, events, VH_SETTING_KEY_EVENTS_RADIUS, 96, [](float val) { return val < 1 || val > 96 * 4; });
            a(m_settings.eventsRequireKeys, events, VH_SETTING_KEY_DUNGEONS_SEEDED, true);

            /*
            m_settings.eventsChance = std::clamp(events[VH_SETTING_KEY_EVENTS_CHANCE].as<float>(.2f), 0.f, 1.f);
            m_settings.eventsInterval = seconds(std::max(0, events[VH_SETTING_KEY_EVENTS_INTERVAL].as<int>(60 * 46)));
            m_settings.eventsRadius = std::clamp(events[VH_SETTING_KEY_EVENTS_RADIUS].as<float>(96), 1.f, 96.f * 4);
            m_settings.eventsRequireKeys = events[VH_SETTING_KEY_DUNGEONS_SEEDED].as<bool>(true);*/



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
