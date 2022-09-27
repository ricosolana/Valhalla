#pragma once

#include <robin_hood.h>
#include "ZMethod.h"
#include "ZSocket.h"

#include "Task.h"

/**
* The client and Rpc should be merged somehow
	* @brief
	*
*/
class ZRpc {
	ISocket::Ptr m_socket;
	std::chrono::steady_clock::time_point m_lastPing;
	Task* m_pingTask = nullptr;
	robin_hood::unordered_map<int32_t, std::unique_ptr<ZMethodBase<ZRpc*>>> m_methods;

	void SendPackage(ZPackage pkg);

public:	
	ZRpc(ISocket::Ptr socket);
	~ZRpc();

	bool IsConnected();

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
		if (!IsConnected())
			return;

		ZPackage pkg;
		auto stable = Utils::GetStableHashCode(method);
		pkg.Write(stable);
		ZPackage::Serialize(pkg, std::move(params)...); // serialize
		SendPackage(std::move(pkg));
	}

	void Update();
};
