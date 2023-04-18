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
		//reader.ToWriter().Write(peer->m_uuid); reader.Read<OWNER_t>(); // force sender in place
		reader.Read<OWNER_t>();
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

		
		if (VH_SETTINGS.worldMode == WorldMode::PLAYBACK) {
			// If this is a real peer, allow ONLY read-only actions
			if (!(hash == Hashes::Routed::C2S_RequestIcon
				|| hash == Hashes::Routed::C2S_RequestZDO
				|| hash == Hashes::Routed::ChatMessage
				|| hash == Hashes::Routed::C2S_RequestIcon
				|| hash == Hashes::View::Talker::Chat))
			{
				// Only replay sockets allowed to do anything
				if (!std::dynamic_pointer_cast<ReplaySocket>(peer->m_socket)) {
					return;
				}
			}
		}

		if (target == EVERYBODY) {
			// Confirmed: targetZDO CAN have a value when globally routed
			//assert(!targetZDO && "might have to change the logic; routed zdos might be globally invoked...");
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
				if (peer->m_uuid != other->m_uuid) {
					other->Invoke(Hashes::Rpc::RoutedRPC, bytes);
				}
			}
		}
		else {
			if (target != VH_ID) {
				if (auto other = NetManager()->GetPeer(target)) {
					if (!VH_DISPATCH_MOD_EVENT(IModManager::Events::Routed ^ hash, peer, other, targetZDO, params))
						return;

					other->Invoke(Hashes::Rpc::RoutedRPC, bytes);
				}
			}
			else {
				if (!targetZDO) {
					auto&& find = m_methods.find(hash);
					if (find != m_methods.end()) {
						find->second->Invoke(peer, params);
					}
				} //else ... // netview is not currently supported
			}
		}
	});
}
