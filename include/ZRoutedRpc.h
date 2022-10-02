#pragma once

#include <robin_hood.h>
#include "ZNetPeer.h"
#include "ZMethod.h"

class ZRoutedRpc {
	struct RoutedRPCData
	{
		// Deserialize() method:
		// rewritten as constructor because its only used for this
		RoutedRPCData(ZPackage::Ptr pkg) 
			: m_msgID(pkg->Read<uuid_t>()),
			m_senderPeerID(pkg->Read<uuid_t>()),
			m_targetPeerID(pkg->Read<uuid_t>()),
			m_targetZDO(pkg->Read<ZDOID>()),
			m_methodHash(pkg->Read<hash_t>()),
			m_parameters(pkg->Read<ZPackage::Ptr>())
		{}

		void Serialize(ZPackage::Ptr pkg) {
			pkg->Write(m_msgID);
			pkg->Write(m_senderPeerID);
			pkg->Write(m_targetPeerID);
			pkg->Write(m_targetZDO);
			pkg->Write(m_methodHash);
			pkg->Write(m_parameters);
		}

		uuid_t m_msgID;
		uuid_t m_senderPeerID;
		uuid_t m_targetPeerID;
		ZDOID m_targetZDO;
		hash_t m_methodHash;
		ZPackage::Ptr m_parameters;// = new ZPackage();
	};

public:
	ZRoutedRpc() {}

	void OnNewPeer(ZNetPeer::Ptr peer) {
		//peer->m_rpc->Register("RoutedRPC", new Action<ZRpc, ZPackage>(this.RPC_RoutedRPC));
		//REGISTER_RPC(peer->m_rpc, "RoutedRPC", RPC_RoutedRPC);
		// also call ZoneSystem callback

		//if (this.m_onNewPeer != null)
		//{
		//	this.m_onNewPeer(peer.m_uid);
		//}
	}

	//void SetUID(UID_t uid);
	//void AddPeer(ZNetPeer *peer);
	//void RemovePeer(ZNetPeer *peer);

	void Register(const char* name, ZMethodBase<uuid_t>* method);

	template <typename... Types>
	void InvokeRoutedRPC(const char* methodName, Types... params) {
		//InvokeRoutedRPC(m_id, methodName, params);
	}

	template <typename... Types>
	void InvokeRoutedRPC(uuid_t uuid, const char* methodName, Types... params) {
		//InvokeRoutedRPC(uuid, ZDOID::NONE, methodName, params);
	}

	//int64_t GetServerPeerID();

	template <typename... Types>
	void InvokeRoutedRPC(uuid_t targetPeerID, ZDOID targetZDO, std::string methodName, Types... params) {
		//auto pkg = new ZPackage();
		//auto stable = Utils::GetStableHashCode(method);
		//pkg->Write(stable);
		//ZRpc::Serialize(pkg, params...); // serialize
		//SendPackage(pkg);

		throw std::runtime_error("not implemented");

		//RoutedRPCData routedRPCData;
		//
		//routedRPCData.m_msgID = m_id + (int64_t)m_rpcMsgID;
		//routedRPCData.m_senderPeerID = m_id;
		//routedRPCData.m_targetPeerID = targetPeerID;
		//routedRPCData.m_targetZDO = targetZDO;
		//routedRPCData.m_methodHash = Utils::GetStableHashCode(methodName);
		//ZRpc::Serialize(routedRPCData.m_parameters, params);
		//routedRPCData.m_parameters->GetStream().ResetPos();
		//
		//m_rpcMsgID++;
		//
		//if (targetPeerID == this.m_id)
		//	// Client is the final receiver
		//	this.HandleRoutedRPC(routedRPCData);
		//else {
		//	// Send to server
		//	//ZPackage pkg;
		//	//rpcData.Serialize(pkg);
		//	//peer->m_rpc.Invoke("RoutedRPC", pkg);
		//}
	}

public:
	static constexpr uuid_t EVERYBODY = 0;

private:
	void RouteRPC(RoutedRPCData rpcData);
	void RPC_RoutedRPC(ZRpc* rpc, ZPackage::Ptr pkg);
	void HandleRoutedRPC(RoutedRPCData data);

	int32_t m_rpcMsgID = 1;
	robin_hood::unordered_map<int32_t, ZMethodBase<uuid_t>*> m_functions;
};

//ZRoutedRpc* RoutedRpc();