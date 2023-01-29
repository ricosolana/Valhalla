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
	NetPackage pkg;
	data.Serialize(pkg);

	if (data.m_targetPeerID == EVERYBODY) {
		auto&& peers = NetManager()->GetPeers();
		for (auto&& pair : peers) {
			auto&& peer = pair.second;
			// send to everyone (except sender)
			if (data.m_senderPeerID != peer->m_uuid) {
                peer->Invoke(Hashes::Rpc::RoutedRPC, pkg);
			}
		}
	}
	else {
		auto peer = NetManager()->GetPeer(data.m_targetPeerID);
		if (peer) {
			peer->Invoke(Hashes::Rpc::RoutedRPC, pkg);
		}
	}
}

void IRouteManager::HandleRoutedRPC(Peer* sender, Data data) {
	// If invocation was for RoutedRPC:
	if (!data.m_targetSync) {
		auto&& find = m_methods.find(data.m_methodHash);
		if (find != m_methods.end()) {
			find->second->Invoke(sender, std::move(data.m_parameters));
		}
		else {
			// Due to the way Valhiem manages it RoutedRPC's, 
			//	the server will receive its own self-invoked RoutedRPC calls
			//LOG(INFO) << "Client tried invoking unknown RoutedRPC: " << data.m_methodHash;
		}
	}
}

void IRouteManager::Invoke(OWNER_t target, const NetID& targetNetSync, HASH_t hash, const NetPackage& pkg) {
	static OWNER_t m_rpcMsgID = 1;

	Data data;
	//data.m_msgID = SERVER_ID + m_rpcMsgID++;
	data.m_msgID = m_rpcMsgID++;
	data.m_senderPeerID = SERVER_ID;
	data.m_targetPeerID = target;
	data.m_targetSync = targetNetSync;
	data.m_methodHash = hash;
	data.m_parameters = pkg;

	data.m_parameters.m_stream.SetPos(0);

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

void IRouteManager::OnNewPeer(Peer *peer) {
	peer->Register(Hashes::Rpc::RoutedRPC, [this](Peer* peer, NetPackage pkg) {
		Data data(pkg);

		// TODO constraint peer sender
		data.m_senderPeerID = peer->m_uuid;

		// Server is the intended receiver (or EVERYONE)
		if (data.m_targetPeerID == SERVER_ID
			|| data.m_targetPeerID == EVERYBODY)
			HandleRoutedRPC(peer, data);

		// Server acts as a middleman
		// Server may validate packet before forwarding it
		if (data.m_targetPeerID != SERVER_ID)
			RouteRPC(data);
	});
}
