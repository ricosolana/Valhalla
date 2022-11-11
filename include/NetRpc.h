#pragma once

#include <robin_hood.h>

#include "Method.h"
#include "NetSocket.h"
#include "Task.h"
#include "NetHashes.h"
#include "Utils.h"

enum class ConnectionStatus;

class NetRpc {
public:
    using MethodPtr = std::unique_ptr<IMethod<NetRpc&>>;

    template<class ...Args>
    using FuncPtr = void(*)(NetRpc&, Args...);

    template<class C, class ...Args>
    using ClFuncPtr = void(C::*)(NetRpc&, Args...);

private:
    static constexpr HASH_t RPC_HASH = Utils::GetStableHashCode("Rpc");

private:
	std::chrono::steady_clock::time_point m_lastPing;
	
	robin_hood::unordered_map<HASH_t, MethodPtr> m_methods;

private:
	void SendPackage(NetPackage pkg) const {
		m_socket->Send(std::move(pkg));
	}

public:
    ISocket::Ptr m_socket;

public:
	explicit NetRpc(ISocket::Ptr socket);

	NetRpc(const NetRpc& other) = delete; // copy
	NetRpc(NetRpc&& other) = delete; // move

	~NetRpc();



    void Register(HASH_t hash, MethodPtr method);

	/**
		* @brief Register a static method for remote invocation
		* @param name function name to register
		* @param method ptr to a static function
	*/
	template<class ...Args>
	auto Register(HASH_t hash, FuncPtr<Args...> f) {
		return Register(hash, MethodPtr(new MethodImpl(f, RPC_HASH ^ hash)));
	}

	template<class ...Args>
	auto Register(Rpc_Hash hash, FuncPtr<Args...> f) { //FuncPtr<Args...> f) {
		return Register(static_cast<HASH_t>(hash), f);
	}

	template<class ...Args>
	auto Register(const char* name, FuncPtr<Args...> f) {
		return Register(Utils::GetStableHashCode(name), f);
	}

	template<class ...Args>
	auto Register(std::string& name, FuncPtr<Args...> f) {
		return Register(name.c_str(), f);
	}
		


	/**
		* @brief Register an instance method for remote invocation
		* @param name function name to register
		* @param object the object containing the member function
		* @param method ptr to a member function
	*/
	template<class C, class ...Args>
	auto Register(HASH_t hash, C* object, ClFuncPtr<C, Args...> f) {
		return Register(hash, std::make_unique<MethodImpl>(object, f, RPC_HASH ^ hash));
	}

	template<class C, class ...Args>
	auto Register(Rpc_Hash hash, C *object, ClFuncPtr<C, Args...> f) {
		return Register(static_cast<HASH_t>(hash), object, f);
	}

	template<class C, class ...Args>
	auto Register(const char* name, C* object, ClFuncPtr<C, Args...> f) {
		return Register(Utils::GetStableHashCode(name), object, f);
	}

	template<class C, class ...Args>
	auto Register(std::string& name, C* object, ClFuncPtr<C, Args...> f) {
		return Register(name.c_str(), object, f);
	}



	/**
		* @brief Invoke a remote function
		* @param name function name to invoke
		* @param ...types function parameters
	*/
	// Passing parameter pack by reference
	// https://stackoverflow.com/a/6361619
	template <typename... Types>
	void Invoke(HASH_t hash, const Types&... params) {
		if (!m_socket->Connected())
			return;

		NetPackage pkg; // TODO make into member to optimize; or make static
		pkg.Write(hash);
#ifdef RPC_DEBUG // debug mode
#error not implemented
		pkg.Write(hash);
#endif
		NetPackage::_Serialize(pkg, params...); // serialize
		SendPackage(std::move(pkg));
	}

	template <typename... Types>
	void Invoke(Rpc_Hash hash, const Types&... params) {
		Invoke(static_cast<HASH_t>(hash), params...);
	}

	template <typename... Types>
	void Invoke(const char* name, Types... params) {
		Invoke(Utils::GetStableHashCode(name), std::move(params)...);
	}

	template <typename... Types>
	void Invoke(std::string &name, const Types&... params) {
		Invoke(name.c_str(), params...);
	}

	// Call every frame
	void Update();

	void SendError(ConnectionStatus status);
};
