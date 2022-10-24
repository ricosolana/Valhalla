#pragma once

#include <robin_hood.h>

#include "Method.h"
#include "NetSocket.h"
#include "Task.h"
#include "NetHashes.h"
#include "Utils.h"

enum class ConnectionStatus;

class NetRpc {
	std::chrono::steady_clock::time_point m_lastPing;
	bool m_closeEventually = false;
	
	robin_hood::unordered_map<HASH_t, std::unique_ptr<IMethod<NetRpc*>>> m_methods;

	void SendPackage(const NetPackage &pkg) {
		m_socket->Send(pkg);
	}

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
	auto Register(HASH_t hash, void(*f)(NetRpc*, Args...)) {
		return Register(hash, new MethodImpl(f));
	}

	template<class ...Args>
	auto Register(Rpc_Hash hash, void(*f)(NetRpc*, Args...)) {
		return Register(static_cast<HASH_t>(hash), new MethodImpl(f));
	}

	//template<class ...Args>
	//auto Register(const char* name, void(*f)(NetRpc*, Args...)) {
	//	return Register(Utils::GetStableHashCode(name), new MethodImpl(f));
	//}

	template<class ...Args>
	auto Register(const std::string& name, void(*f)(NetRpc*, Args...)) {
		return Register(Utils::GetStableHashCode(name), new MethodImpl(f));
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

	template<class C, class ...Args>
	auto Register(Rpc_Hash hash, C *object, void(C::*f)(NetRpc*, Args...)) {
		return Register(static_cast<HASH_t>(hash), new MethodImpl(object, f));
	}

	//template<class C, class ...Args>
	//auto Register(const char* name, C* object, void(C::* f)(NetRpc*, Args...)) {
	//	return Register(Utils::GetStableHashCode(name), new MethodImpl(object, f));
	//}

	template<class C, class ...Args>
	auto Register(const std::string& name, C* object, void(C::* f)(NetRpc*, Args...)) {
		return Register(Utils::GetStableHashCode(name), new MethodImpl(object, f));
	}



	/**
		* @brief Invoke a remote function
		* @param name function name to invoke
		* @param ...types function parameters
	*/
	// Passing parameter pack by reference
	// https://stackoverflow.com/a/6361619
	template <typename... Types>
	void Invoke(HASH_t hash, Types... params) {
		if (!m_socket->Connected())
			return;

		NetPackage pkg; // TODO make into member to optimize; or even crazier, make static
		pkg.Write(hash);
#ifdef RPC_DEBUG // debug mode
#error not implemented
		pkg.Write(hash);
#endif
		NetPackage::_Serialize(pkg, std::move(params)...); // serialize
		SendPackage(pkg);
	}

	template <typename... Types>
	void Invoke(Rpc_Hash hash, Types... params) {
		Invoke(static_cast<HASH_t>(hash), std::move(params)...);
	}

	//template <typename... Types>
	//void Invoke(const char* name, Types... params) {
	//	Invoke(Utils::GetStableHashCode(name), std::move(params)...);
	//}

	template <typename... Types>
	void Invoke(const std::string &name, Types... params) {
		Invoke(Utils::GetStableHashCode(name), std::move(params)...);
	}

	// To be called every tick
	void Update();

	// Get the ping
	//std::chrono::milliseconds GetPing();

	void SendError(ConnectionStatus status);
};
