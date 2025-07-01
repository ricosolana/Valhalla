#include <chrono>
#include <mutex>
#include <stdlib.h>
#include <thread>
#include <type_traits>
#include <utility>
#ifdef _WIN32
    #include <winstring.h>
#endif

#include <magic_enum.hpp>
#include <quill/core/LogLevel.h>
#include <quill/LogMacros.h>
#include <quill/sinks/RotatingFileSink.h>
#include <tracy/Tracy.hpp>
#include <yaml-cpp/yaml.h>

#include "DiscordManager.h"
#include "DungeonManager.h"
#include "GeoManager.h"
#include "Hashes.h"
#include "HeightmapBuilder.h"
#include "ModManager.h"
#include "NetManager.h"
#include "RandomEventManager.h"
#include "RouteManager.h"
#include "ServerSettings.h"
#include "ValhallaServer.h"
#include "VUtilsResource.h"
#include "VUtilsString.h"
#include "ZDOManager.h"
#include "ZoneManager.h"

// Defined
quill::Logger *AVL_LOGGER {};

auto VALHALLA_INSTANCE = std::make_unique<IAvledet>();

IAvledet *Avledet()
{
    return VALHALLA_INSTANCE.get();
}

template<class Enum>
concept scoped_enum = requires { typename std::is_scoped_enum<Enum>; };

namespace YAML {
    template<scoped_enum Enum>
    struct convert<Enum>
    {
        static Node encode(Enum const &rhs)
        {
            auto val = magic_enum::enum_name(rhs);
            return Node(avledet::lexicon::to_lower(std::string(val)));
        }

        static bool decode(Node const &node, Enum &rhs)
        {
            if (!node.IsScalar())
                return false;

            if (auto opt
                = magic_enum::enum_cast<Enum>(node.as<std::string>(), magic_enum::case_insensitive)) {
                rhs = opt.value();
                return true;
            }

            return false;
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
    static bool parseDuration(std::string const &s, T &out)
    {
        std::int64_t dur  = 0;
        std::size_t index = 0;
        std::int64_t sign = 1;
        for (; index < s.length(); index++) {
            std::int64_t const ch = (std::int64_t) s[index];
            if (ch == '-') {
                sign = -1;
            } else if (ch >= '0' && ch <= '9') {
                dur *= 10;
                dur += (ch - '0');
            } else if (index > 0) {
                for (; index < s.length() && s[index] == ' '; index++) {}// skip spaces

                dur *= sign;
                std::int64_t const ch2 = index < s.length() - 1 ? s[index + 1] : ' ';
                switch (ch) {
                case 'n': out = std::chrono::duration_cast<T>(std::chrono::nanoseconds(dur)); return true;
                case 't': out = std::chrono::duration_cast<T>(avledet::util::Ticks(dur)); return true;
                case 'u': out = std::chrono::duration_cast<T>(std::chrono::microseconds(dur)); return true;
                case 'm': {
                    switch (ch2) {
                    case 's':
                        out = std::chrono::duration_cast<T>(std::chrono::milliseconds(dur));
                        return true;
                    case 'i': out = std::chrono::duration_cast<T>(std::chrono::minutes(dur)); return true;
                    case 'o': out = std::chrono::duration_cast<T>(std::chrono::months(dur)); return true;
                    default: break;
                    }
                    break;
                }
                case 's': out = std::chrono::duration_cast<T>(std::chrono::seconds(dur)); return true;
                case 'h': out = std::chrono::duration_cast<T>(std::chrono::hours(dur)); return true;
                case 'd': out = std::chrono::duration_cast<T>(std::chrono::days(dur)); return true;
                case 'w': out = std::chrono::duration_cast<T>(std::chrono::weeks(dur)); return true;
                case 'y': out = std::chrono::duration_cast<T>(std::chrono::years(dur)); return true;
                }
                break;
            }
        }
        out = T(dur);
        return false;
    };

    template<typename Rep, typename Period>
    struct convert<std::chrono::duration<Rep, Period>>
    {
        static Node encode(std::chrono::duration<Rep, Period> const &rhs)
        {
            //using D = std::remove_reference_t<std::remove_const_t<decltype(rhs)>>;
            using D = std::remove_cvref_t<decltype(rhs)>;

            if constexpr (std::is_same_v<D, std::chrono::nanoseconds>)
                return Node(std::to_string(rhs.count()) + "ns");
            else if constexpr (std::is_same_v<D, avledet::util::Ticks>)
                return Node(std::to_string(rhs.count()) + " ticks");
            else if constexpr (std::is_same_v<D, std::chrono::microseconds>)
                return Node(std::to_string(rhs.count()) + "us");
            else if constexpr (std::is_same_v<D, std::chrono::milliseconds>)
                return Node(std::to_string(rhs.count()) + "ms");
            else if constexpr (std::is_same_v<D, std::chrono::seconds>)
                return Node(std::to_string(rhs.count()) + "s");
            else if constexpr (std::is_same_v<D, std::chrono::minutes>)
                return Node(std::to_string(rhs.count()) + "min");
            else if constexpr (std::is_same_v<D, std::chrono::hours>)
                return Node(std::to_string(rhs.count()) + " hours");
            else if constexpr (std::is_same_v<D, std::chrono::days>)
                return Node(std::to_string(rhs.count()) + " days");
            else if constexpr (std::is_same_v<D, std::chrono::weeks>)
                return Node(std::to_string(rhs.count()) + " weeks");
            else if constexpr (std::is_same_v<D, std::chrono::months>)
                return Node(std::to_string(rhs.count()) + " months");
            else if constexpr (std::is_same_v<D, std::chrono::years>)
                return Node(std::to_string(rhs.count()) + " years");

            assert(false);
            return Node(std::to_string(rhs.count()) + "?durationtype");
            //else if constexpr (true)
            //static_assert(false, "Unsupported type provided to convert");
        }

        static bool decode(Node const &node, std::chrono::duration<Rep, Period> &rhs)
        {
            if (!node.IsScalar())
                return false;

            auto &&s = node.Scalar();

            return parseDuration(node.Scalar(), rhs);
        }
    };

#if AVL_IS_ON(AVL_DISCORD_INTEGRATION)
    template<>
    struct convert<dpp::snowflake>
    {
        static Node encode(dpp::snowflake const &rhs)
        {
            return Node(std::to_string((std::uint64_t) rhs));
        }

        static bool decode(Node const &node, dpp::snowflake &rhs)
        {
            if (!node.IsScalar())
                return false;

            rhs = node.as<std::int64_t>();
            return true;
        }
    };
#endif

    template<typename K, typename V, typename Hash, typename Eq, typename Alloc,
             typename Bucket>// = ankerl::unordered_dense::hash<K>>
    struct convert<ankerl::unordered_dense::map<K, V, Hash, Eq, Alloc, Bucket>>
    {
        static Node encode(ankerl::unordered_dense::map<K, V, Hash, Eq, Alloc, Bucket> const &rhs)
        {
            Node node(NodeType::Map);
            for (auto const &element : rhs) node.force_insert(element.first, element.second);
            return node;
        }

        static bool decode(Node const &node, ankerl::unordered_dense::map<K, V, Hash, Eq, Alloc, Bucket> &rhs)
        {
            if (!node.IsMap())
                return false;

            rhs.clear();
            for (auto const &element : node)
#if defined(__GNUC__) && __GNUC__ < 4
                // workaround for GCC 3:
                rhs[element.first.template as<K>()] = element.second.template as<V>();
#else
                rhs[element.first.as<K>()] = element.second.as<V>();
#endif
            return true;
        }
    };

    template<typename K, typename Hash, typename Eq, typename Alloc,
             typename Bucket>// = ankerl::unordered_dense::hash<K>>
    struct convert<ankerl::unordered_dense::set<K, Hash, Eq, Alloc, Bucket>>
    {
        static Node encode(ankerl::unordered_dense::set<K, Hash, Eq, Alloc, Bucket> const &rhs)
        {
            Node node(NodeType::Sequence);
            for (auto const &element : rhs) node.push_back(element);
            return node;
        }

        static bool decode(Node const &node, ankerl::unordered_dense::set<K, Hash, Eq, Alloc, Bucket> &rhs)
        {
            if (!node.IsSequence())
                return false;

            rhs.clear();
            for (auto const &element : node)
#if defined(__GNUC__) && __GNUC__ < 4
                // workaround for GCC 3:

                rhs.insert(element.template as<K>());
#else
                rhs.insert(element.as<K>());
#endif
            return true;
        }
    };

}// namespace YAML

template<class T>
struct is_duration : std::false_type
{};

template<class Rep, class Period>
struct is_duration<std::chrono::duration<Rep, Period>> : std::true_type
{};

// Retrieve a config value
//  Returns the value or the default
//  The key will be set in the config
//  Accepts an optional predicate for whether to use the default value
//template<typename T, typename Func = decltype([](const T&) -> bool {})>


template<typename T, typename D, typename Func = std::nullptr_t>
    requires(std::is_same_v<Func, std::nullptr_t>
             || ((is_duration<typename std::tuple_element_t<
                          0, typename VUtils::Traits::func_traits<Func>::args_type>>::value
                  && is_duration<T>::value)
                 == is_duration<D>::value))
void a(T &set, YAML::Node mutableNode, std::string const &key, D const &default_value,
       Func valueSanitizer = nullptr, bool skip = false)
{
    if (skip)
        return;

    auto &&mapping = mutableNode[key];

    try {
        auto &&val = mapping.as<T>();

        if constexpr (!std::is_same_v<Func, std::nullptr_t>) {
            using Param0 = std::tuple_element_t<0, typename VUtils::Traits::func_traits<Func>::args_type>;

            if constexpr (is_duration<T>::value) {
                if (!valueSanitizer(std::chrono::duration_cast<Param0>(val))) {
                    set = val;
                    return;
                }
            } else {
                if (!valueSanitizer(static_cast<Param0>(val))) {
                    set = val;
                    return;
                }
            }
        } else {
            set = val;
            return;
        }
    } catch (const YAML::Exception &) {
    }

    mapping = default_value;

    assert(mutableNode[key].IsDefined());

    if constexpr (is_duration<T>::value) {
        set = std::chrono::duration_cast<T>(default_value);
    } else
        set = T(default_value);
};

void IAvledet::LoadFiles(bool reloading)
{
    bool fileError = false;

    {
        YAML::Node node;
        {
            if (auto opt = VUtils::Resource::ReadFile<std::string>("server.yml")) {
                try {
                    node = YAML::Load(opt.value());
                } catch (const YAML::ParserException &e) {
                    LOG_INFO(AVL_LOGGER, "{}", e.what());
                    fileError = true;
                }
            } else {
                if (!reloading) {
                    LOG_INFO(AVL_LOGGER, "Server config not found, creating...");
                }
                fileError = true;
            }
        }

        // If the server has just started or theres no config error
        if (!reloading || !fileError) {
            auto &&general  = node["general"];
            auto &&server   = node["server"];
            auto &&players  = node["players"];
            auto &&world    = node["world"];
            auto &&zdo      = node["zdos"];
            auto &&dungeons = node["dungeons"];
            auto &&events   = node["events"];
            auto &&discord  = node["discord"];

            /*
                Server settings
            */

            a(m_settings.serverName, server, "name", "Valhalla server",
              [](std::string const &val) { return val.empty() || val.length() < 3 || val.length() > 64; });
            a(m_settings.serverPassword, server, "password", "",
              [](std::string const &val) { return !val.empty() && (val.length() < 5 || val.length() > 11); });
            a(m_settings.serverPort, server, "port", 2456, nullptr, reloading);
            a(m_settings.serverPublic, server, "public", false, nullptr);
            a(m_settings.serverDedicated, server, "dedicated", true, nullptr, reloading);

            /*
                Player settings
            */

            a(m_settings.playerWhitelist, players, "whitelist", true, nullptr);
            a(m_settings.playerMax, players, "max-online", 10, [](int val) { return val < 1; });
            a(m_settings.playerOnline, players, "authenticate", true, nullptr);
            a(m_settings.playerTimeout, players, "timeout", 30s,
              [](std::chrono::seconds val) { return val < 0s; });
#if AVL_IS_ON(AVL_PLAYER_SLEEP)
            a(m_settings.playerSleepSolo, players, "player-sleep-solo", false, nullptr);
#endif
            a(m_settings.TEST_playerRestrict, players, "experimental-restrict", false, nullptr, false);

            {
                auto &&player_list = players["playerlist"];
                a(m_settings.playerListSmoothUpdating, players, "smooth-updating", 2s,
                  [](std::chrono::seconds val) { return val < 0s; });
                a(m_settings.playerListForceVisible, players, "locations-always-on", false, nullptr);
            }

            /*
                World generation settings
            */

            a(
                    m_settings.worldName, world, "world", "world",
                    [](std::string const &val) { return val.empty() || val.length() < 3; }, reloading);
            a(
                    m_settings.worldSeed, world, "seed", VUtils::Random::GenerateAlphaNum(10),
                    [](std::string const &val) { return val.empty(); }, reloading);
            a(m_settings.TEST_worldPregenerate, world, "experimental-pregenerate", false, nullptr, reloading);
            a(m_settings.worldSaveInterval, world, "save-interval", 30min,
              [](std::chrono::seconds val) { return val < 0s; });
            a(m_settings.worldFeatures, world, "features", true, nullptr);
            a(m_settings.worldVegetation, world, "vegetation", true, nullptr);
            a(m_settings.worldCreatures, world, "creatures", true, nullptr);
            a(m_settings.worldHeightmapThreads, world, "heightmap-threading", 1, nullptr, reloading);

            // limit to physically available threads
            if (m_settings.worldHeightmapThreads == 0
                || m_settings.worldHeightmapThreads > std::jthread::hardware_concurrency())
                m_settings.worldHeightmapThreads = std::jthread::hardware_concurrency();

            // If desired threads is set to max threads, decrement by 1 (because main thread exists duh)
            if (std::jthread::hardware_concurrency() > 1
                && m_settings.worldHeightmapThreads >= std::jthread::hardware_concurrency())
                m_settings.worldHeightmapThreads = std::jthread::hardware_concurrency() - 1;

            /*
                ZDO traffic settings
            */

            a(m_settings.zdoSendInterval, zdo, "send-interval", 50ms,
              [](std::chrono::seconds val) { return val <= 0s; });
            a(m_settings.zdoMaxCongestion, zdo, "max-send-threshold", 10240,
              [](int val) { return val < 1000; });
            a(m_settings.zdoMinCongestion, zdo, "min-send-threshold", 2048,
              [](int val) { return val < 1000; });
            a(m_settings.zdoAssignInterval, zdo, "assign-interval", 2s,
              [](std::chrono::seconds val) { return val < 1s; });
            a(m_settings.TEST_zdoAssignAlgorithm, zdo, "experimental-assign-algorithm", AssignAlgorithm::NONE,
              nullptr);

            /*
                Dungeon generation settings
            */

            a(m_settings.dungeonsEnabled, dungeons, "enabled", true, nullptr);
            {
                auto &&endcaps = dungeons["endcaps"];
                a(m_settings.dungeonsEndcapsEnabled, endcaps, "enabled", true, nullptr);
                a(m_settings.dungeonsEndcapsInsetFrac, endcaps, "inset-ratio", .5f,
                  [](float val) { return val < 0.f || val > 1.f; });
            }

            a(m_settings.dungeonsDoors, dungeons, "doors", true, nullptr);

            {
                auto &&rooms = dungeons["rooms"];
                a(m_settings.dungeonsRoomsFlipped, rooms, "flipped", true, nullptr);
                a(m_settings.dungeonsRoomsZoneBounded, rooms, "zone-bounded", true, nullptr);
                a(m_settings.dungeonsRoomsInsetSize, rooms, "inset-size", .1f,
                  [](float val) { return val < 0; });
                a(m_settings.dungeonsRoomsFurnishing, rooms, "furnishing", true, nullptr);
            }

            {
                auto &&regeneration = dungeons["experimental-regeneration"];
                a(m_settings.TEST_dungeonsRegenerationInterval, regeneration, "interval",
                  std::chrono::days(3), [](std::chrono::minutes val) { return val < 5s; });
                a(m_settings.TEST_dungeonsRegenerationMaxSteps, regeneration, "steps", 3,
                  [](int val) { return val < 1; });
            }

            a(m_settings.dungeonsSeeded, dungeons, "seeded", true, nullptr);

            /*
                Random event / raid settings
            */

            a(m_settings.eventsChance, events, "chance", .2f, [](float val) { return val < 0 || val > 1; });
            a(m_settings.eventsInterval, events, "interval", 46min,
              [](std::chrono::seconds val) { return val < 0s; });
            a(m_settings.eventsRadius, events, "activation-radius", 96,
              [](float val) { return val < 1 || val > 96 * 4; });
            a(m_settings.eventsRequireKeys, events, "require-keys", true, nullptr);

            /*
                Discord settings
            */

#if AVL_IS_ON(AVL_DISCORD_INTEGRATION)
            a(m_settings.discordEnabled, discord, "enabled", false, nullptr, reloading);
            a(m_settings.discordWebhook, discord, "webhook", "",
              nullptr);  //TODO move this somewhere more secure
            a(m_settings.discordToken, discord, "token", "", nullptr,
              reloading);//TODO move this somewhere more secure!!!
            a(m_settings.discordGuild, discord, "guild", 0, nullptr, reloading);
            a(m_settings.TEST_discordAccountLinking, discord, "experimental-account-linking", false, nullptr,
              reloading);
            a(m_settings.TEST_discordSyncLeaves, discord, "experimental-sync-leaves", false, nullptr,
              reloading);
            //a(m_settings.discordDeleteCommands, discord, "delete-commands", false, nullptr, reloading);

            //a(m_settings.discordDevAccount, discord, "dev-account", avledet::util::Set<std::string>());

            //a(m_settings.discordEnableDevCommands, discord, "enable-dev-commands", true);

#endif

            // reload log level
            {
                quill::LogLevel level;
                a(level, general, "log-level", quill::LogLevel::Info, nullptr);
                AVL_LOGGER->set_log_level(level);
            }

            if (m_settings.serverPassword.empty()) {
                LOG_INFO(AVL_LOGGER, "Server does not have a password");
            } else {
                LOG_NOTICE(AVL_LOGGER, "Server password is {}{}", COLOR_GOLD, m_settings.serverPassword);
            }
        }

        if (!reloading) {
            YAML::Emitter out;
            out.SetIndent(2);
            out << node;

            VUtils::Resource::WriteFile("server.yml", out.c_str());
        }
    }

    if (auto &&opt = VUtils::Resource::ReadFile<std::string>("blacklist.yml")) {
        try {
            auto node   = YAML::Load(*opt);
            m_blacklist = node.as<decltype(m_blacklist)>();
        } catch (const YAML::Exception &e) {
            LOG_ERROR(AVL_LOGGER, "{}", e.what());
        }
    }

    if (auto &&opt = VUtils::Resource::ReadFile<std::string>("whitelist.yml")) {
        try {
            auto node   = YAML::Load(*opt);
            m_whitelist = node.as<decltype(m_whitelist)>();
        } catch (const YAML::Exception &e) {
            LOG_ERROR(AVL_LOGGER, "{}", e.what());
        }
    }

    if (auto &&opt = VUtils::Resource::ReadFile<std::string>("admin.yml")) {
        try {
            auto node = YAML::Load(*opt);
            m_admin   = node.as<decltype(m_admin)>();
        } catch (const YAML::Exception &e) {
            LOG_ERROR(AVL_LOGGER, "{}", e.what());
        }
    }

#if AVL_IS_ON(AVL_DISCORD_INTEGRATION)
    if (m_settings.TEST_discordAccountLinking) {
        if (auto &&opt = VUtils::Resource::ReadFile<std::string>("discord-linked.yml")) {
            try {
                auto node                           = YAML::Load(*opt);
                DiscordManager()->m_linked_accounts = node.as<decltype(IDiscordManager::m_linked_accounts)>();
            } catch (const YAML::Exception &e) {
                LOG_ERROR(AVL_LOGGER, "{}", e.what());
            }
        }
    }
#endif

    if (reloading) {
        // then iterate players, settings active and inactive
        for (auto &&peer : NetManager()->GetPeers()) {
            peer->SetAdmin(m_admin.contains(peer->m_name));

            // TODO add a 'previously gated' bit
            //  so discord integration doesnt get messed up
            peer->SetGated(m_settings.TEST_playerRestrict);
        }
    }

    NetManager()->OnConfigLoad(reloading);

#ifdef _WIN32
    {
        //std::string title = m_settings.serverName + " - " + VConstants::GAME;
        std::string title
                = "Valhalla " + std::string(AVLEDET_VERSION) + " - Valheim " + std::string(VConstants::GAME);
        SetConsoleTitle(title.c_str());
    }
#endif

    std::error_code err;
    this->m_settingsLastTime = std::filesystem::last_write_time("server.yml", err);
}

void IAvledet::SaveFiles()
{
    {
        WorldManager()->GetWorld()->WriteFiles();
    }

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

#if AVL_IS_ON(AVL_DISCORD_INTEGRATION)
    {
        YAML::Node node(DiscordManager()->m_linked_accounts);

        YAML::Emitter emit;
        emit.SetIndent(2);

        emit << YAML::Comment("Discord-linked accounts") << YAML::Newline
             << YAML::Comment("Entries are in the form of 'steam-id: discord-id'") << node;

        VUtils::Resource::WriteFile("linked.yml", emit.c_str());
    }
#endif
}

avledet::util::UserID IAvledet::ID() const
{
    return m_serverID;
}

ServerSettings &IAvledet::Settings()
{
    return m_settings;
}

// Get the time since the server started
// Updated once per frame
std::chrono::nanoseconds IAvledet::Elapsed() const
{
    //return m_nowUpdate - m_startTime;
    return std::chrono::nanoseconds((std::int64_t)(
            (double) std::chrono::duration_cast<std::chrono::nanoseconds>(m_nowUpdate - m_startTime).count()
            * m_serverTimeMultiplier));
}

std::chrono::nanoseconds IAvledet::Nanos() const
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(Elapsed());
}

// Get the time in Ticks (C# DateTime.Ticks)
//auto Ticks() {
//    return duration_cast<avledet::util::Ticks>(Nanos());
//}

// Get the time in seconds (Unity Time.time)
float IAvledet::Time() const
{
    return (float) ((double) Nanos().count()
                    / (double) std::chrono::duration_cast<std::chrono::nanoseconds>(1s).count());
}

// The time in seconds since the last frame
float IAvledet::delta() const
{
    auto elapsed = m_nowUpdate - m_prevUpdate;
    return (float) (((double) elapsed.count() * m_serverTimeMultiplier)
                    / (double) std::chrono::duration_cast<decltype(elapsed)>(1s).count());
}

// The time in nanoseconds since the last frame
std::chrono::nanoseconds IAvledet::DeltaNanos() const
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(m_nowUpdate - m_prevUpdate);
}

void IAvledet::init()
{
    assert(!m_run_state && "unexpected run state during init(), did you call init() twice?");

    tracy::SetThreadName("game");

    {
        quill::BackendOptions options;
        options.enable_yield_when_idle = false;
        options.sleep_duration         = 1ms;

        quill::Backend::start(options);
    }

    {
        std::vector<std::shared_ptr<quill::Sink>> sinks {
                quill::Frontend::create_or_get_sink<quill::ConsoleSink>(
                        "server_con",
                        []() {
                            // See RotatingFileSinkConfig for more options

                            quill::ConsoleSinkConfig cfg;
                            //cfg.set_colour_mode(quill::ConsoleSinkConfig::ColourMode::Automatic);
                            //cfg

                            //cfg.set_open_mode('w');
                            //cfg.set_filename_append_option(quill::FilenameAppendOption::StartDateTime);
                            //cfg.set_rotation_time_daily("24:00");
                            //cfg.set_rotation_max_file_size(1024); // small value to demonstrate the example

                            return cfg;
                        }()),
                quill::Frontend::create_or_get_sink<quill::RotatingFileSink>(
                        "server.log",
                        []() {
                            // See RotatingFileSinkConfig for more options

                            quill::RotatingFileSinkConfig cfg;

                            cfg.set_open_mode('w');
                            cfg.set_filename_append_option(quill::FilenameAppendOption::StartDateTime);
                            cfg.set_rotation_time_daily("00:00");
                            //cfg.set_rotation_max_file_size(1024); // small value to demonstrate the example
                            //cfg.set_minimum_fsync_interval(); //fsync forces a disk write
                            //cfg.set_write_buffer_size()
                            //cfg.set_fsync_enabled(bool value)
                            //cfg.set_write_buffer_size(size_t value)

                            return cfg;
                        }()),
        };

        quill::PatternFormatterOptions options {
                "%(time) [%(thread_name)] %(short_source_location:<30) %(log_level:<9) %(message)",// format
                //"%D %H:%M:%S.%Qms",                                              // timestamp format
                "%H:%M:%S.%Qms",// timestamp format
                quill::Timezone::LocalTime};

        auto logger = quill::Frontend::create_or_get_logger("main", std::move(sinks), options);

        logger->set_log_level(quill::LogLevel::TraceL3);

        /* global assigned */ AVL_LOGGER = logger;
    }

    m_serverID  = VUtils::Random::GenerateUID();
    m_startTime = std::chrono::steady_clock::now();

    this->LoadFiles(false);

    LOG_NOTICE(AVL_LOGGER, "Starting Valhalla {} (Valheim {})", AVLEDET_VERSION, VConstants::GAME);

    //m_worldTime = 2040;
    m_worldTime = GetMorning(1);

    m_serverTimeMultiplier = 1;

    ZDOManager()->Init();
#if AVL_IS_ON(AVL_RANDOM_EVENTS)
    RandomEventManager()->Init();
#endif
    PrefabManager()->Init();

    ZoneManager()->PostPrefabInit();
#if AVL_IS_ON(AVL_DUNGEON_GENERATION)
    DungeonManager()->post_prefab_init();
#endif
    WorldManager()->PostZoneInit();
#if AVL_IS_ON(AVL_ZONE_GENERATION)
    GeoManager()->PostWorldInit();
    HeightmapBuilder()->PostGeoInit();
    ZoneManager()->PostGeoInit();
#endif

    WorldManager()->PostInit();
    NetManager()->PostInit();
#if AVL_IS_ON(AVL_ENABLE_SCRIPTING)
    ScriptManager()->PostInit();
#endif

#if AVL_IS_ON(AVL_DISCORD_INTEGRATION)
    DiscordManager()->init();
#endif

    AVL_DISPATCH_WEBHOOK("Server started");

    m_prevUpdate = std::chrono::steady_clock::now();
    m_nowUpdate  = m_prevUpdate;

    m_run_state = true;

#ifdef _WIN32
    SetConsoleCtrlHandler(
            [](DWORD dwCtrlType) {
#else                                              // !_WIN32
    signal(SIGINT, [](int) {
#endif                                             // !_WIN32
                tracy::SetThreadName("kernel");

                Avledet()->Stop();                 // set to true
                Avledet()->m_run_state.wait(false);// block until notify by uninit()
#ifdef _WIN32
                return TRUE;
            },
            TRUE);
#else // !_WIN32
    });
#endif// !_WIN32
}

void IAvledet::uninit()
{
    AVL_DISPATCH_WEBHOOK("Server stopping");

    LOG_INFO(AVL_LOGGER, "Terminating server");

    // Cleanup
    NetManager()->Uninit();
#if AVL_IS_ON(AVL_ZONE_GENERATION)
    HeightmapBuilder()->Uninit();
#endif

#if AVL_IS_ON(AVL_ENABLE_SCRIPTING)
    ScriptManager()->Uninit();
#endif

    this->SaveFiles();

    LOG_INFO(AVL_LOGGER, "Server was gracefully terminated");

    // notify
    m_run_state = true;
}

bool IAvledet::update()
{
    ZoneScoped;

    auto now     = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(m_nowUpdate - m_prevUpdate);

    m_prevUpdate = m_nowUpdate;// old state
    m_nowUpdate  = now;        // new state

    // Mutex is scoped
    {
        std::scoped_lock lock(m_taskMutex);
        for (auto itr = m_tasks.begin(); itr != m_tasks.end();) {
            auto ptr = itr->get();
            if (ptr->m_at < now) {
                if (ptr->m_period == std::chrono::milliseconds::min()) {// if task cancelled
                    itr = m_tasks.erase(itr);
                } else {
                    ptr->m_func(*ptr);
                    if (ptr->Repeats()) {
                        ptr->m_at += ptr->m_period;
                        ++itr;
                    } else
                        itr = m_tasks.erase(itr);
                }
            } else
                ++itr;
        }
    }

    // Update();
    // This is important to processing RPC remote invocations
    if (!NetManager()->GetPeers().empty()) {
        m_worldTime += delta() * m_worldTimeMultiplier;
    }

    AVL_SCRIPT_EVENT(IScriptManager::Events::Update);

    NetManager()->Update();
    ZDOManager()->Update();
    ZoneManager()->Update();
#if AVL_IS_ON(AVL_RANDOM_EVENTS)
    RandomEventManager()->Update();
#endif
#if AVL_IS_ON(AVL_ZONE_GENERATION)
    HeightmapBuilder()->Update();
#endif

    ScriptManager()->update();

    //TODO run periodically starting from now?
    if (VUtils::run_periodic<struct server_period_update>(1s)) {
        PeriodUpdate();
    }

    std::this_thread::sleep_for(1ms);

    FrameMark;

    return m_run_state;
}

void IAvledet::PeriodUpdate()
{
    if (VUtils::run_periodic<struct periodic_peer_print>(3min)) {
        LOG_INFO(AVL_LOGGER, "There are a total of {} peers online", NetManager()->GetPeers().size());
    }

    //PERIODIC_NOW(180s, {
    //    LOG_INFO(AVL_LOGGER, "There are a total of {} peers online", NetManager()->GetPeers().size());
    //});

    AVL_SCRIPT_EVENT(IScriptManager::Events::PeriodicUpdate);

#if AVL_IS_ON(AVL_DISCORD_INTEGRATION)
    DiscordManager()->period_update();
#endif

#if AVL_IS_ON(AVL_DUNGEON_REGENERATION)
    if (m_settings.dungeonsRegenerationInterval > 0s)
        DungeonManager()->TryRegenerateDungeons();
#endif


#if AVL_IS_ON(AVL_PLAYER_SLEEP)
    //if (m_settings.playerSleep) {
    if (m_playerSleep) {
        if (m_worldTime > m_playerSleepUntil) {
            // Wake up players

            if (m_settings.playerSleepSolo) {
                // only awake sleeping players
                for (auto &&peer : NetManager()->GetPeers()) {
                    auto &&zdo = peer->GetZDO();
                    if (zdo && zdo->GetBool(avledet::util::hashes::ZDO::Player::IN_BED, false)) {
                        RouteManager()->Invoke(peer->GetUserID(),
                                               avledet::util::hashes::Routed::S2C_RequestStopSleep);
                    }
                }
            } else {
                // wake every player
                RouteManager()->InvokeAll(avledet::util::hashes::Routed::S2C_RequestStopSleep);
            }

            m_playerSleep         = false;
            m_worldTimeMultiplier = 1;
        }
    } else {
        if (IsAfternoon() || IsNight()) {
            bool allInBed = true;
            bool anyInBed = false;

            for (auto &&peer : NetManager()->GetPeers()) {
                auto &&zdo = peer->GetZDO();
                bool inBed = zdo && zdo->GetBool(avledet::util::hashes::ZDO::Player::IN_BED, false);
                if (!inBed) {
                    allInBed = false;
                    if (!m_settings.playerSleepSolo)// early break if special sleep mode is not enabled
                        break;
                } else {
                    // Early break if the special sleep is enabled
                    if (m_settings.playerSleepSolo) {
                        anyInBed = true;
                        break;
                    }
                }
            }

            if ((allInBed || (anyInBed && m_settings.playerSleepSolo)) && !NetManager()->GetPeers().empty()) {
                m_playerSleep = true;

                // Skip to time
                m_playerSleepUntil = GetNextMorning();

                // Set skip interval
                m_worldTimeMultiplier = (m_playerSleepUntil - m_worldTime) / 12.0;

                if (m_settings.playerSleepSolo) {
                    // Players who are ALREADY in bed, go ahead and signal them to sleep
                    for (auto &&peer : NetManager()->GetPeers()) {
                        auto &&zdo = peer->GetZDO();
                        if (zdo && zdo->GetBool(avledet::util::hashes::ZDO::Player::IN_BED, false)) {
                            RouteManager()->Invoke(peer->GetUserID(),
                                                   avledet::util::hashes::Routed::S2C_RequestSleep);
                        } else {
                            peer->CornerMessage("The world is sleeping");
                        }
                    }
                } else {
                    // Just signal to all players to sleep
                    //  This assumes they are all already in bed
                    RouteManager()->InvokeAll(avledet::util::hashes::Routed::S2C_RequestSleep);
                }
            }
        }
    }
    //}
#endif


    std::error_code err;
    auto lastWriteTime = std::filesystem::last_write_time("server.yml", err);
    if (lastWriteTime != this->m_settingsLastTime) {
        // reload the file
        LOG_INFO(AVL_LOGGER, "Config change detected!");
        LoadFiles(true);
        LOG_INFO(AVL_LOGGER, "Config was reloaded");
    }

    if (m_settings.worldSaveInterval > 0s) {
        // save warming message
        if (VUtils::run_periodic_later<struct periodic_save_message>(m_settings.worldSaveInterval,
                                                                     m_settings.worldSaveInterval)) {
            LOG_INFO(AVL_LOGGER, "World saving in 30s");
            Broadcast(UIMsgType::Center, "$msg_worldsavewarning 30s");
        }

        if (VUtils::run_periodic_later<struct periodic_save>(m_settings.worldSaveInterval,
                                                             m_settings.worldSaveInterval + 30s)) {
            WorldManager()->GetWorld()->WriteFiles();
        }
    }
}

// Intended to be ran from ANY thread
void IAvledet::Stop()
{
    m_run_state = false;
}

void IAvledet::Start()
{
    this->init();

    while (this->update()) {}

    this->uninit();
}

Task &IAvledet::RunTask(Task::F f)
{
    return RunTaskLater(std::move(f), 0ms);
}

Task &IAvledet::RunTaskLater(Task::F f, std::chrono::milliseconds after)
{
    return RunTaskLaterRepeat(std::move(f), after, -1ms);
}

Task &IAvledet::RunTaskAt(Task::F f, std::chrono::steady_clock::time_point at)
{
    return RunTaskAtRepeat(std::move(f), at, -1ms);
}

Task &IAvledet::RunTaskRepeat(Task::F f, std::chrono::milliseconds period)
{
    return RunTaskLaterRepeat(std::move(f), 0ms, period);
}

Task &IAvledet::RunTaskLaterRepeat(Task::F f, std::chrono::milliseconds after,
                                   std::chrono::milliseconds period)
{
    return RunTaskAtRepeat(std::move(f), std::chrono::steady_clock::now() + after, period);
}

Task &IAvledet::RunTaskAtRepeat(Task::F f, std::chrono::steady_clock::time_point at,
                                std::chrono::milliseconds period)
{
    std::scoped_lock lock(m_taskMutex);
    m_tasks.push_back(std::make_unique<Task>(f, at, period));
    return *m_tasks.back();
}

void IAvledet::Broadcast(UIMsgType type, std::string_view text)
{
    RouteManager()->InvokeAll(avledet::util::hashes::Routed::S2C_UIMessage, type, text);
}
