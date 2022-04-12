#include "ZRoutedRpc.hpp"
#include "ZRoutedRpcMethod.hpp"

ZRoutedRpc::ZRoutedRpc(ZNetPeer* peer) 
	: m_peer(peer) {}

void ZRoutedRpc::SetUID(UID_t uid) {
	m_id = uid;
}

void ZRoutedRpc::Register(const char* name, ZRpcMethodBase* method) {
	throw std::runtime_error("not implemented");

	//m_functions.insert({Utils::GetStableHashCode(name), new ZRoutedRpcMethod(});
}

void ZRoutedRpc::RPC_RoutedRPC(ZRpc rpc, ZPackage pkg)
{
	throw std::runtime_error("not implemented");
	//RoutedRPCData routedRPCData;
	//routedRPCData.Deserialize(pkg);
	//if (routedRPCData.m_targetPeerID == m_id 
	//	|| routedRPCData.m_targetPeerID == 0L)
	//	HandleRoutedRPC(std::move(routedRPCData));
	
}

// Token: 0x06000AA3 RID: 2723 RVA: 0x00050474 File Offset: 0x0004E674
void ZRoutedRpc::HandleRoutedRPC(RoutedRPCData data)
{
	throw std::runtime_error("not implemented");
	//if (data.m_targetZDO)
	//{
	//
	//
	//	ZRoutedRpcMethodBase* base;
	//	RoutedMethodBase routedMethodBase;
	//	if (this.m_functions.TryGetValue(data.m_methodHash, out routedMethodBase))
	//	{
	//		routedMethodBase.Invoke(data.m_senderPeerID, data.m_parameters);
	//		return;
	//	}
	//}
	//else
	//{
	//	ZDO zdo = ZDOMan.instance.GetZDO(data.m_targetZDO);
	//	if (zdo != null)
	//	{
	//		ZNetView znetView = ZNetScene.instance.FindInstance(zdo);
	//		if (znetView != null)
	//		{
	//			znetView.HandleRoutedRPC(data);
	//		}
	//	}
	//}
}

void ZRoutedRpc::RoutedRPCData::Serialize(ZPackage pkg) {
	throw std::runtime_error("not implemented");
	pkg.Write(m_msgID);
	pkg.Write(m_senderPeerID);
	pkg.Write(m_targetPeerID);
	pkg.Write(m_targetZDO);
	pkg.Write(m_methodHash);
	pkg.Write(*m_parameters);
}

void ZRoutedRpc::RoutedRPCData::Deserialize(ZPackage pkg) {
	throw std::runtime_error("not implemented");
	//m_msgID = pkg.ReadLong();
	//m_senderPeerID = pkg.ReadLong();
	//m_targetPeerID = pkg.ReadLong();
	//m_targetZDO = pkg.ReadZDOID();
	//m_methodHash = pkg.ReadInt();
	//m_parameters = pkg.ReadPackage();
}
