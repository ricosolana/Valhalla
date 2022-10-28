#pragma once

#include <sol/sol.hpp>

#include "Utils.h"
#include "NetHashes.h"

class NetRpc;


class Mod {
public:
    const std::string m_name;

    sol::state m_state;

    Mod(const std::string& name)
            : m_name(name), m_state() {}
};

using ModCallback = std::pair<Mod*, std::pair<sol::function, int>>;

struct ModCallbacks {
    std::vector<ModCallback> m_onEnable;
    std::vector<ModCallback> m_onDisable;
    std::vector<ModCallback> m_onUpdate;
    std::vector<ModCallback> m_onPeerInfo;
    robin_hood::unordered_map<HASH_t, std::vector<ModCallback>> m_onRpc;
    robin_hood::unordered_map<HASH_t, std::vector<ModCallback>> m_onRoute;
    robin_hood::unordered_map<HASH_t, std::vector<ModCallback>> m_onSync;
    //robin_hood::unordered_map<HASH_t, std::vector<std::pair<sol::function, int>>> m_onRouteWatch;
    //robin_hood::unordered_map<HASH_t, std::vector<std::pair<sol::function, int>>> m_onSyncWatch;
};


namespace ModManager {
	void Init();
	void UnInit();

    ModCallbacks& getCallbacks();

	namespace Event {
		// Do not call externally
		void OnEnable();
		// Do not call externally
		void OnDisable();

		/// Event calls
		bool OnPeerInfo(NetRpc* rpc,
                        OWNER_t uuid,
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
