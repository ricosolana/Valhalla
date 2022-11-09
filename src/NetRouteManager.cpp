#include <robin_hood.h>

#include "NetRouteManager.h"
#include "NetManager.h"
#include "Method.h"
#include "MethodLua.h"
#include "ValhallaServer.h"
#include "ZoneSystem.h"
#include "NetSyncManager.h"
#include "NetObject.h"

namespace NetRouteManager {

	// Method hash, pair<callback, watcher>
	robin_hood::unordered_map<HASH_t, 
		std::pair<std::unique_ptr<IMethod<OWNER_t>>, std::unique_ptr<IMethod<OWNER_t>>>> m_methods;

	void _Register(HASH_t hash, IMethod<OWNER_t>* method) {
        _Register(hash, method, nullptr);
	}

	void _Register(HASH_t hash, IMethod<OWNER_t>* method, IMethod<OWNER_t>* watcher) {
		m_methods.insert({ hash, std::make_pair(std::unique_ptr<IMethod<OWNER_t>>(method), std::unique_ptr<IMethod<OWNER_t>>(watcher)) });
	}

	void _RouteRPC(const Data& data) {
		NetPackage pkg;
		data.Serialize(pkg);

		if (data.m_targetPeerID == EVERYBODY) {
			for (auto&& peer : NetManager::GetPeers()) {
				// send to everyone (except sender)
				if (data.m_senderPeerID != peer->m_uuid) {


                    // If the call was from this SERVER, no validation is necessary; data is valid
                    //if (data.m_senderPeerID != SERVER_ID) {
                    //    // Incur the watcher to validate the peers data
                    //    auto &&find = m_methods.find(data.m_methodHash);
                    //    if (find != m_methods.end() && find->second.second) {
                    //        find->second.second->Invoke(data.m_targetPeerID, data.m_parameters,
                    //                                    Utils::GetStableHashCode("RoutingFilter"), data.m_methodHash);
                    //    }
                    //}

                    peer->m_rpc->Invoke(Rpc_Hash::RoutedRPC, pkg);
				}
			}
		}
		else {
			auto peer = NetManager::GetPeer(data.m_targetPeerID);
			if (peer) {
				peer->m_rpc->Invoke(Rpc_Hash::RoutedRPC, pkg);
			}
		}
	}

	void _HandleRoutedRPC(Data data) {
		// If invocation was for RoutedRPC:
		if (data.m_targetSync) {

			/// Intended code implementation:
			//auto sync = NetSyncManager::Get(data.m_targetSync);
			//if (sync) {
			//	auto obj = NetScene::Get(data.m_targetSync);
			//	if (obj) {
			//		obj->HandleRoutedRPC(data);
			//	}
			//}
			


			//throw std::runtime_error("Not implemented");
			//NetSync NetSync = NetSyncMan.instance.GetNetSync(data.m_targetSync);
			//if (NetSync != null) {
			//	ZNetView znetView = ZNetScene.instance.FindInstance(NetSync);
			//	if (znetView != null) {
			//		znetView.HandleRoutedRPC(data);
			//	}
			//}
		}
		else {
			auto&& find = m_methods.find(data.m_methodHash);
			if (find != m_methods.end()) {
				find->second.first->Invoke(data.m_senderPeerID, std::move(data.m_parameters),
                                           Utils::GetStableHashCode("Routing") ^ data.m_methodHash);
			}
			else {
				LOG(INFO) << "Client tried invoking unknown RoutedRPC: " << data.m_methodHash;
			}
		}
	}

	void RPC_RoutedRPC(NetRpc* rpc, NetPackage pkg) {
		Data data(pkg);

        // todo
        // the pkg should have the sender as the peer id
        // check to see the id matches

        // do not trust the client to provide their correct id
        data.m_senderPeerID = NetManager::GetPeer(rpc)->m_uuid;

		// Server is the intended receiver (or EVERYONE)
		if (data.m_targetPeerID == SERVER_ID
			|| data.m_targetPeerID == EVERYBODY)
			_HandleRoutedRPC(data);

		// Server acts as a middleman
        // Server may validate packet before forwarding it
		if (data.m_targetPeerID != SERVER_ID)
			_RouteRPC(data);
	}

	void _Invoke(OWNER_t target, const NetID& targetNetSync, HASH_t hash, const NetPackage& pkg) {
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
		if (target == SERVER_ID
			|| target == EVERYBODY) {
			_HandleRoutedRPC(data);
		}

        // No validation is necessary as server is the sender due to this Invoke function
		if (target != SERVER_ID) {
			_RouteRPC(data);
		}
	}

	void OnNewPeer(NetPeer *peer) {
		peer->m_rpc->Register(Rpc_Hash::RoutedRPC, &RPC_RoutedRPC);
	}

	void OnPeerQuit(NetPeer *peer) {
		throw std::runtime_error("Not implemented");
	}
}