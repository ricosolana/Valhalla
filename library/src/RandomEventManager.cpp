#include <quill/LogMacros.h>
#include <string_view>

#include "DiscordManager.h"
#include "GeoManager.h"
#include "Hashes.h"
#include "NetManager.h"
#include "Prefab.h"
#include "RandomEventManager.h"
#include "RouteManager.h"
#include "ValhallaServer.h"
#include "VUtilsResource.h"
#include "ZDOManager.h"
#include "ZoneManager.h"

auto RANDOM_EVENT_MANAGER = std::make_unique<IRandomEventManager>();

IRandomEventManager *RandomEventManager()
{
    return RANDOM_EVENT_MANAGER.get();
}

void IRandomEventManager::Init()
{
    // interval: 46
    // chance: 20
    // range: 96

    LOG_NOTICE(VH_LOGGER, "Initializing EventManager");

    {
        // load Foliage:
        auto opt = VUtils::Resource::ReadFile<avledet::util::Bytes>("randomEvents.pkg");
        if (!opt)
            throw std::runtime_error("randomEvents.pkg missing");

        DataReader pkg(*opt);

        auto comment = pkg.read<std::string_view>();// comment
        LOG_DEBUG(VH_LOGGER, "pkg comment: {}", comment);

        auto ver = pkg.read<std::string_view>();
        if (ver != VConstants::GAME) {
            LOG_WARNING(VH_LOGGER, "randomEvents.pkg uses different game version than server ({})", ver);
        }

        auto count = pkg.read<std::int32_t>();
        for (int i = 0; i < count; i++) {
            auto e = std::make_unique<Event>();

            e->m_name     = pkg.read<std::string>();
            e->m_duration = duration_cast<std::chrono::nanoseconds>(
                    std::chrono::seconds((std::int64_t) pkg.read<float>()));
            e->m_nearBaseOnly          = pkg.read<bool>();
            e->m_pauseIfNoPlayerInArea = pkg.read<bool>();
            e->m_biome                 = (avledet::util::Biome) pkg.read<std::int32_t>();

            e->m_presentGlobalKeys = pkg.read<decltype(Event::m_presentGlobalKeys)>();
            e->m_absentGlobalKeys  = pkg.read<decltype(Event::m_absentGlobalKeys)>();

            auto &&sv    = e->m_name;// Make a temp key because assign eval order is not guaranteed
            m_events[sv] = std::move(e);
        }

        LOG_NOTICE(VH_LOGGER, "Loaded {} random events", count);
    }
}

void IRandomEventManager::Update()
{
    ZoneScoped;

    // update event timer if an event is active
    if (m_activeEvent) {
        // Update the timer of the current event
        if (!m_activeEvent->m_pauseIfNoPlayerInArea
            || ZDOManager()->AnyZDO(this->m_activeEventPos, VH_SETTINGS.eventsRadius,
                                    avledet::util::hashes::Object::Player, Prefab::Flag::NONE,
                                    Prefab::Flag::NONE))
            //m_activeEventTimer += Valhalla()->Delta();
            m_activeEventRemaining -= Valhalla()->DeltaNanos();

        //if (m_activeEventTimer > this->m_activeEvent->m_duration) {
        if (m_activeEventRemaining <= 0ns) {
            VH_DISPATCH_WEBHOOK("Random event stopped: `" + this->m_activeEvent->m_name + "`");

            m_activeEvent    = nullptr;
            m_activeEventPos = Vector3f::ZERO;
        }
    } else if (VH_SETTINGS.eventsInterval > 0s) {
        m_eventIntervalTimer += Valhalla()->delta();

        // try to set a new current event
        if (m_eventIntervalTimer > VH_SETTINGS.eventsInterval.count()) {
            m_eventIntervalTimer = 0;
            if (VUtils::Random::State().next_float() <= VH_SETTINGS.eventsChance) {

                if (auto opt = GetPossibleRandomEvent()) {
                    auto &&e   = opt.value().first;
                    auto &&pos = opt.value().second;

                    SetCurrentRandomEvent(e.get(), pos, e.get().m_duration);

                    /*
					this->m_activeEvent = &e.get();
					this->m_activeEventPos = pos;
					this->m_activeEventRemaining = this->m_activeEvent->m_duration;

					LOG_INFO(VH_LOGGER, "Set current random event: {}", e.get().m_name);

					VH_DISPATCH_WEBHOOK("Random event started in world `" + this->m_activeEvent->m_name + "`");

					// send event
					//SendCurrentRandomEvent();
					*/
                }
            }
        }
    }

    if (VUtils::run_periodic<struct send_events>(1s)) {
        SendCurrentRandomEvent();
    }
}

void IRandomEventManager::SetCurrentRandomEvent(Event const &e, Vector3f pos, std::chrono::nanoseconds nanos)
{
    this->m_activeEvent                = &e;
    this->m_activeEventPos             = pos;
    this->m_activeEventRemaining       = nanos;
    this->m_activeEventInitialDuration = nanos;

    LOG_INFO(VH_LOGGER, "Set current random event: {}", e.m_name);
    VH_DISPATCH_WEBHOOK("Random event started in world `" + e.m_name + "`");
}

std::optional<std::pair<std::reference_wrapper<IRandomEventManager::Event const>, Vector3f>>
IRandomEventManager::GetPossibleRandomEvent()
{
    std::vector<std::pair<std::reference_wrapper<Event const>, Vector3f>> result;

    for (auto &&pair : this->m_events) {

        auto &&e = pair.second;
        if (CheckGlobalKeys(*e)) {

            std::vector<Vector3f> positions;

            // now look for valid spaces
            for (auto &&peer : NetManager()->GetPeers()) {
                auto &&zdo = peer->GetZDO();
                if (!zdo)
                    continue;

                if (
                        // Check biome first
                        (e->m_biome == avledet::util::Biome::None
                         || (std::to_underlying(GeoManager()->GetBiome(zdo->GetPosition()))
                             & std::to_underlying(e->m_biome))
                                    != std::to_underlying(avledet::util::Biome::None))
                        // check base next
                        && (!e->m_nearBaseOnly
                            || zdo->GetInt(avledet::util::hashes::ZDO::Player::BASE_VALUE) >= 3)
                        // check that player is not in dungeon
                        && (zdo->GetPosition().y < 3000.f)) {
                    //result.push_back({VUtils::Random::State().Range(0, )})
                    positions.push_back(zdo->GetPosition());
                }
            }

            if (!positions.empty())
                result.push_back({*e, positions[VUtils::Random::State().range(0, positions.size())]});
        }
    }

    if (!result.empty())
        return result[VUtils::Random::State().range(0, result.size())];

    return std::nullopt;
}

bool IRandomEventManager::CheckGlobalKeys(Event const &e)
{
    if (VH_SETTINGS.eventsRequireKeys) {
        for (auto &&key : e.m_presentGlobalKeys) {
            if (!ZoneManager()->GlobalKeys().contains(key))
                return false;
        }

        for (auto &&key : e.m_absentGlobalKeys) {
            if (ZoneManager()->GlobalKeys().contains(key))
                return false;
        }
    }

    return true;
}

void IRandomEventManager::Save(DataWriter &writer)
{
    writer.write(m_eventIntervalTimer);
    writer.write(m_activeEvent ? std::string_view(m_activeEvent->m_name) : "");
    //writer.write(m_activeEventTimer);
    writer.write(std::chrono::duration<float>(m_activeEventInitialDuration - m_activeEventRemaining).count());
    writer.write(m_activeEventPos);
}

void IRandomEventManager::Load(DataReader &reader, int version)
{
    m_eventIntervalTimer = reader.read<float>();

#if VH_IS_ON(VH_LEGACY_WORLD_LOADING)
    if (version >= 25) {
#endif// VH_LEGACY_WORLD_LOADING
        this->m_activeEvent          = GetEvent(reader.read<std::string_view>());
        this->m_activeEventRemaining = std::chrono::seconds((std::int64_t) reader.read<float>());
        this->m_activeEventPos       = reader.read<Vector3f>();
    }

    //VLOG(1) << "interval: " << this->m_eventIntervalTimer
    //<< ", event: " << (this->m_activeEvent ? this->m_activeEvent->m_name : "none")
    //<< ", pos: " << this->m_activeEventPos;
}

void IRandomEventManager::SendCurrentRandomEvent()
{
    if (m_activeEvent) {
        RouteManager()->InvokeAll(
                avledet::util::hashes::Routed::S2C_SetEvent, std::string_view(m_activeEvent->m_name),
                std::chrono::duration<float>(m_activeEventInitialDuration - m_activeEventRemaining).count(),
                m_activeEventPos);
    } else {
        RouteManager()->InvokeAll(avledet::util::hashes::Routed::S2C_SetEvent, std::string_view(""), 0.f,
                                  Vector3f::ZERO);
    }
}
