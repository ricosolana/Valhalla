#pragma once

#include "Peer.h"
#include "Method.h"
#include "ValhallaServer.h"

class IRouteManager {
	friend class INetManager;

	struct Data {
		OWNER_t m_msgID;
		OWNER_t m_senderPeerID;
		OWNER_t m_targetPeerID;
		NetID m_targetSync;
		HASH_t m_methodHash;
		NetPackage m_parameters;

		Data() : m_msgID(0), m_senderPeerID(0), m_targetPeerID(0), m_methodHash(0) {}

		// Will unpack the package
		explicit Data(NetPackage& pkg)
			: m_msgID(pkg.Read<OWNER_t>()),
			m_senderPeerID(pkg.Read<OWNER_t>()),
			m_targetPeerID(pkg.Read<OWNER_t>()),
			m_targetSync(pkg.Read<NetID>()),
			m_methodHash(pkg.Read<HASH_t>()),
			m_parameters(pkg.Read<NetPackage>())
		{}

		void Serialize(NetPackage& pkg) const {
			pkg.Write(m_msgID);
			pkg.Write(m_senderPeerID);
			pkg.Write(m_targetPeerID);
			pkg.Write(m_targetSync);
			pkg.Write(m_methodHash);
			pkg.Write(m_parameters);
		}
	};

public:
	static constexpr OWNER_t EVERYBODY = 0;

private:	
	robin_hood::unordered_map<HASH_t, std::unique_ptr<IMethod<Peer*>>> m_methods;

private:
	// Called from NetManager
	void OnNewPeer(Peer *peer);

	// Internal use only by NetRouteManager
	void InvokeImpl(OWNER_t target, const NetID& targetNetSync, HASH_t hash, const NetPackage& pkg);
	void HandleRoutedRPC(Peer* peer, Data data);

	void RouteRPC(const Data& data);

public:

	/**
		* @brief Register a static method for routed remote invocation
		* @param name function name to register
		* @param method ptr to a static function
	*/
	template<typename F>
	void Register(HASH_t hash, F func) {
		assert(!m_methods.contains(hash));
		m_methods[hash] = std::unique_ptr<IMethod<Peer*>>(new MethodImpl(func));
	}

	template<typename F>
	auto Register(const std::string& name, F func) {
		return Register(VUtils::String::GetStableHashCode(name), func);
	}



	// Invoke a routed function bound to a peer with sub zdo
	template <typename... Args>
	void InvokeView(OWNER_t target, const NetID& targetNetSync, HASH_t hash, const Args&... params) {
		InvokeImpl(target, targetNetSync, hash, NetPackage::Serialize(params...));
	}

	// Invoke a routed function bound to a peer with sub zdo
	template <typename... Args>
	void InvokeView(OWNER_t target, const NetID& targetNetSync, const char* name, const Args&... params) {
		InvokeImpl(target, targetNetSync, VUtils::String::GetStableHashCode(name), NetPackage::Serialize(params...));
	}

	// Invoke a routed function bound to a peer with sub zdo
	template <typename... Args>
	void InvokeView(OWNER_t target, const NetID& targetNetSync, std::string& name, const Args&... params) {
		InvokeImpl(target, targetNetSync, VUtils::String::GetStableHashCode(name), NetPackage::Serialize(params...));
	}



	// Invoke a routed function bound to a peer
	template <typename... Args>
	void Invoke(OWNER_t target, HASH_t hash, const Args&... params) {
		InvokeView(target, NetID::NONE, hash, params...);
	}

	// Invoke a routed function bound to a peer
	template <typename... Args>
	void Invoke(OWNER_t target, const char* name, const Args&... params) {
		InvokeView(target, NetID::NONE, VUtils::String::GetStableHashCode(name), params...);
	}

	// Invoke a routed function bound to a peer
	template <typename... Args>
	void Invoke(OWNER_t target, std::string& name, const Args&... params) {
		InvokeView(target, NetID::NONE, VUtils::String::GetStableHashCode(name), params...);
	}



	// Invoke a routed function targeted to all peers
	template <typename... Args>
	void InvokeAll(HASH_t hash, const Args&... params) {
		Invoke(EVERYBODY, hash, params...);
	}

	// Invoke a routed function targeted to all peers
	template <typename... Args>
	void InvokeAll(const char* name, const Args&... params) {
		Invoke(EVERYBODY, VUtils::String::GetStableHashCode(name), params...);
	}

	// Invoke a routed function targeted to all peers
	template <typename... Args>
	void InvokeAll(std::string& name, const Args&... params) {
		Invoke(EVERYBODY, VUtils::String::GetStableHashCode(name), params...);
	}

};

IRouteManager* RouteManager();
