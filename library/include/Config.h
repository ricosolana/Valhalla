#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <type_traits>
#include <yaml-cpp/yaml.h>
#include <magic_enum/magic_enum.hpp>

#include "CompileSettings.h"
#include "VUtilsString.h"

#if AVL_IS_ON(AVL_DISCORD_INTEGRATION)
    #include <dpp/snowflake.h>
#endif

// TODO rename to config
#define AVL_SETTINGS (Config::instance())

// TODO encapsulate in avl namespace

enum class ReplayMode {
    NONE,
    CAPTURE,
    PLAYBACK,
};

//TODO this is finicky at best, dangerous at worst,
//  consider majorly reworking, or removing it entirely...
enum class AssignAlgorithm
{
    NONE,
    DYNAMIC_RADIUS,
    RADIUS_LATENCY
};

class Config
{
public:
    static Config& instance() {
        static Config inst;
        return inst;
    }

    bool reloading() const;

    void load();

private:
    Config() = default;
    Config(const Config&) = delete;

private:
    bool m_first_load = true;

public:
    std::string serverName {};
    std::uint16_t serverPort {};
    std::string m_server_password {};
    bool serverPublic {};
    bool serverDedicated {};
    std::string serverBindAddress {};
    bool TEST_serverTcp {};

    bool playerWhitelist {};
    std::uint32_t playerMax {};
    bool playerOnline {};
    std::chrono::seconds playerTimeout {};
    std::chrono::milliseconds playerListSmoothUpdating {};
    bool playerListForceVisible {};
#if AVL_IS_ON(AVL_PLAYER_SLEEP)
    bool playerSleepSolo {};
#endif
    bool TEST_playerRestrict {};

    std::string worldName {};
    std::string worldSeed {};
    bool TEST_worldPregenerate;
    std::chrono::seconds worldSaveInterval {};// set to 0 to disable
    bool worldFeatures {};
    bool worldVegetation {};
    bool worldCreatures {};
    std::uint32_t worldHeightmapThreads {};

    std::uint32_t zdoMaxCongestion {};// congestion rate
    std::uint32_t zdoMinCongestion {};// congestion rate
    std::chrono::milliseconds zdoSendInterval {};
    std::chrono::seconds zdoAssignInterval {};
    AssignAlgorithm TEST_zdoAssignAlgorithm {};

    bool dungeonsEnabled {};
    bool dungeonsEndcapsEnabled {};
    float dungeonsEndcapsInsetFrac {};
    bool dungeonsDoors {};
    bool dungeonsRoomsFlipped {};
    bool dungeonsRoomsZoneBounded {};
    float dungeonsRoomsInsetSize {};
    bool dungeonsRoomsFurnishing {};
    std::chrono::seconds TEST_dungeonsRegenerationInterval {};
    std::uint32_t TEST_dungeonsRegenerationMaxSteps {};
    bool dungeonsSeeded {};

    float eventsChance {};
    std::chrono::seconds eventsInterval {};
    float eventsRadius {};
    bool eventsRequireKeys {};

#if AVL_IS_ON(AVL_DISCORD_INTEGRATION)
    bool discordEnabled {};
    std::string discordWebhook {};
    std::string discordToken {};
    dpp::snowflake discordGuild {};
    bool TEST_discordAccountLinking {};
    // Kick players who leave the Discord server?
    bool TEST_discordSyncLeaves {};
    // Sync kicks between Valheim and Discord?
    //bool            discordSyncKicks;
    // Sync bans between Valhiem and Discord?
    //bool            discordSyncBans;
#endif
    //bool            discordEnableDevCommands;
    //avledet::util::Set<dpp::snowflake> discordDevAccount;
    //bool            discordDeleteCommands;

    bool luaUnsafe {};

    //bool replay_enabled;

    ReplayMode m_replay_mode {};
    std::string m_replay_playback_path {};
    
    //bool replay_kick_on_fail;
};


namespace YAML {
    template<typename Enum>
        requires std::is_scoped_enum_v<Enum>
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
