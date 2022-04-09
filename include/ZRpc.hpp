#pragma once

#include <robin_hood.h>
#include "ZRpcMethod.hpp"
#include "ZSocket.hpp"

#include "Task.hpp"

struct ZNetPeer;

/**
* The client and Rpc should be merged somehow
	* @brief
	*
*/
class ZRpc {
	ZSocket2::Ptr m_socket;
	std::chrono::steady_clock::time_point m_lastPing;
	Task* m_pingTask = nullptr;
	robin_hood::unordered_map<int32_t, std::unique_ptr<ZRpcMethodBase>> m_methods;

	void Serialize(ZPackage pkg) {}

	void SendPackage(ZPackage pkg);

public:
	ZRpc(ZSocket2::Ptr socket);
	~ZRpc();

	bool IsConnected();

	template <typename T, typename... Types>
	static void Serialize(ZPackage pkg, T var1, Types... var2) {
		pkg->Write(var1);

		Serialize(pkg, var2...);
	}

	/**
		* @brief Register a method to be remotely invoked
		* @param name the function identifier
		* @param method the function
	*/
	// TODO hide away the 'new' operator while being passed
	// and/or instead use std function or bind?
	void Register(const char* name, ZRpcMethodBase* method);

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
		Serialize(pkg, params...); // serialize
		SendPackage(pkg);
	}

	void Update();
};
