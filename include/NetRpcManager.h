#pragma once

#include "NetPeer.h"
#include "Method.h"

namespace NetRpcManager {

	static constexpr UUID_t EVERYBODY = 0;

	struct Data {
		UUID_t m_msgID;
		UUID_t m_senderPeerID;
		UUID_t m_targetPeerID;
		NetID m_targetNetSync;
		HASH_t m_methodHash;
		NetPackage::Ptr m_parameters;

		Data() {}

		// Will unpack the package
		Data(NetPackage::Ptr pkg)
			: m_msgID(pkg->Read<UUID_t>()),
			m_senderPeerID(pkg->Read<UUID_t>()),
			m_targetPeerID(pkg->Read<UUID_t>()),
			m_targetNetSync(pkg->Read<NetID>()),
			m_methodHash(pkg->Read<HASH_t>()),
			m_parameters(pkg->Read<NetPackage::Ptr>())
		{}

		void Serialize(NetPackage::Ptr pkg) {
			pkg->Write(m_msgID);
			pkg->Write(m_senderPeerID);
			pkg->Write(m_targetPeerID);
			pkg->Write(m_targetNetSync);
			pkg->Write(m_methodHash);
			pkg->Write(m_parameters);
		}
	};

	// Called from NetManager
	void OnNewPeer(NetPeer::Ptr peer);
	void OnPeerQuit(NetPeer::Ptr peer);

	// Internal use only by NetRpcManager
	UUID_t _ServerID();
	void _InvokeRoute(UUID_t target, const NetID& targetNetSync, HASH_t hash, NetPackage::Ptr pkg);
	void _HandleRoutedRPC(Data data);

	void _Register(HASH_t hash, IMethod<UUID_t>* method);



	/**
		* @brief Register a static method for routed remote invocation
		* @param name function name to register
		* @param method ptr to a static function
	*/
	template<class ...Args>
	auto Register(HASH_t hash, void(*f)(UUID_t, Args...)) {
		return _Register(hash, new MethodImpl(f));
	}

	template<class ...Args>
	auto Register(RoutedRpc_Hash hash, void(*f)(UUID_t, Args...)) {
		return _Register(static_cast<HASH_t>(hash), new MethodImpl(f));
	}

	template<class ...Args>
	auto Register(const char* name, void(*f)(UUID_t, Args...)) {
		return _Register(Utils::GetStableHashCode(name), new MethodImpl(f));
	}

	template<class ...Args>
	auto Register(const std::string& name, void(*f)(UUID_t, Args...)) {
		return _Register(Utils::GetStableHashCode(name), new MethodImpl(f));
	}



	/**
		* @brief Register an instance method for routed remote invocation
		* @param name function name to register
		* @param object the object containing the member function
		* @param method ptr to a member function
	*/
	template<class C, class ...Args>
	auto Register(HASH_t hash, C* object, void(C::* f)(UUID_t, Args...)) {
		return _Register(hash, new MethodImpl(object, f));
	}

	template<class C, class ...Args>
	auto Register(RoutedRpc_Hash hash, C* object, void(C::* f)(UUID_t, Args...)) {
		return _Register(static_cast<HASH_t>(hash), new MethodImpl(object, f));
	}

	template<class C, class ...Args>
	auto Register(const char* name, C* object, void(C::* f)(UUID_t, Args...)) {
		return _Register(Utils::GetStableHashCode(name), new MethodImpl(object, f));
	}

	template<class C, class ...Args>
	auto Register(const std::string& name, C* object, void(C::* f)(UUID_t, Args...)) {
		return _Register(Utils::GetStableHashCode(name), new MethodImpl(object, f));
	}



	/**
		* @brief Invoke a routed remote function
		* @param name function name to invoke
		* @param ...types function parameters
	*/
	template <typename... Args>
	void Invoke(UUID_t target, const NetID& targetNetSync, HASH_t hash, Args... params) {
		auto pkg(PKG());
		NetPackage::Serialize(pkg, params...);
		_InvokeRoute(target, targetNetSync, hash, pkg);
	}

	template <typename... Args>
	void Invoke(UUID_t target, const NetID& targetNetSync, RoutedRpc_Hash hash, Args... params) {
		auto pkg(PKG());
		NetPackage::Serialize(pkg, params...);
		_InvokeRoute(target, targetNetSync, static_cast<HASH_t>(hash), pkg);
	}

	template <typename... Args>
	void Invoke(UUID_t target, const NetID& targetNetSync, const char* name, Args... params) {
		auto pkg(PKG());
		NetPackage::Serialize(pkg, params...);
		_InvokeRoute(target, targetNetSync, Utils::GetStableHashCode(name), pkg);
	}

	template <typename... Args>
	void Invoke(UUID_t target, const NetID& targetNetSync, const std::string& name, Args... params) {
		auto pkg(PKG());
		NetPackage::Serialize(pkg, params...);
		_InvokeRoute(target, targetNetSync, Utils::GetStableHashCode(name), pkg);
	}



	/**
		* @brief Invoke a routed remote function
		* @param name function name to invoke
		* @param ...types function parameters
	*/
	template <typename... Args>
	void Invoke(UUID_t target, HASH_t hash, Args... params) {
		Invoke(target, NetID::NONE, hash, params...);
	}

	template <typename... Args>
	void Invoke(UUID_t target, RoutedRpc_Hash hash, Args... params) {
		Invoke(target, NetID::NONE, static_cast<HASH_t>(hash), params...);
	}

	template <typename... Args>
	void Invoke(UUID_t target, const char* name, Args... params) {
		Invoke(target, NetID::NONE, Utils::GetStableHashCode(name), params...);
	}

	template <typename... Args>
	void Invoke(UUID_t target, const std::string& name, Args... params) {
		Invoke(target, NetID::NONE, Utils::GetStableHashCode(name), params...);
	}



	/**
		* @brief Invoke a routed remote function
		* @param name function name to invoke
		* @param ...types function parameters
	*/
	template <typename... Args>
	void Invoke(HASH_t hash, Args... params) {
		Invoke(_ServerID(), hash, params...);
	}

	template <typename... Args>
	void Invoke(RoutedRpc_Hash hash, Args... params) {
		Invoke(_ServerID(), static_cast<HASH_t>(hash), params...);
	}

	template <typename... Args>
	void Invoke(const char* name, Args... params) {
		Invoke(_ServerID(), Utils::GetStableHashCode(name), params...);
	}

	template <typename... Args>
	void Invoke(const std::string& name, Args... params) {
		Invoke(_ServerID(), Utils::GetStableHashCode(name), params...);
	}

};
