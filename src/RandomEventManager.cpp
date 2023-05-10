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

auto RANDOM_EVENT_MANAGER(std::make_unique<IRandomEventManager>());
IRandomEventManager *RandomEventManager() {
	return RANDOM_EVENT_MANAGER.get();
}

void IRandomEventManager::Init() {
	// interval: 46
	// chance: 20
	// range: 96

	//LOG(INFO) << "Initializing EventManager";

	{
		// load Foliage:
		auto opt = VUtils::Resource::ReadFile<BYTES_t>("randomEvents.pkg");
		if (!opt)
			throw std::runtime_error("randomEvents.pkg missing");

		DataReader pkg(*opt);

		pkg.Read<std::string_view>(); // comment
		auto ver = pkg.Read<std::string_view>();
		if (ver != VConstants::GAME)
			//LOG(WARNING) << "randomEvents.pkg uses different game version than server (" << ver << ")";

		auto count = pkg.Read<int32_t>();
		for (int i = 0; i < count; i++) {
			auto e = std::make_unique<Event>();

			e->m_name = pkg.Read<std::string>();
			e->m_duration = pkg.Read<float>();
			e->m_nearBaseOnly = pkg.Read<bool>();
			e->m_pauseIfNoPlayerInArea = pkg.Read<bool>();
			e->m_biome = (Biome) pkg.Read<int32_t>();
			
			e->m_presentGlobalKeys = pkg.Read<UNORDERED_SET_t<std::string>>();
			e->m_absentGlobalKeys = pkg.Read<UNORDERED_SET_t<std::string>>();

			m_events.insert({ VUtils::String::GetStableHashCode(e->m_name), std::move(e) });
		}

		//LOG(INFO) << "Loaded " << count << " random events";
	}
}

void IRandomEventManager::Update() {
	ZoneScoped;

	m_eventIntervalTimer += Valhalla()->Delta();

	// update event timer if an event is active
	if (m_activeEvent) {
		// Update the timer of the current event
		if (!m_activeEvent->m_pauseIfNoPlayerInArea
			|| ZDOManager()->AnyZDO(this->m_activeEventPos, VH_SETTINGS.eventsRadius, 0, Prefab::Flag::PLAYER, Prefab::Flag::NONE))
			m_activeEventTimer += Valhalla()->Delta();

		if (m_activeEventTimer > this->m_activeEvent->m_duration) {
			VH_DISPATCH_WEBHOOK("Random event stopped: `" + this->m_activeEvent->m_name + "`");

			m_activeEvent = nullptr;
			m_activeEventPos = Vector3f::Zero();
		}
	}
	else if (VH_SETTINGS.eventsInterval > 0s) {
		// try to set a new current event
		if (m_eventIntervalTimer > VH_SETTINGS.eventsInterval.count()) {
			m_eventIntervalTimer = 0;
			if (VUtils::Random::State().NextFloat() <= VH_SETTINGS.eventsChance) {

				if (auto opt = GetPossibleRandomEvent()) {
					auto&& e = opt.value().first;
					auto&& pos = opt.value().second;

					this->m_activeEvent = &e.get();
					this->m_activeEventPos = pos;
					this->m_activeEventTimer = 0;

					//LOG(INFO) << "Set current random event: " << e.get().m_name;

					VH_DISPATCH_WEBHOOK("Random event started in world `" + this->m_activeEvent->m_name + "`");

					// send event
					//SendCurrentRandomEvent();
				}
			}
		}
	}

	PERIODIC_NOW(1s, { SendCurrentRandomEvent(); });
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
					(e->m_biome == Biome::None || (GeoManager()->GetBiome(zdo->Position()) & e->m_biome) != Biome::None)
					// check base next
					&& (!e->m_nearBaseOnly || zdo->GetInt(Hashes::ZDO::Player::BASE_VALUE) >= 3)
					// check that player is not in dungeon
					&& (zdo->Position().y < 3000.f)) 
				{
					//result.push_back({VUtils::Random::State().Range(0, )})
					positions.push_back(zdo->Position());
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
	writer.Write(m_activeEventTimer);
	writer.Write(m_activeEventPos);
}

void IRandomEventManager::Load(DataReader& reader, int version) {
	m_eventIntervalTimer = reader.Read<float>();
	if (version >= 25) {
		this->m_activeEvent = GetEvent(reader.Read<std::string>());
		this->m_activeEventTimer = reader.Read<float>();
		this->m_activeEventPos = reader.Read<Vector3f>();
	}

	VLOG(1) << "interval: " << this->m_eventIntervalTimer
		<< ", event: " << (this->m_activeEvent ? this->m_activeEvent->m_name : "none")
		<< ", pos: " << this->m_activeEventPos;
}

void IRandomEventManager::SendCurrentRandomEvent() {
	if (m_activeEvent) {
		RouteManager()->InvokeAll(Hashes::Routed::S2C_SetEvent, 
			std::string_view(m_activeEvent->m_name),
			m_activeEventTimer,
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
