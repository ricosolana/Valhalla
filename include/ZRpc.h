#pragma once

#include <robin_hood.h>
#include "ZMethod.h"
#include "ZSocket.h"
//#include "ValhallaServer.h"

#include "Task.h"

// Register an rpc method for remote invocation
//#define REGISTER_RPC(rpc, name, method) rpc->Register(name, new ZMethod(this, &method));

#define RPC_DEBUG

enum class ConnectionStatus;

/**
* The client and Rpc should be merged somehow
	* @brief
	*
*/
class ZRpc {
	std::chrono::steady_clock::time_point m_lastPing;
	Task* m_pingTask = nullptr;
	bool m_ignore = false;
	
	robin_hood::unordered_map<hash_t, std::unique_ptr<ZMethodBase<ZRpc*>>> m_methods;

	void SendPackage(ZPackage::Ptr pkg);
	void Register(const char* name, ZMethodBase<ZRpc*>* method);

public:	
	ISocket::Ptr m_socket;

	ZRpc(ISocket::Ptr socket);
	~ZRpc();

	/**
		* @brief Register a static method for remote invocation
		* @param name function name to register
		* @param method ptr to a static function
	*/
	template<class ...Args>
	auto Register(const char* name, void(*f)(ZRpc*, Args...)) {
		return Register(name, new ZMethod(f));
	}

	//template<class
	//auto Register(const char* name, )
	
	/**
		* @brief Register an instance method for remote invocation
		* @param name function name to register
		* @param object the object containing the member function
		* @param method ptr to a member function
	*/
	template<class C, class ...Args>
	auto Register(const char* name, C *object, void(C::*f)(ZRpc*, Args...)) {
		return Register(name, new ZMethod(object, f));
	}

	/**
		* @brief Invoke a remote function
		* @param name function name to invoke
		* @param ...types function parameters
	*/
	template <typename... Types>
	void Invoke(const char* method, Types... params) {
		if (m_socket->GetConnectivity() == Connectivity::CLOSED)
			return;

		auto pkg(PKG());
		auto stable = Utils::GetStableHashCode(method);
		pkg->Write(stable);
#ifdef RPC_DEBUG // debug mode
		pkg->Write(method);
#endif
		ZPackage::Serialize(pkg, std::move(params)...); // serialize
		SendPackage(pkg);
	}

	// To be called every tick
	void Update();

	// Get the ping in ms
	std::chrono::milliseconds GetPing();

	void SendError(ConnectionStatus status);
};
