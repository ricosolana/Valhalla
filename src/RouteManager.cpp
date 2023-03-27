#include <robin_hood.h>

#include "RouteManager.h"
#include "NetManager.h"
#include "Method.h"
#include "ValhallaServer.h"
#include "ZoneManager.h"
#include "ZDOManager.h"
#include "Hashes.h"
#include "Peer.h"

auto ROUTE_MANAGER(std::make_unique<IRouteManager>()); // TODO stop constructing in global
IRouteManager* RouteManager() {
	return ROUTE_MANAGER.get();
}



void IRouteManager::OnNewPeer(Peer &peer) {
	peer.Register(Hashes::Rpc::RoutedRPC, [this](Peer* peer, BYTES_t bytes) {
		DataReader reader(bytes);
		reader.Read<int64_t>(); // skip msgid
		reader.ToWriter().Write(peer->m_uuid); reader.Read<OWNER_t>(); // force sender in place
		auto target = reader.Read<OWNER_t>();
		auto targetZDO = reader.Read<ZDOID>();
		auto hash = reader.Read<HASH_t>();

		if (target == EVERYBODY) {
			if (!ModManager()->CallEvent(IModManager::Events::RouteInAll ^ hash, bytes))
				return;

			auto&& peers = NetManager()->GetPeers();
			for (auto&& other : peers) {
				// Unlikely
				if (peer->m_uuid != other->m_uuid) {
					other->Invoke(Hashes::Rpc::RoutedRPC, bytes);
				}
			}
		}
		else {
			if (target != SERVER_ID) {
				if (!ModManager()->CallEvent(IModManager::Events::Routed ^ hash, bytes))
					return;

				if (auto peer = NetManager()->GetPeer(target))
					peer->Invoke(Hashes::Rpc::RoutedRPC, bytes);
			}
			else {
				if (!targetZDO) {
					auto&& find = m_methods.find(hash);
					if (find != m_methods.end()) {
						DataReader params(reader.SubRead());
						find->second->Invoke(peer, params);
					}
				} //else ... // netview is not currently supported
			}
		}
	});
}
