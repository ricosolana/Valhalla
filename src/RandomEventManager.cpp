#include "RandomEventManager.h"
#include "RouteManager.h"
#include "Hashes.h"
#include "NetManager.h"
#include "ZoneManager.h"
#include "GeoManager.h"
#include "VUtilsResource.h"
#include "ZDOManager.h"
#include "Prefab.h"
#include "DiscordManager.h"

auto RANDOM_EVENT_MANAGER = std::make_unique<IRandomEventManager>();
IRandomEventManager* RandomEventManager() {
	return RANDOM_EVENT_MANAGER.get();
}

void IRandomEventManager::Init() {
	// interval: 46
	// chance: 20
	// range: 96

	LOG_INFO(LOGGER, "Initializing EventManager");

	{
		// load Foliage:
		auto opt = VUtils::Resource::ReadFile<BYTES_t>("randomEvents.pkg");
		if (!opt)
			throw std::runtime_error("randomEvents.pkg missing");

		DataReader pkg(*opt);

		pkg.Read<std::string_view>(); // comment
		auto ver = pkg.Read<std::string_view>();
		if (ver != VConstants::GAME)
			LOG_WARNING(LOGGER, "randomEvents.pkg uses different game version than server ({})", ver);

		auto count = pkg.Read<int32_t>();
		for (int i = 0; i < count; i++) {
			auto e = std::make_unique<Event>();

			e->m_name = pkg.Read<std::string>();
			e->m_duration = duration_cast<nanoseconds>(seconds((int64_t)pkg.Read<float>()));
			e->m_nearBaseOnly = pkg.Read<bool>();
			e->m_pauseIfNoPlayerInArea = pkg.Read<bool>();
			e->m_biome = (Biome)pkg.Read<int32_t>();

			e->m_presentGlobalKeys = pkg.Read<decltype(Event::m_presentGlobalKeys)>();
			e->m_absentGlobalKeys = pkg.Read<decltype(Event::m_absentGlobalKeys)>();

			auto&& sv = e->m_name; // Make a temp key because assign eval order is not guaranteed
			m_events[sv] = std::move(e);
		}

		LOG_INFO(LOGGER, "Loaded {} random events", count);
	}
}

void IRandomEventManager::Update() {
	ZoneScoped;

	// update event timer if an event is active
	if (m_activeEvent) {
		// TODO implement!

		assert(false);
		// Update the timer of the current event
		//if (!m_activeEvent->m_pauseIfNoPlayerInArea
		//	|| ZDOManager()->AnyZDO(this->m_activeEventPos, VH_SETTINGS.eventsRadius, Hashes::Object::Player, Prefab::Flag::NONE, Prefab::Flag::NONE))
		//	//m_activeEventTimer += Valhalla()->Delta();
		//	m_activeEventRemaining -= Valhalla()->DeltaNanos();

		//if (m_activeEventTimer > this->m_activeEvent->m_duration) {
		if (m_activeEventRemaining <= 0ns) {
			VH_DISPATCH_WEBHOOK("Random event stopped: `" + this->m_activeEvent->m_name + "`");

			m_activeEvent = nullptr;
			m_activeEventPos = Vector3f::Zero();
		}
	}
	else if (VH_SETTINGS.eventsInterval > 0s) {
		m_eventIntervalTimer += Valhalla()->Delta();

		// try to set a new current event
		if (m_eventIntervalTimer > VH_SETTINGS.eventsInterval.count()) {
			m_eventIntervalTimer = 0;
			if (VUtils::Random::State().NextFloat() <= VH_SETTINGS.eventsChance) {

				if (auto opt = GetPossibleRandomEvent()) {
					auto&& e = opt.value().first;
					auto&& pos = opt.value().second;

					SetCurrentRandomEvent(e.get(), pos, e.get().m_duration);

					/*
					this->m_activeEvent = &e.get();
					this->m_activeEventPos = pos;
					this->m_activeEventRemaining = this->m_activeEvent->m_duration;

					LOG_INFO(LOGGER, "Set current random event: {}", e.get().m_name);

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

void IRandomEventManager::SetCurrentRandomEvent(const Event& e, Vector3f pos, nanoseconds nanos) {
	this->m_activeEvent = &e;
	this->m_activeEventPos = pos;
	this->m_activeEventRemaining = nanos;
	this->m_activeEventInitialDuration = nanos;

	LOG_INFO(LOGGER, "Set current random event: {}", e.m_name);
	VH_DISPATCH_WEBHOOK("Random event started in world `" + e.m_name + "`");
}

std::optional<std::pair<std::reference_wrapper<const IRandomEventManager::Event>, Vector3f>> IRandomEventManager::GetPossibleRandomEvent() {
	std::vector<std::pair<std::reference_wrapper<const Event>, Vector3f>> result;

	for (auto&& pair : this->m_events) {

		auto&& e = pair.second;
		if (CheckGlobalKeys(*e)) {

			std::vector<Vector3f> positions;

			// now look for valid spaces
			for (auto&& peer : NetManager()->GetPeers()) {
				auto&& zdo = peer->GetZDO();
				if (!zdo) continue;

				if (
					// Check biome first
					(e->m_biome == Biome::None || (std::to_underlying(GeoManager()->GetBiome(zdo->GetPosition())) & std::to_underlying(e->m_biome)) != std::to_underlying(Biome::None))
					// check base next
					&& (!e->m_nearBaseOnly || zdo->GetInt(Hashes::ZDO::Player::BASE_VALUE) >= 3)
					// check that player is not in dungeon
					&& (zdo->GetPosition().y < 3000.f))
				{
					//result.push_back({VUtils::Random::State().Range(0, )})
					positions.push_back(zdo->GetPosition());
				}
			}

			if (!positions.empty())
				result.push_back({ *e, positions[VUtils::Random::State().Range(0, positions.size())] });

		}
	}

	if (!result.empty())
		return result[VUtils::Random::State().Range(0, result.size())];

	return std::nullopt;
}

bool IRandomEventManager::CheckGlobalKeys(const Event& e) {
	if (VH_SETTINGS.eventsRequireKeys) {
		for (auto&& key : e.m_presentGlobalKeys) {
			if (!ZoneManager()->GlobalKeys().contains(key))
				return false;
		}

		for (auto&& key : e.m_absentGlobalKeys) {
			if (ZoneManager()->GlobalKeys().contains(key))
				return false;
		}
	}

	return true;
}

void IRandomEventManager::Save(DataWriter& writer) {
	writer.Write(m_eventIntervalTimer);
	writer.Write(m_activeEvent ? std::string_view(m_activeEvent->m_name) : "");
	//writer.Write(m_activeEventTimer);
	writer.Write((float)duration_cast<seconds>(m_activeEventInitialDuration - m_activeEventRemaining).count());
	writer.Write(m_activeEventPos);
}

void IRandomEventManager::Load(DataReader& reader, int version) {
	m_eventIntervalTimer = reader.Read<float>();
	if (version >= 25) {
		this->m_activeEvent = GetEvent(reader.Read<std::string_view>());
		this->m_activeEventRemaining = seconds((int64_t)reader.Read<float>());
		this->m_activeEventPos = reader.Read<Vector3f>();
	}

	//VLOG(1) << "interval: " << this->m_eventIntervalTimer
		//<< ", event: " << (this->m_activeEvent ? this->m_activeEvent->m_name : "none")
		//<< ", pos: " << this->m_activeEventPos;
}

void IRandomEventManager::SendCurrentRandomEvent() {
	if (m_activeEvent) {
		RouteManager()->InvokeAll(Hashes::Routed::S2C_SetEvent,
			std::string_view(m_activeEvent->m_name),
			(float)duration_cast<seconds>(m_activeEventInitialDuration - m_activeEventRemaining).count(),
			m_activeEventPos
		);
	}
	else {
		RouteManager()->InvokeAll(Hashes::Routed::S2C_SetEvent,
			"",
			0.f,
			Vector3f::Zero()
		);
	}
}
