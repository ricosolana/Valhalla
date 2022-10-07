#pragma once

#include <robin_hood.h>
#include "Method.h"
#include "NetSocket.h"

#include "Task.h"

enum class ConnectionStatus;

class NetRpc {
	std::chrono::steady_clock::time_point m_lastPing;
	Task* m_pingTask = nullptr;
	bool m_ignore = false;
	
	robin_hood::unordered_map<hash_t, std::unique_ptr<ZMethodBase<NetRpc*>>> m_methods;

	void SendPackage(NetPackage::Ptr pkg);
	void Register(const char* name, ZMethodBase<NetRpc*>* method);

public:	
	ISocket::Ptr m_socket;

	NetRpc(ISocket::Ptr socket);
	~NetRpc();

	/**
		* @brief Register a static method for remote invocation
		* @param name function name to register
		* @param method ptr to a static function
	*/
	template<class ...Args>
	auto Register(const char* name, void(*f)(NetRpc*, Args...)) {
		return Register(name, new ZMethod(f));
	}
		
	/**
		* @brief Register an instance method for remote invocation
		* @param name function name to register
		* @param object the object containing the member function
		* @param method ptr to a member function
	*/
	template<class C, class ...Args>
	auto Register(const char* name, C *object, void(C::*f)(NetRpc*, Args...)) {
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
		NetPackage::Serialize(pkg, std::move(params)...); // serialize
		SendPackage(pkg);
	}

	// To be called every tick
	void Update();

	// Get the ping
	std::chrono::milliseconds GetPing();

	void SendError(ConnectionStatus status);
};
