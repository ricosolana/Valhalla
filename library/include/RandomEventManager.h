#pragma once

#include "DataStream.h"
#include "Types.h"
#include "Vector.h"
#include "VUtils.h"

class IRandomEventManager
{
  public:
    class Event
    {
      public:
        avledet::util::Set<std::string, ankerl::unordered_dense::string_hash, std::equal_to<>>
                m_presentGlobalKeys;
        avledet::util::Set<std::string, ankerl::unordered_dense::string_hash, std::equal_to<>>
                m_absentGlobalKeys;
        std::string m_name;
        std::chrono::nanoseconds m_duration {};
        avledet::util::Biome m_biome {};
        bool m_nearBaseOnly {};
        bool m_pauseIfNoPlayerInArea {};

      public:
        Event() {}
    };

  public:
    avledet::util::Map<std::string_view, std::unique_ptr<Event>> m_events;

    // Event timer for next event attempt
    float m_eventIntervalTimer = 0;

  private:
    // The current random active event in the world
    //	null means no event is active
    Event const *m_activeEvent {};
    Vector3f m_activeEventPos;
    std::chrono::nanoseconds m_activeEventRemaining;
    std::chrono::nanoseconds m_activeEventInitialDuration;

  private:
    void SendCurrentRandomEvent();

    std::optional<std::pair<std::reference_wrapper<Event const>, Vector3f>> GetPossibleRandomEvent();

    // Checks whether the server has or doesnt have
    //	the global keys requested of the event
    bool CheckGlobalKeys(Event const &e);

  public:
    void Init();
    void Update();

    void SetCurrentRandomEvent(Event const &e, Vector3f pos, std::chrono::nanoseconds ns);

    // Get an event by name
    //	Returns null if not found
    Event const *GetEvent(std::string_view name)
    {
        auto &&find = m_events.find(name);
        if (find != m_events.end())
            return find->second.get();
        return nullptr;
    }

    void Save(DataWriter &writer);
    void Load(DataReader &reader, int version);
};

IRandomEventManager *RandomEventManager();
