#include <stdlib.h>
#include <utility>
#include <charconv>

#include "ValhallaServer.h"
#include "VUtilsResource.h"
#include "ServerSettings.h"
#include "NetManager.h"
#include "ZoneManager.h"
#include "ZDOManager.h"
#include "RouteManager.h"
#include "Hashes.h"

quill::Logger *LOGGER = nullptr;

auto VALHALLA_INSTANCE(std::make_unique<IValhalla>());
IValhalla* Valhalla() {
    return VALHALLA_INSTANCE.get();
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
    m_startTime = std::chrono::steady_clock::now();

    //m_worldTime = 2040;
    m_worldTime = GetMorning(1);

    m_serverTimeMultiplier = 1;

    ZDOManager()->Init();

    ZoneManager()->PostPrefabInit();

    WorldManager()->PostZoneInit();
    ZoneManager()->PostGeoInit();

    WorldManager()->PostInit();
    NetManager()->PostInit();

    m_prevUpdate = std::chrono::steady_clock::now();
    m_nowUpdate = std::chrono::steady_clock::now();

    VH_DISPATCH_WEBHOOK("Server started");

    m_terminate = false;
    while (!m_terminate) {
        auto now = steady_clock::now();
        auto elapsed = duration_cast<std::chrono::nanoseconds>(m_nowUpdate - m_prevUpdate);

        m_prevUpdate = m_nowUpdate; // old state
        m_nowUpdate = now; // new state

        // Mutex is scoped
        {
            std::scoped_lock lock(m_taskMutex);
            for (auto itr = m_tasks.begin(); itr != m_tasks.end();) {
                auto ptr = itr->get();
                if (ptr->m_at < now) {
                    if (ptr->m_period < 0s) { // if task cancelled
                        itr = m_tasks.erase(itr);
                    }
                    else {
                        ptr->m_func(*ptr);
                        if (ptr->Repeats()) {
                            ptr->m_at += ptr->m_period;
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

    LOG_INFO(LOGGER, "Server was gracefully terminated");

    // signal any other dummy thread to continue
    m_terminate = false;
}



void IValhalla::Update() {
    // This is important to processing RPC remote invocations
    if (!NetManager()->GetPeers().empty()) {
        m_worldTime += Delta() * m_worldTimeMultiplier;
    }
    
    VH_DISPATCH_MOD_EVENT(IModManager::Events::Update);

    NetManager()->Update();
    ZDOManager()->Update();
    ZoneManager()->Update();
}

void IValhalla::PeriodUpdate() {
    PERIODIC_NOW(180s, {
        LOG_INFO(LOGGER, "There are a total of {} peers online", NetManager()->GetPeers().size());
    });
    
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

Task& IValhalla::RunTaskLater(Task::F f, std::chrono::milliseconds after) {
    return RunTaskLaterRepeat(std::move(f), after, -1ms);
}

Task& IValhalla::RunTaskAt(Task::F f, std::chrono::steady_clock::time_point at) {
    return RunTaskAtRepeat(std::move(f), at, -1ms);
}

Task& IValhalla::RunTaskRepeat(Task::F f, std::chrono::milliseconds period) {
    return RunTaskLaterRepeat(std::move(f), 0ms, period);
}

Task& IValhalla::RunTaskLaterRepeat(Task::F f, std::chrono::milliseconds after, std::chrono::milliseconds period) {
    return RunTaskAtRepeat(std::move(f), std::chrono::steady_clock::now() + after, period);
}

Task& IValhalla::RunTaskAtRepeat(Task::F f, std::chrono::steady_clock::time_point at, std::chrono::milliseconds period) {
    std::scoped_lock lock(m_taskMutex);
    m_tasks.push_back(std::make_unique<Task>(f, at, period));
    return *m_tasks.back();
}

void IValhalla::Broadcast(UIMsgType type, std::string_view text) {
    RouteManager()->InvokeAll(Hashes::Routed::S2C_UIMessage, type, text);
}
