#include <robin_hood.h>

#include "NetRouteManager.h"
#include "Method.h"
#include "ValhallaServer.h"
#include "ZoneSystem.h"
#include "NetSyncManager.h"
#include "NetObject.h"

namespace NetRpcManager {
	robin_hood::unordered_map<HASH_t, IMethod<UUID_t>*> m_methods;

	UUID_t _ServerID() {
		return Valhalla()->Uuid();
	}

	void _Register(HASH_t hash, IMethod<UUID_t>* method) {
		m_methods.insert({ hash, method });
	}

	void _RouteRPC(Data data) {
		//auto pkg(PKG());
		NetPackage pkg;
		data.Serialize(pkg);

		if (data.m_targetPeerID == EVERYBODY) {
			for (auto&& peer : NetManager::GetPeers()) {
				// send to everyone (except sender)
				if (data.m_senderPeerID != peer->m_uuid)
					peer->m_rpc->Invoke(Rpc_Hash::RoutedRPC, std::move(pkg));
			}
		}
		else {
			auto peer = NetManager::GetPeer(data.m_targetPeerID);
			if (peer) {
				peer->m_rpc->Invoke(Rpc_Hash::RoutedRPC, std::move(pkg));
			}
		}
	}

	// Token: 0x06000AA3 RID: 2723 RVA: 0x00050474 File Offset: 0x0004E674
	void _HandleRoutedRPC(Data data) {
		// If invocation was for RoutedRPC:
		if (data.m_targetNetSync) {

			/// Intended code implementation:
			//auto sync = NetSyncManager::Get(data.m_targetNetSync);
			//if (sync) {
			//	auto obj = NetScene::Get(data.m_targetNetSync);
			//	if (obj) {
			//		obj->HandleRoutedRPC(data);
			//	}
			//}
			


			//throw std::runtime_error("Not implemented");
			//NetSync NetSync = NetSyncMan.instance.GetNetSync(data.m_targetNetSync);
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
				find->second->Invoke(data.m_targetPeerID, data.m_parameters, NetInvoke::ROUTE, data.m_methodHash);
			}
			else {
				LOG(INFO) << "Client tried invoking unknown RoutedRPC: " << data.m_methodHash;
			}
		}
	}

	void RPC_RoutedRPC(NetRpc* rpc, NetPackage pkg) {
		Data data(pkg);

		// Server is the intended receiver
		if (data.m_targetPeerID == _ServerID()
			|| data.m_targetPeerID == EVERYBODY)
			_HandleRoutedRPC(data);

		// Server acts as a middleman
		if (data.m_targetPeerID != _ServerID())
			_RouteRPC(data);
	}

	void _Invoke(UUID_t target, const NetID& targetNetSync, HASH_t hash, NetPackage &&pkg) {
		static UUID_t m_rpcMsgID = 1;
		static auto SERVER_ID(_ServerID());

		Data data;
		data.m_msgID = SERVER_ID + m_rpcMsgID++;
		data.m_senderPeerID = SERVER_ID;
		data.m_targetPeerID = target;
		data.m_targetNetSync = targetNetSync;
		data.m_methodHash = hash;
		data.m_parameters = std::move(pkg);

		

		data.m_parameters.m_stream.SetPos(0);

		// Handle message
		if (target == SERVER_ID
			|| target == EVERYBODY) {
			_HandleRoutedRPC(data);
		}

		// Pass the message along to the recipient
		if (target != SERVER_ID) {
			_RouteRPC(data);
		}
	}

	void OnNewPeer(NetPeer::Ptr peer) {
		peer->m_rpc->Register(Rpc_Hash::RoutedRPC, &RPC_RoutedRPC);
	}

	void OnPeerQuit(NetPeer::Ptr peer) {
		throw std::runtime_error("Not implemented");
	}
}