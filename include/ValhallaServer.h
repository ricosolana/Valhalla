#pragma once

#include <thread>
#include "Task.h"
#include "ServerSettings.h"
#include "NetAcceptor.h"
#include "VUtilsRandom.h"

#define SERVER_ID Valhalla()->ID()
#define SERVER_SETTINGS Valhalla()->Settings()

class NetRpc;

class IValhalla {
    friend class ValhallaLauncher;

private:
    std::atomic_bool m_running; // mostly const

    std::list<std::unique_ptr<Task>> m_tasks;
    std::recursive_mutex m_taskMutex;

    ServerSettings m_settings;
    OWNER_t m_serverID; // const

    steady_clock::time_point m_startTime; // const
    steady_clock::time_point m_prevUpdate;
    steady_clock::time_point m_nowUpdate;

    double m_netTime;

private:
    void LoadFiles();

    void Start();
    void Stop();

public:
    OWNER_t ID() const {
        return m_serverID;
    }

    ServerSettings& Settings() {
        return m_settings;
    }

    robin_hood::unordered_set<std::string> m_banned;

    // Get the time since the server started
    // Updated once per frame
    auto Elapsed() {
        return m_nowUpdate - m_startTime;
    }

    auto Nanos() {
        return duration_cast<nanoseconds>(Elapsed());
    }

    // Get the time in Ticks (C# DateTime.Ticks)
    auto Ticks() {
        return duration_cast<TICKS_t>(Nanos());
    }

    // Get the time in seconds (Unity Time.time)
    float Time() {
        return float((double)Nanos().count() / (double)duration_cast<nanoseconds>(1s).count());
    }

    // Get the time in seconds since the last frame
    float Delta() {
        auto elapsed = m_nowUpdate - m_prevUpdate;
        return (double)elapsed.count() / (double)duration_cast<decltype(elapsed)>(1s).count();
    }

    // Returns game server time
    // Will freeze as long as players are offline
    double NetTime() {
        return m_netTime;
    }
    
    bool IsPeerAllowed(NetRpc* rpc);

    Task& RunTask(Task::F f);
    Task& RunTaskLater(Task::F f, std::chrono::milliseconds after);
    Task& RunTaskAt(Task::F f, std::chrono::steady_clock::time_point at);
    Task& RunTaskRepeat(Task::F f, std::chrono::milliseconds period);
    Task& RunTaskLaterRepeat(Task::F f, std::chrono::milliseconds after, std::chrono::milliseconds period);
    Task& RunTaskAtRepeat(Task::F f, std::chrono::steady_clock::time_point at, std::chrono::milliseconds period);

private:
    void Update();
};

IValhalla* Valhalla();
