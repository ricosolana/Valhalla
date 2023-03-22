#include "EventManager.h"
#include "RouteManager.h"
#include "Hashes.h"
#include "NetManager.h"
#include "ZoneManager.h"
#include "GeoManager.h"

auto EVENT_MANAGER(std::make_unique<IEventManager>());
IEventManager *EventManager() {
	return EVENT_MANAGER.get();
}



void IEventManager::Update() {
	// update periodically

	// get random events that are able to be played
	
	//PERIODIC_NOW(SERVER_SETTINGS.eventInterval, {

		if (VUtils::Random::State().NextFloat() <= SERVER_SETTINGS.eventsChance) {

			
			if (auto opt = GetPossibleRandomEvent()) {
				auto&& e = opt.value().first;
				auto&& pos = opt.value().second;

				this->m_activeEvent = &e.get();
				this->m_activeEventPos = pos;

				// send event
				SendCurrentRandomEvent();
			}

		}

	//});

	
}

std::optional<std::pair<std::reference_wrapper<IEventManager::Event>, Vector3>> IEventManager::GetPossibleRandomEvent() {
	std::vector<std::pair<std::reference_wrapper<IEventManager::Event>, Vector3>> result;
	
	for (auto&& e : this->m_events) {

		if (e->m_random && HaveGlobalKeys(*e)) {

			std::vector<Vector3> positions;

			// now look for valid spaces
			for (auto&& peer : NetManager()->GetPeers()) {
				auto&& zdo = peer->GetZDO();
				if (!zdo) continue;

				if (
					// Check biome first
					(e->m_biome == Biome::None || (GeoManager()->GetBiome(zdo->Position()) & e->m_biome) == e->m_biome)
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

bool IEventManager::HaveGlobalKeys(Event& e) {
	for (auto&& key : e.m_requiredGlobalKeys) {
		if (!ZoneManager()->GlobalKeys().contains(key))
			return false;
	}

	for (auto&& key : e.m_notRequiredGlobalKeys) {
		if (ZoneManager()->GlobalKeys().contains(key))
			return false;
	}

	return true;
}

void IEventManager::Save(DataWriter& writer) {
	writer.Write(m_eventTimer);
	writer.Write(m_activeEvent ? m_activeEvent->m_name : "");
	writer.Write(m_activeEvent ? m_activeEventTimer : 0.f);
	writer.Write(m_activeEvent ? m_activeEventPos : Vector3::ZERO);
}

void IEventManager::Load(DataReader& reader, int version) {
	m_eventTimer = reader.Read<float>();
	if (version >= 25) {
		reader.Read<std::string>();
		reader.Read<float>();
		reader.Read<Vector3>();
	}
}

void IEventManager::SendCurrentRandomEvent() {
	if (m_activeEvent) {
		RouteManager()->InvokeAll(Hashes::Routed::SetEvent, 
			m_activeEvent->m_name,
			m_activeEventTimer,
			m_activeEventPos
		);
	}
	else {
		RouteManager()->InvokeAll(Hashes::Routed::SetEvent,
			"",
			0.f,
			Vector3::ZERO
		);
	}
}
