#pragma once

#include <sol/sol.hpp>

#include "Utils.h"
#include "NetHashes.h"
//#include "NetRpc.h"

class NetRpc;

class Mod {
public:
	// required attributes
	const std::string m_name;
	const int m_priority;

	// lua
	sol::state m_state;

	const std::function<void()> m_onEnable;
	const std::function<void()> m_onDisable;
	const std::function<void(float)> m_onUpdate;
	const std::function<bool(NetRpc*, UUID_t, std::string, std::string)> m_onPeerInfo; // onHandshake; //onNewConnection;

	robin_hood::unordered_map<HASH_t, sol::function> m_rpcCallbacks;
	robin_hood::unordered_map<HASH_t, sol::function> m_routeCallbacks;
	robin_hood::unordered_map<HASH_t, sol::function> m_syncCallbacks;

	//Mod(const std::string &name,
	//	int priority,
	//	sol::state state,
	//	const std::function<void()> &onEnable,
	//	const std::function<void()> &onDisable,
	//	const std::function<void(float)> &onUpdate,
	//	const std::function<bool(NetRpc*, UUID_t, std::string, std::string)> &onPeerInfo)
	//	:	m_name(name), m_priority(priority), m_state(std::move(state)), 
	//		m_onEnable(onEnable), m_onDisable(onDisable), m_onUpdate(onUpdate), m_onPeerInfo(onPeerInfo) {}

	Mod(const std::string& name,
		int priority,
		sol::state state)
		: m_name(name), m_priority(priority), m_state(std::move(state)),
		m_onEnable(m_state["onEnable"].get_or(std::function<void()>())),
		m_onDisable(m_state["onDisable"].get_or(std::function<void()>())),
		m_onUpdate(m_state["onUpdate"].get_or(std::function<void(float)>())),
		m_onPeerInfo(m_state["onPeerInfo"].get_or(std::function<bool(NetRpc*, UUID_t, std::string, std::string)>())) {}

};

namespace ModManager {
	void Init();
	void Uninit();

	std::vector<Mod*>& GetMods();

	namespace Event {
		// Do not call externally
		void OnEnable();
		// Do not call externally
		void OnDisable();

		/// Event calls
		bool OnPeerInfo(NetRpc* rpc, 
			UUID_t uuid, 
			const std::string& name, 
			const std::string& version);
		void OnUpdate(float delta);

		//template <class Tuple>
		//constexpr void OnRpcInvoke(Rpc_Hash hash, Tuple& t) {
		//
		//}

		//void OnRpcCallback(Rpc_Hash hash, sol::variadic_args args);
	}
};
