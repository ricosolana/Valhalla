#pragma once

#include "Method.h"
#include "ValhallaServer.h"
#include "DataReader.h"
#include "DataWriter.h"
#include "ModManager.h"
#include "RouteData.h"

class Peer;

class IRouteManager {
	friend class INetManager;

public:
	static constexpr OWNER_t EVERYBODY = 0;

private:	
	robin_hood::unordered_map<HASH_t, std::unique_ptr<IMethod<Peer*>>> m_methods;

private:
	// Called from NetManager
	void OnNewPeer(Peer &peer);

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
	void Invoke(HASH_t hash, Args&&... params) {
		// Prefix
		//if (target == EVERYBODY) {
			if (!ModManager()->CallEvent(IModManager::Events::RouteOutAll ^ hash, targetZDO, params))
				return;

			BYTES_t bytes;
			DataWriter writer(bytes);

			writer.Write<int64_t>(0); // msg id
			writer.Write(SERVER_ID);
			writer.Write(target);
			writer.Write(ZDOID::NONE);
			writer.Write(hash);
			writer.Write(DataWriter::Serialize(params...));

			for (auto&& peer : peers) {
				peer->Invoke(Hashes::Rpc::RoutedRPC, bytes);
			}
		//}
		//else {
		//	if (auto peer = NetManager()->GetPeer(target))
		//		peer->Route(targetZDO, hash, std::forward<Args>(params)...);
		//}
	}

	// Invoke a routed function bound to a peer with sub zdo
	template <typename... Args>
	void InvokeView(OWNER_t target, const ZDOID& targetZDO, const std::string& name, Args&&... params) {
		InvokeView(target, targetZDO, VUtils::String::GetStableHashCode(name), std::forward<Args>(params)...);
	}

	void InvokeViewLua(OWNER_t target, const ZDOID& targetZDO, const IModManager::MethodSig& repr, const sol::variadic_args& args) {
		
		
		InvokeImpl(target, targetZDO, repr.m_hash, DataWriter::SerializeLua(repr.m_types, sol::variadic_results(args.begin(), args.end())));
	}



	// Invoke a routed function bound to a peer
	template <typename... Args>
	void Invoke(OWNER_t target, HASH_t hash, Args&&... params) {
		InvokeView(target, ZDOID::NONE, hash, std::forward<Args>(params)...);
	}

	// Invoke a routed function bound to a peer
	template <typename... Args>
	void Invoke(OWNER_t target, const std::string& name, Args&&... params) {
		InvokeView(target, ZDOID::NONE, VUtils::String::GetStableHashCode(name), std::forward<Args>(params)...);
	}

	void InvokeLua(OWNER_t target, const IModManager::MethodSig& repr, const sol::variadic_args& args) {
		InvokeViewLua(target, ZDOID::NONE, repr, args);
	}



	// Invoke a routed function targeted to all peers
	template <typename... Args>
	void InvokeAll(HASH_t hash, Args&&... params) {
		Invoke(EVERYBODY, hash, std::forward<Args>(params)...);
	}

	// Invoke a routed function targeted to all peers
	template <typename... Args>
	void InvokeAll(const std::string& name, Args&&... params) {
		Invoke(EVERYBODY, VUtils::String::GetStableHashCode(name), std::forward<Args>(params)...);
	}

	void InvokeAllLua(const IModManager::MethodSig& repr, const sol::variadic_args& args) {
		InvokeLua(EVERYBODY, repr, args);
	}

};

// Manager class for everything related to high-level networking for simulated p2p communication
IRouteManager* RouteManager();
