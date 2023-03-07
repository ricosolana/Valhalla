#pragma once

#include <thread>

#include <robin_hood.h>

#include "Task.h"
#include "ServerSettings.h"
#include "NetAcceptor.h"
#include "VUtilsRandom.h"
#include "VUtilsMathf.h"

#define SERVER_ID Valhalla()->ID()
#define SERVER_SETTINGS Valhalla()->Settings()

enum class UIMsgType : int32_t {
    TopLeft = 1,
    Center
};

class NetRpc;
class IWorldManager;

class IValhalla {
    friend class ValhallaLauncher;
    friend class IWorldManager;
    friend class Tests;

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

    bool m_playerSleep = false;
    double m_playerSleepUntil = 0;

    double m_timeMultiplier = 1;

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

    float DayFrac() {
        float m_totalSeconds = NetTime();

        // with nettime wrapped by 1800
        //  morning?: 240 (2040 - 1800)
        //  day: 270
        //  night: 

        //int64_t num = m_totalSeconds;
        //double num2 = m_totalSeconds * 1000.0;
        //int64_t num3 = 1260 * 1000L;
        //float num4 = VUtils::Mathf::Clamp01(std::fmod(m_totalSeconds * 1000.0f, 1260) / 1260.f);
        //num4 = this.RescaleDayFraction(num4);
        //float smoothDayFraction = this.m_smoothDayFraction;
        //float t = Mathf.LerpAngle(this.m_smoothDayFraction * 360f, num4 * 360f, 0.01f);
        //this.m_smoothDayFraction = Mathf.Repeat(t, 360f) / 360f;
    }

    float NetTimeWrapped() const {
        return std::fmod(m_netTime, 1800);
    }

    bool IsMorning() const {
        auto time = NetTimeWrapped();
        return time >= 240 && time < 270;
    }

    // Time of day functions
    bool IsDay() const {
        auto time = NetTimeWrapped();
        return time >= 270 && time < 900;
    }
    
    bool IsAfternoon() const {
        auto time = NetTimeWrapped();
        return time >= 900 && time < 1530;
    }

    bool IsNight() const {
        auto time = NetTimeWrapped();
        return time >= 1530 || time < 240;
    }

    int GetDay() const {
        return (m_netTime - 270.0) / 1800.0;
    }

    double GetNextMorning() const {
        auto day = GetDay();
        return ((day + 1) * 1800) + 240;
    }

    bool IsPeerAllowed(NetRpc* rpc);

    Task& RunTask(Task::F f);
    Task& RunTaskLater(Task::F f, std::chrono::milliseconds after);
    Task& RunTaskAt(Task::F f, std::chrono::steady_clock::time_point at);
    Task& RunTaskRepeat(Task::F f, std::chrono::milliseconds period);
    Task& RunTaskLaterRepeat(Task::F f, std::chrono::milliseconds after, std::chrono::milliseconds period);
    Task& RunTaskAtRepeat(Task::F f, std::chrono::steady_clock::time_point at, std::chrono::milliseconds period);

    void Broadcast(UIMsgType type, const std::string &text);

private:
    void Update();
    void PeriodUpdate();
};

IValhalla* Valhalla();
