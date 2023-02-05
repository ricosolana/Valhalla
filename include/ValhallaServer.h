#pragma once

#include <thread>

#include <robin_hood.h>

#include "Task.h"
#include "ServerSettings.h"
#include "NetAcceptor.h"
#include "VUtilsRandom.h"

#define SERVER_ID Valhalla()->ID()
#define SERVER_SETTINGS Valhalla()->Settings()

enum class MessageType : int32_t {
    TopLeft = 1,
    Center
};

class NetRpc;
class IWorldManager;

class IValhalla {
    friend class ValhallaLauncher;
    friend class IWorldManager;

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

    robin_hood::unordered_set<std::string> m_blacklist; // banned steam ids
    robin_hood::unordered_set<std::string> m_admin;     // admin steam ids
    robin_hood::unordered_set<std::string> m_whitelist; // whitelisted steam ids
    robin_hood::unordered_set<std::string> m_bypass;    // password-bypass steam ids

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

    void Broadcast(MessageType type, const std::string &text);

private:
    void Update();
};

IValhalla* Valhalla();
