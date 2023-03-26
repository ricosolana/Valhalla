#pragma once

#include "Method.h"
#include "ValhallaServer.h"
#include "DataReader.h"
#include "DataWriter.h"
#include "ModManager.h"

class Peer;

class IRouteManager {
	friend class INetManager;

public:
	struct Data {
		//OWNER_t m_msgID; // TODO this is never utilized
		OWNER_t m_sender;
		OWNER_t m_target;
		ZDOID m_targetZDO;
		HASH_t m_method;
		BYTES_t m_params;

		Data() : m_sender(0), m_target(0), m_method(0) {}

		// Will unpack the package
		explicit Data(DataReader reader) {
			reader.Read<int64_t>(); // msgID
			m_sender = reader.Read<OWNER_t>();
			m_target = reader.Read<OWNER_t>();
			m_targetZDO = reader.Read<ZDOID>();
			m_method = reader.Read<HASH_t>();
			m_params = reader.Read<BYTES_t>();
		}

		void Serialize(DataWriter writer) const {
			writer.Write<int64_t>(0);
			writer.Write(m_sender);
			writer.Write(m_target);
			writer.Write(m_targetZDO);
			writer.Write(m_method);
			writer.Write(m_params);
		}
	};

	static constexpr OWNER_t EVERYBODY = 0;

private:	
	robin_hood::unordered_map<HASH_t, std::unique_ptr<IMethod<Peer*>>> m_methods;

private:
	// Called from NetManager
	void OnNewPeer(Peer &peer);

	// Internal use only by NetRouteManager
	void InvokeImpl(OWNER_t target, const ZDOID& targetNetSync, HASH_t hash, BYTES_t params);
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
		m_methods[hash] = std::make_unique<MethodImpl<Peer*, F>>(func, IModManager::Events::RouteIn, hash);
	}

	template<typename F>
	decltype(auto) Register(const std::string& name, F func) {
		return Register(VUtils::String::GetStableHashCode(name), func);
	}



	// Invoke a routed function bound to a peer with sub zdo
	template <typename... Args>
	void InvokeView(OWNER_t target, const ZDOID& targetNetSync, HASH_t hash, const Args&... params) {
		// Prefix
		//if (!ModManager()->CallEvent(IModManager::EVENT_RouteOut))
		// Routed is a bit more finicky because they are both serialized and deserialized at different places with different intent...

		InvokeImpl(target, targetNetSync, hash, DataWriter::Serialize(params...));
	}

	// Invoke a routed function bound to a peer with sub zdo
	template <typename... Args>
	void InvokeView(OWNER_t target, const ZDOID& targetNetSync, const std::string& name, const Args&... params) {
		InvokeView(target, targetNetSync, VUtils::String::GetStableHashCode(name), params...);
	}

	void InvokeViewLua(OWNER_t target, const ZDOID& targetNetSync, const IModManager::MethodSig& repr, const sol::variadic_args& args) {
		
		InvokeImpl(target, targetNetSync, repr.m_hash, DataWriter::SerializeLua(repr.m_types, sol::variadic_results(args.begin(), args.end())));
	}

	//template <typename... Args>
	//void InvokeView(Peer& peer, const ZDOID& targetZDO, const std::string& name, const Args&... params) {
	//	InvokeView(peer, targetZDO, VUtils::String::GetStableHashCode(name), params...);
	//}



	// Invoke a routed function bound to a peer
	template <typename... Args>
	void Invoke(OWNER_t target, HASH_t hash, const Args&... params) {
		InvokeView(target, ZDOID::NONE, hash, params...);
	}

	//template <typename... Args>
	//void Invoke(Peer& peer, HASH_t hash, const Args&... params) {
	//	InvokeView(peer, ZDOID::NONE, hash, params...);
	//}

	// Invoke a routed function bound to a peer
	template <typename... Args>
	void Invoke(OWNER_t target, const std::string& name, const Args&... params) {
		InvokeView(target, ZDOID::NONE, VUtils::String::GetStableHashCode(name), params...);
	}

	void InvokeLua(OWNER_t target, const IModManager::MethodSig& repr, const sol::variadic_args& args) {
		InvokeViewLua(target, ZDOID::NONE, repr, args);
	}

	//template <typename... Args>
	//void Invoke(Peer& peer, const std::string& name, const Args&... params) {
	//	InvokeView(peer, ZDOID::NONE, VUtils::String::GetStableHashCode(name), params);
	//}




	// Invoke a routed function targeted to all peers
	template <typename... Args>
	void InvokeAll(HASH_t hash, const Args&... params) {
		Invoke(EVERYBODY, hash, params...);
	}

	// Invoke a routed function targeted to all peers
	template <typename... Args>
	void InvokeAll(const std::string& name, const Args&... params) {
		Invoke(EVERYBODY, VUtils::String::GetStableHashCode(name), params...);
	}

	void InvokeAllLua(const IModManager::MethodSig& repr, const sol::variadic_args& args) {
		InvokeLua(EVERYBODY, repr, args);
	}

};

// Manager class for everything related to high-level networking for simulated p2p communication
IRouteManager* RouteManager();
