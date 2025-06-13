#pragma once

#include "Method.h"
#include "ValhallaServer.h"
#include "DataStream.h"
#include "DataStream.h"
#include "ModManager.h"
#include "Hashes.h"
#include "NetManager.h"

class Peer;

class IRouteManager {
	friend class INetManager;

public:
	static constexpr std::int64_t EVERYBODY = 0;

private:	
	avledet::util::Map<avledet::util::Hash, std::unique_ptr<IMethod<Peer*>>> m_methods;

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
	void Register(avledet::util::Hash hash, F func) {
#if VH_IS_ON(VH_USE_MODS)
		m_methods[hash] = std::make_unique<MethodImpl<Peer*, F>>(func, IModManager::Events::RouteIn, hash);
#else
		m_methods[hash] = std::make_unique<MethodImpl<Peer*, F>>(func);
#endif
	}

	template<typename F>
	decltype(auto) Register(std::string_view name, F func) {
		return Register(avledet::util::get_stable_hash(name), func);
	}

#if VH_IS_ON(VH_USE_MODS)
	void RegisterLua(IModManager::MethodSig const& sig, sol::function const& func) {
		//VLOG(1) << "RegisterLua, func: " << sol::state_view(func.lua_state())["tostring"](func).get<std::string>() << ", hash: " << sig.m_hash;

		m_methods[sig.m_hash] = std::make_unique<MethodImplLua<Peer*>>(func, sig.m_types);
	}
#endif

	// Forwards raw data to peer(s) with no Lua handlers
	//void InvokeParams(avledet::util::UserID target, const ZDOID& targetZDO, avledet::util::Hash hash, avledet::util::Bytes params);


	// Invoke a routed function bound to a peer with sub zdo
	template <typename... Args>
	void InvokeView(avledet::util::UserID target, ZDOID const& targetZDO, avledet::util::Hash hash, Args&&... params) {
		// Prefix
		if ((std::int64_t)target == EVERYBODY) {
			// targetZDO can have a value apparently
			if (!VH_DISPATCH_MOD_EVENT(IModManager::Events::RouteOutAll ^ hash, targetZDO, params...))
				return;

			auto bytes = Serialize(VH_ID, target, targetZDO, hash, DataWriter::serialize(params...));

			for (auto&& peer : NetManager()->GetPeers()) {
				peer->Invoke(avledet::util::hashes::Rpc::RoutedRPC, bytes);
			}
		}
		else {
			if (auto peer = NetManager()->FindPeerByUserID(target)) {
				peer->RouteView(targetZDO, hash, std::forward<Args>(params)...);
			}
		}
	}

	// Invoke a routed function bound to a peer with sub zdo
	template <typename... Args>
	void InvokeView(avledet::util::UserID target, ZDOID const& targetZDO, std::string_view name, Args&&... params) {
		InvokeView(target, targetZDO, avledet::util::get_stable_hash(name), std::forward<Args>(params)...);
	}

#if VH_IS_ON(VH_USE_MODS)
	void InvokeViewLua(Int64Wrapper target, ZDOID const& targetZDO, IModManager::MethodSig const& repr, sol::variadic_args const& args) {		
		if ((std::int64_t)target == EVERYBODY) {
			if (args.size() != repr.m_types.size())
				throw std::runtime_error("mismatched number of args");

			auto results = sol::variadic_results(args.begin(), args.end());

#if VH_IS_ON(VH_REFLECTIVE_MOD_EVENTS)
			if (!VH_DISPATCH_MOD_EVENT(IModManager::Events::RouteOutAll ^ repr.m_hash, sol::as_args(results)))
				return;
#endif

			avledet::util::Writer writer;
			writer.write(repr.m_types, results);
			auto bytes = Serialize(VH_ID, (std::int64_t) target, targetZDO, repr.m_hash, std::move(writer.get_buf()));

			for (auto&& peer : NetManager()->GetPeers()) {
				peer->Invoke(avledet::util::hashes::Rpc::RoutedRPC, bytes);
			}
		}
		else {
			if (auto peer = NetManager()->FindPeerByUserID((std::int64_t)target))
				peer->RouteViewLua(targetZDO, repr, args);
		}
		
		//Serialize(VH_ID, target, targetZDO, repr.m_hash,
			//DataWriter::serializeLua(repr.m_types, sol::variadic_results(args.begin(), args.end())));
		
		//Invoke(target, targetZDO, repr.m_hash, DataWriter::serializeLua(repr.m_types, sol::variadic_results(args.begin(), args.end())));
	}
#endif



	// Invoke a routed function bound to a peer
	template <typename... Args>
	void Invoke(avledet::util::UserID target, avledet::util::Hash hash, Args&&... params) {
		InvokeView(target, ZDOID::NONE, hash, std::forward<Args>(params)...);
	}

	// Invoke a routed function bound to a peer
	template <typename... Args>
	void Invoke(avledet::util::UserID target, std::string_view name, Args&&... params) {
		InvokeView(target, ZDOID::NONE, avledet::util::get_stable_hash(name), std::forward<Args>(params)...);
	}

#if VH_IS_ON(VH_USE_MODS)
	void InvokeLua(Int64Wrapper target, IModManager::MethodSig const& repr, sol::variadic_args const& args) {
		InvokeViewLua(target, ZDOID::NONE, repr, args);
	}
#endif



	// Invoke a routed function targeted to all peers
	template <typename... Args>
	void InvokeAll(avledet::util::Hash hash, Args&&... params) {
		Invoke(EVERYBODY, hash, std::forward<Args>(params)...);
	}

	// Invoke a routed function targeted to all peers
	template <typename... Args>
	void InvokeAll(std::string_view name, Args&&... params) {
		Invoke(EVERYBODY, avledet::util::get_stable_hash(name), std::forward<Args>(params)...);
	}

#if VH_IS_ON(VH_USE_MODS)
	void InvokeAllLua(const IModManager::MethodSig& repr, sol::variadic_args const& args) {
		InvokeLua(EVERYBODY, repr, args);
	}
#endif

	avledet::util::Bytes Serialize(avledet::util::UserID sender, avledet::util::UserID target, ZDOID const& targetZDO, avledet::util::Hash hash, avledet::util::Bytes const& params) {
		DataWriter writer;

		writer.write((std::int64_t)0); // msg id
		writer.write(sender);
		writer.write(target);
		writer.write(targetZDO);
		writer.write(hash);
		writer.write(params);

		return writer.get_buf();
	}

};

// Manager class for everything related to high-level networking for simulated p2p communication
IRouteManager* RouteManager();
