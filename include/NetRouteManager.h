#pragma once

#include "NetPeer.h"
#include "Method.h"
#include "ValhallaServer.h"

class IRouteManager {
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
	robin_hood::unordered_map<HASH_t, std::unique_ptr<IMethod<NetPeer*>>> m_methods;

private:
	// Called from NetManager
	void OnNewPeer(NetPeer *peer);

	// Internal use only by NetRouteManager
	void Invoke(OWNER_t target, const NetID& targetNetSync, HASH_t hash, const NetPackage& pkg);
	void HandleRoutedRPC(NetPeer* peer, Data data);

	void RouteRPC(const Data& data);

	IRouteManager() {}

public:

	//enum ChatManager::Type;
	//void Register(HASH_t, std::function<void(OWNER_t, Vector3, ChatManager::Type, std::string, std::string, std::string)>);

	/**
		* @brief Register a static method for routed remote invocation
		* @param name function name to register
		* @param method ptr to a static function
	*/
	template<typename F>
	void Register(HASH_t hash, F func) {
		assert(!m_methods.contains(hash));
		m_methods[hash] = std::unique_ptr<IMethod<NetPeer*>>(new MethodImpl(func));
	}

	template<typename F>
	auto Register(const std::string& name, F func) {
		return Register(VUtils::String::GetStableHashCode(name), func);
	}



	/**
		* @brief Invoke a routed function bound to a peer with sub zdo
		* @param name function name
		* @param ...types function parameters
	*/
	template <typename... Args>
	void Invoke(OWNER_t target, const NetID& targetNetSync, HASH_t hash, const Args&... params) {
		Invoke(target, targetNetSync, hash, NetPackage::Serialize(params...));
	}

	template <typename... Args>
	void Invoke(OWNER_t target, const NetID& targetNetSync, const char* name, const Args&... params) {
		Invoke(target, targetNetSync, VUtils::String::GetStableHashCode(name), params...);
	}

	template <typename... Args>
	void Invoke(OWNER_t target, const NetID& targetNetSync, std::string& name, const Args&... params) {
		Invoke(target, targetNetSync, name.c_str(), params...);
	}



	/**
		* @brief Invoke a routed function bound to a peer
		* @param name function name
		* @param ...types function parameters
	*/
	template <typename... Args>
	void Invoke(OWNER_t target, HASH_t hash, const Args&... params) {
		Invoke(target, NetID::NONE, hash, params...);
	}

	template <typename... Args>
	void Invoke(OWNER_t target, const char* name, const Args&... params) {
		Invoke(target, NetID::NONE, VUtils::String::GetStableHashCode(name), params...);
	}

	template <typename... Args>
	void Invoke(OWNER_t target, std::string& name, const Args&... params) {
		Invoke(target, NetID::NONE, name.c_str(), params...);
	}
};

IRouteManager* RouteManager();