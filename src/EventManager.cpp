#include "EventManager.h"
#include "RouteManager.h"
#include "Hashes.h"

void IEventManager::Update() {
    PERIODIC_NOW(2s, {

    });
}

void IEventManager::Save(DataWriter& writer) {
	writer.Write(m_eventTimer);
	writer.Write(m_randomEvent ? m_randomEvent->m_name : "");
	writer.Write(m_randomEvent ? m_activeEventTimer : 0.f);
	writer.Write(m_randomEvent ? m_activeEventPos : Vector3::ZERO);
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
	if (m_randomEvent) {
		RouteManager()->InvokeAll(Hashes::Routed::SetEvent, 
			m_randomEvent->m_name,
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
