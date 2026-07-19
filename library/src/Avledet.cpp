#include "Config.h"
#include "ReplayManager.h"
#include <chrono>
#include <mutex>
#include <stdlib.h>
#include <thread>
#include <type_traits>
#include <utility>
#ifdef _WIN32
//#include <winstring.h> //TODO getting many dumb errors 'LookuPContect' unknown override specifier'...
#endif

//#include <magic_enum.hpp> //TODO magic
#include <magic_enum/magic_enum.hpp>
#include <quill/core/LogLevel.h>
#include <quill/LogMacros.h>
#include <quill/sinks/RotatingFileSink.h>
#include <tracy/Tracy.hpp>
#include <yaml-cpp/yaml.h>

#include "Avledet.h"
#include "DiscordManager.h"
#include "DungeonManager.h"
#include "GeoManager.h"
#include "Hashes.h"
#include "HeightmapBuilder.h"
#include "ModManager.h"
#include "NetManager.h"
#include "RandomEventManager.h"
#include "RouteManager.h"
#include "VUtilsResource.h"
#include "VUtilsString.h"
#include "ZDOManager.h"
#include "ZoneManager.h"

// Defined
quill::Logger *AVL_LOGGER {};

auto AVLEDET_INSTANCE = std::make_unique<IAvledet>();

IAvledet *Avledet()
{
    return AVLEDET_INSTANCE.get();
}

void IAvledet::LoadFiles(bool reloading)
{
    Config::instance().load();

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
    if (AVL_SETTINGS.TEST_discordAccountLinking) {
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
            peer->SetAdmin(m_admin.contains(peer->m_socket->get_host_name()));

            // TODO add a 'previously gated' bit
            //  so discord integration doesnt get messed up
            peer->SetGated(AVL_SETTINGS.TEST_playerRestrict);
        }
    }

    NetManager()->OnConfigLoad(reloading);

#ifdef _WIN32
    {
        //std::string title = AVL_SETTINGS.serverName + " - " + VConstants::GAME;
        std::string title
                = "Avledet " + std::string(AVL_VERSION) + " - Valheim " + std::string(VConstants::GAME);
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

    LOG_NOTICE(AVL_LOGGER, "Starting Avledet {} (Valheim {})", AVL_VERSION, VConstants::GAME);

    //m_worldTime = 2040;
    m_worldTime = GetMorning(1);

    m_serverTimeMultiplier = 1;

    ZDOManager()->Init();
#if AVL_IS_ON(AVL_RANDOM_EVENTS)
    RandomEventManager()->Init();
#endif
    PrefabManager()->Init();

//TODO add Script Init here, with callback
//ScriptManager()->Init()// Will initially load ALL scripts (anything that immediately runs in the body can be considered pre zdo-init [occurs in worldmanager])
#if AVL_IS_ON(AVL_ENABLE_SCRIPTING)
    ScriptManager()->Init();
#endif

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

    if (AVL_SETTINGS.m_replay_mode != ReplayMode::NONE) {
        avledet::replay::ReplayManager()->init();
    }

    AVL_SCRIPT_EVENT(IScriptManager::Events::Enable);

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

#if AVL_IS_ON(AVL_ENABLE_SCRIPTING)
    ScriptManager()->update();
#endif

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

    AVL_SCRIPT_EVENT(IScriptManager::Events::PeriodicUpdate);

#if AVL_IS_ON(AVL_DISCORD_INTEGRATION)
    DiscordManager()->period_update();
#endif

#if AVL_IS_ON(AVL_DUNGEON_REGENERATION)
    if (AVL_SETTINGS.dungeonsRegenerationInterval > 0s)
        DungeonManager()->TryRegenerateDungeons();
#endif


#if AVL_IS_ON(AVL_PLAYER_SLEEP)
    //if (AVL_SETTINGS.playerSleep) {
    if (m_playerSleep) {
        if (m_worldTime > m_playerSleepUntil) {
            // Wake up players

            if (AVL_SETTINGS.playerSleepSolo) {
                // only awake sleeping players
                for (auto &&peer : NetManager()->GetPeers()) {
                    auto &&zdo = peer->find_zdo();
                    if (zdo && zdo->get_bool(avledet::util::hashes::ZDO::Player::IN_BED, false)) {
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
                auto &&zdo = peer->find_zdo();
                bool inBed = zdo && zdo->get_bool(avledet::util::hashes::ZDO::Player::IN_BED, false);
                if (!inBed) {
                    allInBed = false;
                    if (!AVL_SETTINGS.playerSleepSolo)// early break if special sleep mode is not enabled
                        break;
                } else {
                    // Early break if the special sleep is enabled
                    if (AVL_SETTINGS.playerSleepSolo) {
                        anyInBed = true;
                        break;
                    }
                }
            }

            if ((allInBed || (anyInBed && AVL_SETTINGS.playerSleepSolo)) && !NetManager()->GetPeers().empty()) {
                m_playerSleep = true;

                // Skip to time
                m_playerSleepUntil = GetNextMorning();

                // Set skip interval
                m_worldTimeMultiplier = (m_playerSleepUntil - m_worldTime) / 12.0;

                if (AVL_SETTINGS.playerSleepSolo) {
                    // Players who are ALREADY in bed, go ahead and signal them to sleep
                    for (auto &&peer : NetManager()->GetPeers()) {
                        auto &&zdo = peer->find_zdo();
                        if (zdo && zdo->get_bool(avledet::util::hashes::ZDO::Player::IN_BED, false)) {
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
    if (lastWriteTime != m_settingsLastTime) {
        // reload the file
        LOG_INFO(AVL_LOGGER, "Config change detected!");
        LoadFiles(true);
        LOG_INFO(AVL_LOGGER, "Config was reloaded");
    }

    if (AVL_SETTINGS.worldSaveInterval > 0s) {
        // save warming message
        if (VUtils::run_periodic_later<struct periodic_save_message>(AVL_SETTINGS.worldSaveInterval,
                                                                     AVL_SETTINGS.worldSaveInterval)) {
            LOG_INFO(AVL_LOGGER, "World saving in 30s");
            Broadcast(UIMsgType::Center, "$msg_worldsavewarning 30s");
        }

        if (VUtils::run_periodic_later<struct periodic_save>(AVL_SETTINGS.worldSaveInterval,
                                                             AVL_SETTINGS.worldSaveInterval + 30s)) {
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
