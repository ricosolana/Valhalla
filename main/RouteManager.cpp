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
	peer.Register(Hashes::Rpc::RoutedRPC, [this](Peer* peer, DataReader reader) {
		reader.Read<int64_t>(); // skip msgid
		DataWriter(BYTE_VIEW_t(reader.data(), reader.size()), reader.Position()).Write(peer->m_uuid); reader.Read<OWNER_t>();
		auto target = reader.Read<OWNER_t>();
		auto targetZDO = reader.Read<ZDOID>();
		auto hash = reader.Read<HASH_t>();
		auto params = reader.Read<DataReader>();

		/*
		* Rpc and multi-execution dilemna
		*	Should I let multiple handlers be able to call RoutedRpc methods?
		*	What about packets coming in that do not refer to any currently register Rpc?
		*		Should they still dispatch the Lua handlers?
		*	Similarly, what about missing RoutedRpc handlers?
		*/

		if (target == EVERYBODY) {
			// Confirmed: targetZDO CAN have a value when globally routed
			// 'EVERYBODY' also targets the server
			if (!targetZDO) {
				auto&& find = m_methods.find(hash);
				if (find != m_methods.end()) {
					find->second->Invoke(peer, params);
				}
			} //else ... // netview is not currently supported

			auto&& peers = NetManager()->GetPeers();
			for (auto&& other : peers) {
				// Ignore the src peer
				if (peer->m_uuid != other->m_uuid) {
					other->Invoke(Hashes::Rpc::RoutedRPC, 0LL, peer->m_uuid, target, targetZDO, hash, params);
				}
			}
		}
		else {
			if (target != VH_ID) {
				if (auto other = NetManager()->GetPeerByUUID(target)) {
					//other->Invoke(Hashes::Rpc::RoutedRPC, reader);
					other->Invoke(Hashes::Rpc::RoutedRPC, 0LL, peer->m_uuid, target, targetZDO, hash, params);
				}
			}
			else {
				if (!targetZDO) {
					auto&& find = m_methods.find(hash);
					if (find != m_methods.end()) {
						//find->second->Invoke(peer, reader.Read<DataReader>());
						find->second->Invoke(peer, params);
					}
				} //else ... // netview is not currently supported
			}
		}
	});
}
