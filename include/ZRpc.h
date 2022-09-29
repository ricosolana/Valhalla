#pragma once

#include <robin_hood.h>
#include "ZMethod.h"
#include "ZSocket.h"
//#include "ValhallaServer.h"

#include "Task.h"

// Register an rpc method for remote invocation
#define REGISTER_RPC(rpc, name, method) rpc->Register(name, new ZMethod(this, &method));

/**
* The client and Rpc should be merged somehow
	* @brief
	*
*/
class ZRpc {
	std::chrono::steady_clock::time_point m_lastPing;
	Task* m_pingTask = nullptr;
		
	// should probably use std::function
	// as its more flexible
	robin_hood::unordered_map<int32_t, std::unique_ptr<ZMethodBase<ZRpc*>>> m_methods;

	void SendPackage(ZPackage::Ptr pkg);

public:	
	ISocket::Ptr m_socket;

	ZRpc(ISocket::Ptr socket);
	~ZRpc();

	/**
		* @brief Register a method to be remotely invoked
		* @param name the function identifier
		* @param method the function
	*/
	// TODO hide away the 'new' operator while being passed
	// and/or instead use std function or bind?
	void Register(const char* name, ZMethodBase<ZRpc*> *method);

	/**
		* @brief Invoke a function remotely
		* @param name function name
		* @param ...types function params
	*/
	template <typename... Types>
	void Invoke(const char* method, Types... params) {
		if (m_socket->GetConnectivity() == Connectivity::CLOSED)
			return;

		auto pkg(PKG());
		auto stable = Utils::GetStableHashCode(method);
		pkg->Write(stable);
#if TRUE // debug mode
		pkg->Write(method);
#endif
		ZPackage::Serialize(pkg, std::move(params)...); // serialize
		SendPackage(pkg);
	}

	// To be called every tick
	void Update();



	/* RPC wrapped methods
	*/
	//void RemotePrint(std::string& s);
	//void SendPlayerList();
	//void 
};
