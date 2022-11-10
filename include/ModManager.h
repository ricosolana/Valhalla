#pragma once

#include <sol/sol.hpp>

#include "Utils.h"
#include "NetHashes.h"
#include "Vector.h"
#include "ChatManager.h"

class NetRpc;

struct EventHandler;

class Mod {
public:
    const std::string m_name;
    const std::string m_version;
    const int m_apiVersion;

    sol::state m_state;

    //EventHandler *m_currentEvent;
    // replace the intended delegate call? (basically cancel it?)
    //bool m_cancelDelegate = false;

    Mod(const std::string& name, const std::string &version, int apiVersion)
            : m_name(name), m_version(version), m_apiVersion(apiVersion), m_state() {}
};

struct EventHandler {
    Mod* m_mod;
    sol::function m_func;
    int m_priority;
};

struct EventHandlerStream {
    std::vector<EventHandler> m_callbacks;
    bool m_cancel = false;
};

//enum class ModEventMode : uint8_t {
//    PRE = 0,
//    //TRANSPILE,
//    POST,
//    _MAX,
//};

//using EventHandler = std::pair<Mod*, std::pair<sol::function, int>>;

//struct EventHandlerStream {
//    std::vector<EventHandler> m_onEnable;
//    std::vector<EventHandler> m_onDisable;
//    std::vector<EventHandler> m_onUpdate;
//    std::vector<EventHandler> m_onPeerInfo;
//    robin_hood::unordered_map<HASH_t, std::vector<EventHandler>> m_onRpc;
//    robin_hood::unordered_map<HASH_t, std::vector<EventHandler>> m_onRoute;
//    robin_hood::unordered_map<HASH_t, std::vector<EventHandler>> m_onSync;
//    robin_hood::unordered_map<HASH_t, std::vector<EventHandler>> m_onRouteWatch;
//    //robin_hood::unordered_map<HASH_t, std::vector<EventHandler>> m_onSyncWatch;
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

// TODO event subscriber system?
// Make all Managers into objects
// What things should be namespaces?
// perhaps everything?
// Shouldnt be too verbose
// Should be trivial to use and recognizable
// Intended usages:
//  - Valhalla()->ModManager()->CallEvent(...)
//      MOD_EVENT(...) as a easy macro
//  - Valhalla()->NetManager()->...
//  - Valhalla()->
class ModManager {
public:
	void Init();
	void UnInit();

    std::vector<EventHandler>& GetCallbacks(HASH_t hash);
    std::vector<EventHandler>& GetCallbacks(const char* name);

    std::optional<EventHandlerStream> &CurrentEventStream();

    // get event results

    // Mod might be able to be hidden if var args can be constructed


    template <typename... Args>
    bool CallEvent(HASH_t name, Args... params) {
        OPTICK_EVENT();
        auto&& callbacks = GetCallbacks(name);

        auto &&opt = CurrentEventStream();
        opt =

        // have a method to get the status of the event results
        // instead of weirdly passing a temporary around
        //sol::table eventResults;
        for (auto&& callback : callbacks) {
            try {
                callback.m_func(std::move(params)...);
            } catch (const sol::error& e) {
                LOG(ERROR) << e.what();
            }
        }
        //return eventResults;
        return CurrentEventStream()
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
