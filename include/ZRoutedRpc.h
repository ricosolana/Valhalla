#pragma once

#include <robin_hood.h>
#include "ZNetPeer.h"
#include "ZMethod.h"

class ZRoutedRpc {
	struct RoutedRPCData
	{
		void Serialize(ZPackage pkg);
		void Deserialize(ZPackage pkg);

		int64_t m_msgID;
		int64_t m_senderPeerID;
		int64_t m_targetPeerID;
		ZDOID m_targetZDO;
		int32_t m_methodHash;
		ZPackage m_parameters;// = new ZPackage();
	};

public:
	ZRoutedRpc(UID_t uid);

	//void SetUID(UID_t uid);
	//void AddPeer(ZNetPeer *peer);
	//void RemovePeer(ZNetPeer *peer);

	void Register(const char* name, ZMethodBase<UID_t>* method);

	template <typename... Types>
	void InvokeRoutedRPC(const char* methodName, Types... params) {
		InvokeRoutedRPC(m_id, methodName, params);
	}

	template <typename... Types>
	void InvokeRoutedRPC(UID_t targetPeerID, const char* methodName, Types... params) {
		InvokeRoutedRPC(targetPeerID, ZDOID::NONE, methodName, params);
	}

	//int64_t GetServerPeerID();

	template <typename... Types>
	void InvokeRoutedRPC(long targetPeerID, ZDOID targetZDO, std::string methodName, Types... params) {
		//auto pkg = new ZPackage();
		//auto stable = Utils::GetStableHashCode(method);
		//pkg->Write(stable);
		//ZRpc::Serialize(pkg, params...); // serialize
		//SendPackage(pkg);

		throw std::runtime_error("not implemented");

		RoutedRPCData routedRPCData;
		
		routedRPCData.m_msgID = m_id + (int64_t)m_rpcMsgID;
		routedRPCData.m_senderPeerID = m_id;
		routedRPCData.m_targetPeerID = targetPeerID;
		routedRPCData.m_targetZDO = targetZDO;
		routedRPCData.m_methodHash = Utils::GetStableHashCode(methodName);
		ZRpc::Serialize(routedRPCData.m_parameters, params);
		routedRPCData.m_parameters.GetStream().ResetPos();

		m_rpcMsgID++;

		if (targetPeerID == this.m_id)
			// Client is the final receiver
			this.HandleRoutedRPC(routedRPCData);
		else {
			// Send to server
			//ZPackage pkg;
			//rpcData.Serialize(pkg);
			//peer->m_rpc.Invoke("RoutedRPC", pkg);
		}
	}

public:
	static UID_t Everybody;

private:
	//void RouteRPC(RoutedRPCData rpcData);
	void RPC_RoutedRPC(ZRpc rpc, ZPackage pkg);
	void HandleRoutedRPC(RoutedRPCData data);

	int32_t m_rpcMsgID = 1;
	UID_t m_id;
	//std::vector<ZNetPeer> m_peers;
	robin_hood::unordered_map<int32_t, ZMethodBase<UID_t>*> m_functions;
};

//ZRoutedRpc* RoutedRpc();