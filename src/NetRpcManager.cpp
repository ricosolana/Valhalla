#include "NetRpcManager.h"
#include "Method.h"
#include "ValhallaServer.h"
#include "ZoneSystem.h"

namespace NetRpcManager {
	static robin_hood::unordered_map<hash_t, ZMethodBase<uuid_t>*> m_methods;

	uuid_t _ServerID() {
		return Valhalla()->m_serverUuid;
	}

	void _Register(const std::string& name, ZMethodBase<uuid_t>* method) {
		m_methods.insert({ Utils::GetStableHashCode(name.c_str()), method });
	}

	void _RouteRPC(Data data) {
		auto pkg(PKG());
		data.Serialize(pkg);

		if (data.m_targetPeerID == EVERYBODY) {
			for (auto&& peer : NetManager::GetPeers()) {
				peer->m_rpc->Invoke("RoutedRPC", pkg);
			}
		}
		else {
			auto peer = NetManager::GetPeer(data.m_targetPeerID);
			if (peer) {
				peer->m_rpc->Invoke("RoutedRPC", pkg);
			}
		}
	}

	// Token: 0x06000AA3 RID: 2723 RVA: 0x00050474 File Offset: 0x0004E674
	void _HandleRoutedRPC(Data data) {
		// If method call is for rerouting
		if (data.m_targetZDO) {
			throw std::runtime_error("Not implemented");
			//ZDO zdo = ZDOMan.instance.GetZDO(data.m_targetZDO);
			//if (zdo != null) {
			//	ZNetView znetView = ZNetScene.instance.FindInstance(zdo);
			//	if (znetView != null) {
			//		znetView.HandleRoutedRPC(data);
			//	}
			//}
		}
		else {
			auto&& find = m_methods.find(data.m_methodHash);
			if (find != m_methods.end()) {
				find->second->Invoke(data.m_targetPeerID, data.m_parameters);
			}
			else {
				LOG(INFO) << "Client tried invoking unknown RoutedRPC handler";
			}
		}
	}

	void RPC_RoutedRPC(NetRpc* rpc, NetPackage::Ptr pkg) {
		Data data(pkg);

		// Server is the intended receiver
		if (data.m_targetPeerID == Valhalla()->m_serverUuid
			|| data.m_targetPeerID == EVERYBODY)
			_HandleRoutedRPC(data);

		// Server acts as a middleman
		if (data.m_targetPeerID != Valhalla()->m_serverUuid)
			_RouteRPC(data);
	}

	void _InvokeRoute(uuid_t target, const ZDOID& targetZDO, const std::string& name, NetPackage::Ptr pkg) {
		static uuid_t m_rpcMsgID = 1;
		static auto SERVER_ID(Valhalla()->m_serverUuid);

		Data data;
		data.m_msgID = SERVER_ID + m_rpcMsgID++;
		data.m_senderPeerID = SERVER_ID;
		data.m_targetPeerID = target;
		data.m_targetZDO = targetZDO;
		data.m_methodHash = Utils::GetStableHashCode(name.c_str());
		data.m_parameters = pkg;

		data.m_parameters->GetStream().ResetPos();

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
		peer->m_rpc->Register("RoutedRPC", &RPC_RoutedRPC);
		// ZoneSystem callback
		//if (this.m_onNewPeer != null)
		//{
		//	this.m_onNewPeer(peer.m_uid);
		//}
	}

	void OnPeerQuit(NetPeer::Ptr peer) {
		throw std::runtime_error("Not implemented");
	}
}