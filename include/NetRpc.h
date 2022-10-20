#pragma once

#include <robin_hood.h>

#include "Method.h"
#include "NetSocket.h"
#include "Task.h"

enum class ConnectionStatus;

class NetRpc {
	std::chrono::steady_clock::time_point m_lastPing;
	bool m_ignore = false;
	
	robin_hood::unordered_map<HASH_t, std::unique_ptr<IMethod<NetRpc*>>> m_methods;

	void SendPackage(NetPackage::Ptr pkg);

	void Register(const char* name, IMethod<NetRpc*>* method);
	void Register(HASH_t hash, IMethod<NetRpc*>* method);

public:	
	ISocket::Ptr m_socket;

	NetRpc(ISocket::Ptr socket);

	NetRpc(const NetRpc& other) = delete; // copy
	NetRpc(NetRpc&& other) = delete; // move

	~NetRpc();

	/**
		* @brief Register a static method for remote invocation
		* @param name function name to register
		* @param method ptr to a static function
	*/
	template<class ...Args>
	auto Register(const char* name, void(*f)(NetRpc*, Args...)) {
		return Register(name, new MethodImpl(f));
	}
		
	/**
		* @brief Register an instance method for remote invocation
		* @param name function name to register
		* @param object the object containing the member function
		* @param method ptr to a member function
	*/
	template<class C, class ...Args>
	auto Register(const char* name, C *object, void(C::*f)(NetRpc*, Args...)) {
		return Register(name, new MethodImpl(object, f));
	}

	/**
		* @brief Register a static method for remote invocation
		* @param name function name to register
		* @param method ptr to a static function
	*/
	template<class ...Args>
	auto Register(HASH_t hash, void(*f)(NetRpc*, Args...)) {
		return Register(hash, new MethodImpl(f));
	}

	/**
		* @brief Register an instance method for remote invocation
		* @param name function name to register
		* @param object the object containing the member function
		* @param method ptr to a member function
	*/
	template<class C, class ...Args>
	auto Register(HASH_t hash, C* object, void(C::* f)(NetRpc*, Args...)) {
		return Register(hash, new MethodImpl(object, f));
	}



	/**
		* @brief Invoke a remote function
		* @param name function name to invoke
		* @param ...types function parameters
	*/
	template <typename... Types>
	void Invoke(const char* method, Types... params) {
		if (!m_socket->Connected())
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
	//std::chrono::milliseconds GetPing();

	void SendError(ConnectionStatus status);
};
