#pragma once

#include <thread>

#include "Task.h"
#include "ServerSettings.h"
#include "VUtilsRandom.h"
#include "VUtilsMathf.h"
#include "HashUtils.h"

#define VH_ID Valhalla()->ID()
#define VH_SETTINGS Valhalla()->Settings()

enum class UIMsgType : int32_t {
    TopLeft = 1,
    Center
};

/*
// Will contain a bunch of logged player data, such as playtime, markers of position over time
//  Maybe draw a graphical map to show position over time? sounds cool...
//  Could consider using bStats to log misc information
struct PlayerInfo_t {

};*/



using WorldTime = double;
// relative time from 0 in the day cycle
using TimeOfDay = double;

static constexpr TimeOfDay TIME_MORNING = 240;
static constexpr TimeOfDay TIME_DAY = 270;
static constexpr TimeOfDay TIME_AFTERNOON = 900;
static constexpr TimeOfDay TIME_NIGHT = 1530;

class IValhalla {
    friend class IModManager;
    friend class IDiscordManager;
    friend class IWorldManager;
    friend class Tests;
    friend class World;

private:
    std::atomic_bool m_terminate {};

    std::list<std::unique_ptr<Task>> m_tasks;
    std::recursive_mutex m_taskMutex;

    ServerSettings m_settings {};
    OWNER_t m_serverID {}; // const

    steady_clock::time_point m_startTime; // const
    steady_clock::time_point m_prevUpdate;
    steady_clock::time_point m_nowUpdate;

    WorldTime m_worldTime {};

#if VH_IS_ON(VH_PLAYER_SLEEP)
    bool m_playerSleep {};
    double m_playerSleepUntil {};
#endif

    double m_worldTimeMultiplier = 1;

    fs::file_time_type m_settingsLastTime {};

private:
    void LoadFiles(bool reloading);
    void SaveFiles();

public:
    void Start();
    void Stop();

public:
    OWNER_t ID() const {
        return m_serverID;
    }

    ServerSettings& Settings() {
        return m_settings;
    }

    double m_serverTimeMultiplier = 1;

    UNORDERED_SET_t<std::string, ankerl::unordered_dense::string_hash, std::equal_to<>> m_blacklist; // banned steam ids
    UNORDERED_SET_t<std::string, ankerl::unordered_dense::string_hash, std::equal_to<>> m_admin;     // admin steam ids
    UNORDERED_SET_t<std::string, ankerl::unordered_dense::string_hash, std::equal_to<>> m_whitelist; // whitelisted steam ids
    //ankerl::unordered_dense::map<std::string, PlayerInfo_t, ankerl::unordered_dense::string_hash, std::equal_to<>> m_users;

    // Get the time since the server started
    // Updated once per frame
    auto Elapsed() {
        //return m_nowUpdate - m_startTime;
        return nanoseconds((int64_t)(duration_cast<nanoseconds>(m_nowUpdate - m_startTime).count() * m_serverTimeMultiplier));
    }

    auto Nanos() {
        return duration_cast<nanoseconds>(Elapsed());
    }

    // Get the time in Ticks (C# DateTime.Ticks)
    //auto Ticks() {
    //    return duration_cast<TICKS_t>(Nanos());
    //}

    // Get the time in seconds (Unity Time.time)
    float Time() {
        return float((double)Nanos().count() / (double)duration_cast<nanoseconds>(1s).count());
    }

    // The time in seconds since the last frame
    float Delta() {
        auto elapsed = m_nowUpdate - m_prevUpdate;
        return ((double)elapsed.count() * m_serverTimeMultiplier) / (double)duration_cast<decltype(elapsed)>(1s).count();
    }

    // The time in nanoseconds since the last frame
    auto DeltaNanos() {
        return duration_cast<nanoseconds>(m_nowUpdate - m_prevUpdate);
    }


    //static constexpr int WORLD_TIME_MORNING = 240;
    //static constexpr int WORLD_TIME_DAY = 270;
    //static constexpr int WORLD_TIME_AFTERNOON = 900;
    //static constexpr int WORLD_TIME_NIGHT = 1530;

    static constexpr int WORLD_TIME_LENGTH = 1800;



    // Get the current day given a world time
    static int GetDay(WorldTime worldTime) {
        return (worldTime - TIME_DAY) / WORLD_TIME_LENGTH;
    }
    // Get the time relative to a given world time
    //  Returns a world time ranging [0, WORLD_TIME_LENGTH)
    static TimeOfDay GetTimeOfDay(WorldTime worldTime) {
        auto wrapped = std::fmod(worldTime, WORLD_TIME_LENGTH);
        if (wrapped < 0)
            wrapped += WORLD_TIME_LENGTH;
        return wrapped;
    }
    // Get the time of day given a day
    static WorldTime GetWorldTime(int day, TimeOfDay timeOfDay) {
        return (double) day * (double) WORLD_TIME_LENGTH 
            + timeOfDay;
    }
    static bool IsMorning(TimeOfDay timeOfDay) {
        return timeOfDay >= TIME_MORNING && timeOfDay < TIME_DAY;
    }
    static bool IsDay(TimeOfDay timeOfDay) {
        return timeOfDay >= TIME_DAY && timeOfDay < TIME_AFTERNOON;
    }
    static bool IsAfternoon(TimeOfDay timeOfDay) {
        return timeOfDay >= TIME_AFTERNOON && timeOfDay < TIME_NIGHT;
    }
    static bool IsNight(TimeOfDay timeOfDay) {
        return timeOfDay >= TIME_NIGHT || timeOfDay < TIME_MORNING;
    }
    // Get the morning time of the next given day
    static WorldTime GetMorning(int day) {
        return GetWorldTime(day, TIME_MORNING);
    }
    // Get the day time of the next given day
    static WorldTime GetDay(int day) {
        return GetWorldTime(day, TIME_DAY);
    }
    // Get the morning time of the next given day
    static WorldTime GetAfternoon(int day) {
        return GetWorldTime(day, TIME_AFTERNOON);
    }
    // Get the night time of the next given day
    static WorldTime GetNight(int day) {
        return GetWorldTime(day, TIME_NIGHT);
    }
    // Get the next immediate world time for a given time of day and given world time
    static WorldTime GetNextTime(WorldTime worldTime, TimeOfDay timeOfDay) {
        return GetWorldTime(
            GetDay(worldTime) + (GetTimeOfDay(worldTime) < timeOfDay ? 0 : 1),
            timeOfDay
        );
    }



    // Returns game server time
    //  World time remains unchanged as players are offline
    WorldTime GetWorldTime() const {
        return this->m_worldTime;
    }

    void SetWorldTime(WorldTime worldTime) {
        this->m_worldTime = worldTime;
    }

    TICKS_t GetWorldTicks() const {
        return TICKS_t((int64_t)(this->m_worldTime * (WorldTime)TICKS_t::period::den));
    }


    // Get the current day
    int GetDay() const {
        return GetDay(this->m_worldTime);
    }
    // Set the current day
    //  Maintains the relative world time
    void SetDay(int day) {
        auto timeOfDay = GetTimeOfDay();

        m_worldTime = (double)day * (double)WORLD_TIME_LENGTH + timeOfDay;
    }


    // Get the time relative to this cycle
    TimeOfDay GetTimeOfDay() const {
        return GetTimeOfDay(this->m_worldTime);
    }
    // Set the relative time to this day
    //  The day is unchanged
    void SetTimeOfDay(TimeOfDay timeOfDay) {
        timeOfDay = GetTimeOfDay(timeOfDay); // normalize
        this->m_worldTime = (double)GetDay() * (double)WORLD_TIME_LENGTH + timeOfDay;
    }


    // Time of day functions
    bool IsMorning() const {
        return IsMorning(GetTimeOfDay(m_worldTime));
    }
    bool IsDay() const {
        return IsDay(GetTimeOfDay(m_worldTime));
    }
    bool IsAfternoon() const {
        return IsAfternoon(GetTimeOfDay(m_worldTime));
    }
    bool IsNight() const {
        return IsNight(GetTimeOfDay(m_worldTime));
    }   



    // Get the morning time of the next cycle
    double GetTomorrowMorning() const {
        return GetWorldTime(GetDay() + 1, TIME_MORNING);
    }
    // Get the day time of the next cycle
    double GetTomorrowDay() const {
        return GetWorldTime(GetDay() + 1, TIME_DAY);
    }
    // Get the morning time of the next cycle
    double GetTomorrowAfternoon() const {
        return GetWorldTime(GetDay() + 1, TIME_AFTERNOON);
    }
    // Get the night time of the next cycle
    double GetTomorrowNight() const {
        return GetWorldTime(GetDay() + 1, TIME_NIGHT);
    }



    // Get the next immediate morning time
    double GetNextMorning() const {
        return GetNextTime(m_worldTime, TIME_MORNING);
    }
    
    // Get the next immediate afternoon time
    double GetNextAfternoon() const {
        return GetNextTime(m_worldTime, TIME_AFTERNOON);
    }
    // Get the next immediate night time
    double GetNextNight() const {
        return GetNextTime(m_worldTime, TIME_NIGHT);
    }



    Task& RunTask(Task::F f);
    Task& RunTaskLater(Task::F f, std::chrono::milliseconds after);
    Task& RunTaskAt(Task::F f, std::chrono::steady_clock::time_point at);
    Task& RunTaskRepeat(Task::F f, std::chrono::milliseconds period);
    Task& RunTaskLaterRepeat(Task::F f, std::chrono::milliseconds after, std::chrono::milliseconds period);
    Task& RunTaskAtRepeat(Task::F f, std::chrono::steady_clock::time_point at, std::chrono::milliseconds period);

    void Broadcast(UIMsgType type, std::string_view text);

private:
    void Update();
    void PeriodUpdate();
};

IValhalla* Valhalla();
