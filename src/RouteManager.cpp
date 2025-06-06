#include "RouteManager.h"
#include "DataStream.h"
#include "NetManager.h"
#include "Method.h"
#include "ValhallaServer.h"
#include "ZoneManager.h"
#include "ZDOManager.h"
#include "Hashes.h"
#include "Peer.h"

auto ROUTE_MANAGER = std::make_unique<IRouteManager>(); // TODO stop constructing in global
IRouteManager* RouteManager() {
	return ROUTE_MANAGER.get();
}



void IRouteManager::OnNewPeer(Peer &peer) {
	peer.Register(avledet::util::hashes::Rpc::RoutedRPC, [this](Peer* peer, DataReader reader) {
		if (peer->IsGated())
			return;

		reader.read<std::int64_t>(); // skip msgid
		/*DataWriter(avledet::util::ByteView(reader.data(), reader.size()), reader.get_pos()).Write(peer->m_uuid);*/ 
		reader.read<avledet::util::UserID>(); // skip sender
		auto target = reader.read<avledet::util::UserID>();
		auto targetZDO = reader.read<ZDOID>();
		auto hash = reader.read<avledet::util::Hash>();
		auto params = DataReader(reader.read<std::vector<char>>());

		/*
		* Rpc and multi-execution dilemna
		*	Should I let multiple handlers be able to call RoutedRpc methods?
		*	What about packets coming in that do not refer to any currently register Rpc?
		*		Should they still dispatch the Lua handlers?
		*	Similarly, what about missing RoutedRpc handlers?
		*/

		if (target == EVERYBODY) {
			// Confirmed: targetZDO CAN have a value when globally routed
			if (!VH_DISPATCH_MOD_EVENT(IModManager::Events::RouteInAll ^ hash, peer, targetZDO, params))
				return;

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
				if (peer->GetUserID() != other->GetUserID()) {
					other->Invoke(avledet::util::hashes::Rpc::RoutedRPC, (std::int64_t)0, peer->GetUserID(), target, targetZDO, hash, params);
				}
			}
		}
		else {
			if (target != VH_ID) {
				if (auto other = NetManager()->GetPeerByUserID(target)) {
					if (!VH_DISPATCH_MOD_EVENT(IModManager::Events::Routed ^ hash, peer, reader))
						return;

					//other->Invoke(avledet::util::hashes::Rpc::RoutedRPC, reader);
					other->Invoke(avledet::util::hashes::Rpc::RoutedRPC, (std::int64_t)0, peer->GetUserID(), target, targetZDO, hash, params);
				}
			}
			else {
				if (!targetZDO) {
					auto&& find = m_methods.find(hash);
					if (find != m_methods.end()) {
						//find->second->Invoke(peer, reader.read<DataReader>());
						find->second->Invoke(peer, params);
					}
				} //else ... // netview is not currently supported
			}
		}
	});
}
