#include <robin_hood.h>

#include "NetRouteManager.h"
#include "NetManager.h"
#include "Method.h"
#include "ValhallaServer.h"
#include "ZoneSystem.h"
#include "ZDOManager.h"
#include "NetObject.h"

auto MANAGER_ROUTE(std::make_unique<IManagerRoute>());
IManagerRoute* RouteManager() {
	return MANAGER_ROUTE.get();
}

void IManagerRoute::RouteRPC(const Data& data) {
	NetPackage pkg;
	data.Serialize(pkg);

	if (data.m_targetPeerID == EVERYBODY) {
		for (auto&& peer : NetManager::GetPeers()) {
			// send to everyone (except sender)
			if (data.m_senderPeerID != peer->m_uuid) {
                peer->m_rpc->Invoke(NetHashes::Rpc::RoutedRPC, pkg);
			}
		}
	}
	else {
		auto peer = NetManager::GetPeer(data.m_targetPeerID);
		if (peer) {
			peer->m_rpc->Invoke(NetHashes::Rpc::RoutedRPC, pkg);
		}
	}
}

void IManagerRoute::HandleRoutedRPC(Data data) {
	// If invocation was for RoutedRPC:
	if (!data.m_targetSync) {
		auto&& find = m_methods.find(data.m_methodHash);
		if (find != m_methods.end()) {
			find->second->Invoke(data.m_senderPeerID, std::move(data.m_parameters));
		}
		else {
			LOG(INFO) << "Client tried invoking unknown RoutedRPC: " << data.m_methodHash;
		}
	}
}

void IManagerRoute::Invoke(OWNER_t target, const NetID& targetNetSync, HASH_t hash, const NetPackage& pkg) {
	static OWNER_t m_rpcMsgID = 1;

	Data data;
	data.m_msgID = SERVER_ID + m_rpcMsgID++;
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
		HandleRoutedRPC(data);
	}

    // No validation is necessary as server is the sender due to this Invoke function
	if (target != SERVER_ID) {
		RouteRPC(data);
	}
}

void IManagerRoute::OnNewPeer(NetPeer *peer) {
	peer->m_rpc->Register(NetHashes::Rpc::RoutedRPC, [this](NetRpc* rpc, NetPackage pkg) {
		Data data(pkg);

		// todo
		// the pkg should have the sender as the peer id
		// check to see the id matches

		// do not trust the client to provide their correct id
		data.m_senderPeerID = NetManager::GetPeer(rpc)->m_uuid;

		// Server is the intended receiver (or EVERYONE)
		if (data.m_targetPeerID == SERVER_ID
			|| data.m_targetPeerID == EVERYBODY)
			HandleRoutedRPC(data);

		// Server acts as a middleman
		// Server may validate packet before forwarding it
		if (data.m_targetPeerID != SERVER_ID)
			RouteRPC(data);
	});
}
