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

struct ModCallback {
    Mod* m_mod;
    sol::function m_func;
    int m_priority;
};

//enum class ModEventMode : uint8_t {
//    PRE = 0,
//    //TRANSPILE,
//    POST,
//    _MAX,
//};

//using ModCallback = std::pair<Mod*, std::pair<sol::function, int>>;

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



/*
void _EventConcat(std::string& result) {}

template<typename... STRING>
void _EventConcat(std::string& result, const std::string& s1, const STRING&... s2) {
    //result += std::to_string(Utils::GetStableHashCode(s1));
    _EventConcat(s2...);
}

template<typename... STRING>
std::string EventConcat(const std::string& s1, const STRING&... s2) {
    std::string result;
    _EventConcat(result, s1, s2...);
    return result;
}*/

//HASH_t EventConcat(const std::initializer_list<std::string>& strings);

//template<typename ...Types>

namespace ModManager {
	void Init();
	void UnInit();

    std::vector<ModCallback>& GetCallbacks(HASH_t hash);
    std::vector<ModCallback>& GetCallbacks(const char* name);



    template <typename... Args>
    sol::table CallEvent(HASH_t name, Args... params) {
        OPTICK_EVENT();
        auto&& callbacks = GetCallbacks(name);

        sol::table eventResults;
        for (auto&& callback : callbacks) {
            try {
                callback.m_func(eventResults, std::move(params)...);
            } catch (const sol::error& e) {
                LOG(ERROR) << e.what();
            }
        }
        return eventResults;
    }

    template <typename... Args>
    auto CallEvent(const std::string& name, Args... params) {
        return CallEvent(Utils::GetStableHashCode(name), std::move(params)...);
    }

    //template <typename... Args>
    //auto CallEvent(const std::initializer_list<std::string> &strings, Args... params) {
    //    std::string name;
    //    for (auto&& str : strings)
    //        name += Utils::GetStableHashCode(str);
    //
    //    return CallEvent(Utils::GetStableHashCode(name), std::move(params)...);
    //}

    template<class Tuple, size_t... Is>
    auto CallEventTupleImpl(HASH_t name, Tuple t, std::index_sequence<Is...>) {
        return CallEvent(name, std::move(std::get<Is>(t))...);
    }

    template <class Tuple>
    auto CallEventTuple(HASH_t name, Tuple t) {
        return CallEventTupleImpl(name, std::move(t),
                                  std::make_index_sequence < std::tuple_size<Tuple>{} > {});

        //auto seq = std::make_index_sequence<std::tuple_size<Tuple>{}>{};
        //return CallEvent(name, std::get<decltype(seq)>(std::move(t))...);
    }

    template <class Tuple>
    auto CallEventTuple(const std::string& name, Tuple t) {
        return CallEventTuple(Utils::GetStableHashCode(name), std::move(t));
    }
};
