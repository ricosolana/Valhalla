#include <filesystem>
#include <functional>
#include <quill/core/LogLevel.h>
#include <quill/LogMacros.h>
#include <quill/sinks/RotatingFileSink.h>
#include <tracy/Tracy.hpp>
#include <yaml-cpp/node/node.h>

#include "VUtilsString.h"
#include "VUtilsResource.h"
#include "VUtilsRandom.h"
#include "Avledet.h"
#include "Config.h"

template<typename T>
struct type_identity { using type = T; };

template<typename T>
using type_identity_t = typename type_identity<T>::type;

// r = raw / reloadable
//  will always load the value from config, even on mid-server states
//template<typename T, typename D, typename Func = std::nullptr_t>
//    requires(std::is_same_v<Func, std::nullptr_t>
//             || ((is_duration<typename std::tuple_element_t<
//                          0, typename VUtils::Traits::func_traits<Func>::args_type>>::value
//                  && is_duration<T>::value)
//                 == is_duration<D>::value))
//void tieValue(T &set, YAML::Node mutableNode, std::string const &key, D const &default_value,
//       Func validator = nullptr)
//{
template<bool once, typename T, typename F = std::nullptr_t>
constexpr void tiedParse(T &mut, YAML::Node mutableNode, std::string const &key,
                          std::type_identity_t<T> default_value,
                          F validator = nullptr)
{
    const bool reloading = ConfigManager::instance().reloading();
    if (once && reloading) {
        return;
    }

    YAML::Node mapping = mutableNode[key];
    bool assigned = false;

    try {
        auto val = mapping.as<T>();

        bool rejected = false;
        if constexpr (!std::is_same_v<F, std::nullptr_t>) {
            using Param0 = std::remove_cvref_t<
                std::tuple_element_t<0, typename VUtils::Traits::func_traits<F>::args_type>>;

            if constexpr (avledet::util::traits::is_duration<T>::value 
                && avledet::util::traits::is_duration<Param0>::value) {
                rejected = validator(std::chrono::duration_cast<Param0>(val));
            } else {
                rejected = validator(val);
            }
        }

        if (!rejected) {
            if (!reloading || mut != val) {
                if (reloading) {
                    //LOG_DEBUG(AVL_LOGGER, "reload: [{}]: {} -> {}", key, mut, val);
                }
                assigned = true;
                default_value = std::move(val);
            }
        }
    } catch (YAML::Exception const&) {
    }

    if (assigned) {
        mut = std::move(default_value);
    } else {
        mapping = default_value;
        assert(mutableNode[key].IsDefined());

        if constexpr (avledet::util::traits::is_duration<T>::value) {
            mut = std::chrono::duration_cast<T>(default_value);
        } else {
            mut = T(default_value);
        }
    }
}

// r = reloadable values
template<class... Args>
void r(Args &&... args)
{
    return tiedParse<false>(std::forward<Args>(args)...);
}

// c = const loading
//  for values to be loaded once during server init
template<class... Args>
void c(Args &&... args)
{
    return tiedParse<true>(std::forward<Args>(args)...);
    //if (!ConfigManager::instance().reloading()) {
    //    //return r(std::forward<Args>(args)...);
    //    return tiedParse(std::forward<Args>(args)...);
    //}
}

bool ConfigManager::reloading() const {
    return !m_first_load;
}

void ConfigManager::load()
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
                if (m_first_load) {
                    LOG_INFO(AVL_LOGGER, "Server config not found, creating...");
                }
                fileError = true;
            }
        }

        // If the server has just started or theres no config error
        if (m_first_load || !fileError) {
            auto &&general  = node["general"];
            auto &&server   = node["server"];
            auto &&players  = node["players"];
            auto &&world    = node["world"];
            auto &&zdo      = node["zdos"];
            auto &&dungeons = node["dungeons"];
            auto &&events   = node["events"];
            auto &&discord  = node["discord"];
            //auto &&replay  = node["replay"];
            auto &&experimental = node["experimental"];

            /*
                Server settings
            */

            r(m_server_name, server, "name", "Avledet server",
              [](std::string const &val) { return val.empty() || val.length() < 3 || val.length() > 64; });
            r(m_server_password, server, "password", "",
              [](std::string const &val) { return !val.empty() && (val.length() < 5 || val.length() > 11); });
            c(m_server_port, server, "port", 2456, nullptr);
            r(m_server_public, server, "public", false, nullptr);
            c(m_server_dedicated, server, "dedicated", true, nullptr);
            c(m_server_address, server, "bind-address", "0.0.0.0", nullptr);
            c(m_server_tcp, experimental, "server-tcp", false, nullptr);

            /*
                Player settings
            */

            r(m_player_whitelist_on, players, "whitelist", true, nullptr);
            r(m_player_limit, players, "max-online", 10, [](int val) { return val < 1; });
            r(m_player_auth, players, "authenticate", true, nullptr);
            // If timeout is 0, will never timeout
            r(m_player_timeout, players, "timeout", 30s,
              [](std::chrono::seconds val) { return val < 0s; });
#if AVL_IS_ON(AVL_PLAYER_SLEEP)
            a(playerSleepSolo, players, "player-sleep-solo", false, nullptr);
#endif
            // TODO put under discord option
            c(m_discord_player_restrict, experimental, "players-restrict", false, nullptr);

            {
                auto &&player_list = players["playerlist"];
                r(playerListSmoothUpdating, players, "smooth-updating", 2s,
                  [](std::chrono::seconds val) { return val < 0s; });
                r(playerListForceVisible, players, "locations-always-on", false, nullptr);
            }

            /*
                World generation settings
            */

            c(
                    m_world_name, world, "world", "world",
                    [](std::string const &val) { return val.empty() || val.length() < 3; });
            c(
                    m_world_seed, world, "seed", VUtils::Random::GenerateAlphaNum(10),
                    [](std::string const &val) { return val.empty(); });
            c(m_world_pregenerate, experimental, "world-pregenerate", false, nullptr);
            r(m_world_save_interval, world, "save-interval", 30min,
              [](std::chrono::seconds val) { return val < 0s; });
            // TODO what should the hot reload behaviour of 'features' changes be?
            //  Generate locations? Place only generated locations?
            c(m_world_gen_features, world, "features", true, nullptr);
            r(m_world_gen_vegetation, world, "vegetation", true, nullptr);
            r(m_world_gen_creatures, world, "creatures", true, nullptr);
            c(m_world_heightmap_threads, world, "heightmap-threading", 1, nullptr);

            // limit to physically available threads
            if (m_world_heightmap_threads == 0
                || m_world_heightmap_threads > std::jthread::hardware_concurrency())
                m_world_heightmap_threads = std::jthread::hardware_concurrency();

            // If desired threads is set to max threads, decrement by 1 (because main thread exists duh)
            if (std::jthread::hardware_concurrency() > 1
                && m_world_heightmap_threads >= std::jthread::hardware_concurrency())
                m_world_heightmap_threads = std::jthread::hardware_concurrency() - 1;

            /*
                ZDO traffic settings
            */

            r(m_zdo_send_interval, zdo, "send-interval", 50ms,
              [](std::chrono::seconds val) { return val <= 0s; });
            r(m_zdo_max_congestion, zdo, "max-send-threshold", 10240,
              [](int val) { return val < 1000; });
            r(m_zdo_min_congestion, zdo, "min-send-threshold", 2048,
              [](int val) { return val < 1000; });
            r(m_zdo_assign_interval, zdo, "assign-interval", 2s,
              [](std::chrono::seconds val) { return val < 1s; });
            r(m_zdo_owner_algo, experimental, "zdo-assign-algo", AssignAlgorithm::NONE,
              nullptr);

            /*
                Dungeon generation settings
            */

            r(m_dng_enabled, dungeons, "enabled", true, nullptr);
            {
                auto &&endcaps = dungeons["endcaps"];
                r(m_dng_endcaps_enabled, endcaps, "enabled", true, nullptr);
                r(m_dng_endcaps_inset_ratio, endcaps, "inset-ratio", .5f,
                  [](float val) { return val < 0.f || val > 1.f; });
            }

            r(m_dng_doors_enabled, dungeons, "doors", true, nullptr);

            {
                auto &&rooms = dungeons["rooms"];
                r(m_dng_rooms_flipped, rooms, "flipped", true, nullptr);
                r(m_dng_rooms_zone_bounded, rooms, "zone-bounded", true, nullptr);
                r(m_dng_rooms_inset, rooms, "inset-size", .1f,
                  [](float val) { return val < 0; });
                r(m_dng_rooms_decorated, rooms, "furnishing", true, nullptr);
            }

            // TODO test out dungeon regeneration
            //{
            //    auto &&regeneration = dungeons["dungeons-regeneration"];
            //    a(TEST_dungeonsRegenerationInterval, regeneration, "interval",
            //      std::chrono::days(3), [](std::chrono::minutes val) { return val < 5s; });
            //    a(TEST_dungeonsRegenerationMaxSteps, regeneration, "steps", 3,
            //      [](int val) { return val < 1; });
            //}

            r(m_dng_seeded, dungeons, "seeded", true, nullptr);

            /*
                Random event / raid settings
            */

            r(m_raids_chance, events, "chance", .2f, [](float val) { return val < 0 || val > 1; });
            r(m_raids_interval, events, "interval", 46min,
              [](std::chrono::seconds val) { return val < 0s; });
            r(m_raids_radius, events, "activation-radius", 96,
              [](float val) { return val < 1 || val > 96 * 4; });
            r(m_raids_require_keys, events, "require-keys", true, nullptr);

            /*
                Discord settings
            */

#if AVL_IS_ON(AVL_DISCORD_INTEGRATION)
            a(discordEnabled, discord, "enabled", false, nullptr, reloading);
            a(discordWebhook, discord, "webhook", "",
              nullptr);  //TODO move this somewhere more secure
            a(discordToken, discord, "token", "", nullptr,
              reloading);//TODO move this somewhere more secure!!!
            a(discordGuild, discord, "guild", 0, nullptr, reloading);
            a(TEST_discordAccountLinking, discord, "experimental-account-linking", false, nullptr,
              reloading);
            a(TEST_discordSyncLeaves, discord, "experimental-sync-leaves", false, nullptr,
              reloading);
            //a(discordDeleteCommands, discord, "delete-commands", false, nullptr, reloading);

            //a(discordDevAccount, discord, "dev-account", avledet::util::Set<std::string>());

            //a(discordEnableDevCommands, discord, "enable-dev-commands", true);

#endif

            // reload log level
            {
                quill::LogLevel level;
                r(level, general, "log-level", quill::LogLevel::Info, nullptr);
                AVL_LOGGER->set_log_level(level);
            }

            {
                c(m_lua_unsafe, general, "lua-unsafe", false, nullptr);

                if (m_lua_unsafe) {
                    LOG_WARNING(AVL_LOGGER, "Unsafe Lua is enabled! This allows potentially unsafe code to run!");
                    LOG_WARNING(AVL_LOGGER, "Make sure you are running trusted scripts! Otherwise bad things could happen...");
                }
            }

            // replay loads
            {
                c(m_replay_mode, experimental, "replay-mode", ReplayMode::NONE, nullptr);
                c(m_replay_playback_path, experimental, "playback-path", "", nullptr);
                //a(replay_kick_on_fail, experimental, "replays-kick-on-fail", false, nullptr, reloading);
            }

            if (m_server_password.empty()) {
                if (!m_player_whitelist_on) {
                    LOG_WARNING(AVL_LOGGER, "Server does not have a password");
                }
            } else {
                LOG_NOTICE(AVL_LOGGER, "Server password is {}{}", COLOR_GOLD, m_server_password);
            }
        }

        // save config on server init
        if (m_first_load) {
            YAML::Emitter out;
            out.SetIndent(2);
            out << node;

            const std::filesystem::path current = "server.yml";
            const std::filesystem::path backup  = "server.yml.old";
            const std::filesystem::path temp    = "server.yml.tmp";

            if (VUtils::Resource::WriteFile(temp, out.c_str())) {
                // mv the '.yml' to '.old'
                // mv the '.tmp' to '.yml'
                std::error_code ec;

                // apparently windows is shit, as usual...
                //  - 'rename' may not overwrite
                std::filesystem::remove(backup, ec);

                std::filesystem::rename(current, backup, ec);
                if (ec) {
                    LOG_ERROR(AVL_LOGGER, "Failed to create yml backup (.old)");
                } else {
                    std::filesystem::rename(temp, current, ec);
                    if (ec) {
                        LOG_ERROR(AVL_LOGGER, "Failed to move yml temp (.tmp)");
                    }
                }
            }
        }
    }

    m_first_load = false;
}