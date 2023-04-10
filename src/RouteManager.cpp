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
		auto params = reader.SubRead();

		/*
		* Rpc and multi-execution dilemna
		*	Should I let multiple handlers be able to call RoutedRpc methods?
		*	What about packets coming in that do not refer to any currently register Rpc?
		*		Should they still dispatch the Lua handlers?
		*	Similarly, what about missing RoutedRpc handlers?
		*/

		if (SERVER_SETTINGS.worldMode == WorldMode::PLAYBACK) {
			// Only allow "read-only" RoutedRpcs (where client is requesting data and not changing anything)
			// Client is an observer in this scenario
			if (!(hash == Hashes::Routed::C2S_RequestIcon
				|| hash == Hashes::Routed::C2S_RequestZDO
				|| hash == Hashes::Routed::ChatMessage
				|| hash == Hashes::Routed::C2S_RequestIcon)) 
			{
				// Only replay sockets allowed to do anything
				if (!std::dynamic_pointer_cast<ReplaySocket>(peer->m_socket))
					return;
			}
		}

		if (target == EVERYBODY) {
			// Confirmed: targetZDO CAN have a value when globally routed
			//assert(!targetZDO && "might have to change the logic; routed zdos might be globally invoked...");
			if (!ModManager()->CallEvent(IModManager::Events::RouteInAll ^ hash, peer, targetZDO, params))
				return;

			//if (SERVER_SETTINGS.worldMode == WorldMode::PLAYBACK
			//	&& !std::dynamic_pointer_cast<ReplaySocket>(peer->m_socket))
			//	return;

			if (SERVER_SETTINGS.worldMode == WorldMode::CAPTURE) {
				// capture changes
				peer->m_recordPacket = true;
			}

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
			if (target != SERVER_ID) {
				if (auto other = NetManager()->GetPeer(target)) {
					if (!ModManager()->CallEvent(IModManager::Events::Routed ^ hash, peer, other, targetZDO, params))
						return;

					other->Invoke(Hashes::Rpc::RoutedRPC, bytes);
				}
			}
			else {
				if (!targetZDO) {
					if (SERVER_SETTINGS.worldMode == WorldMode::CAPTURE) {

						// If RoutedRpc is a modification packet then capture
						// This isnt all that too significant, but it slightly reduces sizes
						//if (!(hash == Hashes::Routed::C2S_RequestZDO
						//	//|| hash == Hashes::Routed::Pong
						//	|| hash == Hashes::Routed::C2S_RequestIcon)) 
						//{
						//	// capture changes
						//	peer->m_recordPacket = true;
						//}
						peer->m_recordPacket = true;
					}

					auto&& find = m_methods.find(hash);
					if (find != m_methods.end()) {
						find->second->Invoke(peer, params);
					}
				} //else ... // netview is not currently supported
			}
		}
	});
}
