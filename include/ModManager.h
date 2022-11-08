#pragma once

#include <sol/sol.hpp>

#include "Utils.h"
#include "NetHashes.h"
#include "Vector.h"
#include "ChatManager.h"

class NetRpc;


class Mod {
public:
    const std::string m_name;
    const std::string m_version;
    const int m_apiVersion;

    sol::state m_state;

    Mod(const std::string& name, const std::string &version, int apiVersion)
            : m_name(name), m_version(version), m_apiVersion(apiVersion), m_state() {}
};

using ModCallback = std::pair<Mod*, std::pair<sol::function, int>>;

//struct ModCallbacks {
//    std::vector<ModCallback> m_onEnable;
//    std::vector<ModCallback> m_onDisable;
//    std::vector<ModCallback> m_onUpdate;
//    std::vector<ModCallback> m_onPeerInfo;
//    robin_hood::unordered_map<HASH_t, std::vector<ModCallback>> m_onRpc;
//    robin_hood::unordered_map<HASH_t, std::vector<ModCallback>> m_onRoute;
//    robin_hood::unordered_map<HASH_t, std::vector<ModCallback>> m_onSync;
//    robin_hood::unordered_map<HASH_t, std::vector<ModCallback>> m_onRouteWatch;
//    //robin_hood::unordered_map<HASH_t, std::vector<ModCallback>> m_onSyncWatch;
//};


namespace ModManager {
	void Init();
	void UnInit();

    std::vector<ModCallback>& GetCallbacks(HASH_t category, HASH_t name);
    std::vector<ModCallback>& GetCallbacks(const char* category, const char* name);

    template <typename... Args>
    sol::table CallEvent(HASH_t category, HASH_t name, Args... params) {
        OPTICK_EVENT();
        auto&& callbacks = GetCallbacks(category, name);

        sol::table eventResults;
        for (auto&& callback : callbacks) {
            try {
                callback.second.first(eventResults, std::move(params)...);
            } catch (const sol::error& e) {
                LOG(ERROR) << e.what();
            }
        }
        return eventResults;
    }

    template <typename... Args>
    auto CallEvent(const char* category, const char* name, Args... params) {
        return CallEvent(Utils::GetStableHashCode(category), Utils::GetStableHashCode(name), std::move(params)...);
    }

    template <class Tuple>
    auto CallEventTuple(HASH_t category, HASH_t name, Tuple t) {
        auto seq = std::make_index_sequence<std::tuple_size<Tuple>{}>{};
        return CallEvent(category, name, std::get<decltype(seq)>(t));
    }

    template <class Tuple>
    auto CallEventTuple(const char* category, const char* name, Tuple t) {
        return CallEvent(category, name, std::move(t));
    }
};
