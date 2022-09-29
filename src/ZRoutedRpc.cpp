#include "ZRoutedRpc.h"
#include "ZMethod.h"
#include "ValhallaServer.h"

ZRoutedRpc::ZRoutedRpc(UUID uid)
	: m_id(uid) {}

void ZRoutedRpc::Register(const char* name, ZMethodBase<UUID>* method) {
	//throw std::runtime_error("not implemented");

	m_functions.insert({ Utils::GetStableHashCode(name), method });
}

void ZRoutedRpc::RPC_RoutedRPC(ZRpc *rpc, ZPackage::Ptr pkg) {
	RoutedRPCData data(pkg);

	// Server is the intended receiver
	if (data.m_targetPeerID == m_id 
		|| data.m_targetPeerID == EVERYBODY)
		HandleRoutedRPC(data);

	// Server acts as a middleman
	if (data.m_targetPeerID != m_id)
		RouteRPC(data);	
}

void ZRoutedRpc::RouteRPC(RoutedRPCData data) {
	auto pkg(PKG());
	data.Serialize(pkg);

	if (data.m_targetPeerID == EVERYBODY) {
		for (auto&& peer : m_peers) {
			peer->m_rpc->Invoke("RoutedRPC", pkg);
		}
	} else {
		auto peer = Valhalla()->m_znet->GetPeer(data.m_targetPeerID);
		if (peer) {
			peer->m_rpc->Invoke("RoutedRPC", pkg);
		}
	}
}

// Token: 0x06000AA3 RID: 2723 RVA: 0x00050474 File Offset: 0x0004E674
void ZRoutedRpc::HandleRoutedRPC(RoutedRPCData data) {
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
	} else {
		auto&& find = m_functions.find(data.m_methodHash);
		if (find != m_functions.end()) {
			find->second->Invoke(data.m_targetPeerID, data.m_parameters);
		}
		else {
			LOG(INFO) << "Client tried invoking unknown RoutedRPC handler";
		}
	}
}
