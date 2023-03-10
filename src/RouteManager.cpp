#include <robin_hood.h>

#include "RouteManager.h"
#include "NetManager.h"
#include "Method.h"
#include "ValhallaServer.h"
#include "ZoneManager.h"
#include "ZDOManager.h"
#include "Hashes.h"

auto ROUTE_MANAGER(std::make_unique<IRouteManager>()); // TODO stop constructing in global
IRouteManager* RouteManager() {
	return ROUTE_MANAGER.get();
}



void IRouteManager::RouteRPC(const Data& data) {
	static BYTES_t bytes; bytes.clear();
	data.Serialize(DataWriter(bytes));

	if (data.m_target == EVERYBODY) {
		auto&& peers = NetManager()->GetPeers();
		for (auto&& peer : peers) {
			// send to everyone (except sender)
			if (data.m_sender != peer->m_uuid) {

				// TODO try to unpack route args, regardless of zdo or peer target
				//	(because param passing is same/trivial between both, fortunately)
				//if (data.m_senderPeerID != SERVER_ID)
					//ModManager()->CallEvent(EVENT_HASH_Routed, data);

                peer->Invoke(Hashes::Rpc::RoutedRPC, bytes);
			}
		}
	}
	else {
		auto peer = NetManager()->GetPeer(data.m_target);
		if (peer) {
			peer->Invoke(Hashes::Rpc::RoutedRPC, bytes);
		}
	}
}

void IRouteManager::HandleRoutedRPC(Peer* sender, Data data) {
	// If invocation was for RoutedRPC:
	if (!data.m_targetZDO) {
		auto&& find = m_methods.find(data.m_method);
		if (find != m_methods.end()) {
			DataReader reader(data.m_params);
			find->second->Invoke(sender, reader);
		}
		else {
			// Due to the way Valheim manages it RoutedRPC's, 
			//	the server will receive its own self-invoked RoutedRPC calls
			//LOG(INFO) << "Client tried invoking unknown RoutedRPC: " << data.m_methodHash;
		}
	}
}

void IRouteManager::InvokeImpl(OWNER_t target, const ZDOID& targetNetSync, HASH_t hash, BYTES_t params) {
	Data data;

	data.m_sender = SERVER_ID;
	data.m_target = target;
	data.m_targetZDO = targetNetSync;
	data.m_method = hash;
	data.m_params = std::move(params);

	// Message destined to server or everyone
	// When the server invokes an EVERYBODY call, the server still calls its own method
	if (target == SERVER_ID
		|| target == EVERYBODY) {
		HandleRoutedRPC(nullptr, data);
	}

    // No validation is necessary as server is the sender due to this Invoke function
	if (target != SERVER_ID) {
		RouteRPC(data);
	}
}

void IRouteManager::OnNewPeer(Peer &peer) {
	peer.Register(Hashes::Rpc::RoutedRPC, [this](Peer* peer, BYTES_t bytes) {
		Data data = Data(DataReader(bytes));

		// TODO constraint peer sender
		data.m_sender = peer->m_uuid;

		// Server is the intended receiver (or EVERYONE)
		if (data.m_target == SERVER_ID
			|| data.m_target == EVERYBODY)
			HandleRoutedRPC(peer, data);

		// Server acts as a middleman
		// Server may validate packet before forwarding it
		if (data.m_target != SERVER_ID)
			RouteRPC(data);
	});
}
