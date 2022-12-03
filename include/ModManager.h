#pragma once

#include <sol/sol.hpp>
#include <utility>

#include "VUtils.h"
#include "NetHashes.h"
#include "Vector.h"
#include "ChatManager.h"
#include "VServer.h"

enum class PkgType {
    BYTE_ARRAY,
    PKG,
    STRING,
    NET_ID,
    VECTOR3,
    VECTOR2i,
    QUATERNION,
    STRING_ARRAY,
    BOOL,
    INT8, UINT8, //BOOL, CHAR, UCHAR,
    INT16, UINT16, //SHORT, USHORT,
    INT32, UINT32, //INT, UINT,
    INT64, UINT64, //LONG, ULONG, OWNER_t,
    FLOAT,
    DOUBLE
};

enum class EventStatus {
    DEFAULT,
    PROCEED,
    CANCEL,
};

static constexpr HASH_t EVENT_HASH_RpcIn = __H("RpcIn");
static constexpr HASH_t EVENT_HASH_RpcOut = __H("RpcOut");
static constexpr HASH_t EVENT_HASH_RouteIn = __H("RouteIn");
static constexpr HASH_t EVENT_HASH_RconIn = __H("RconIn");
static constexpr HASH_t EVENT_HASH_Update = __H("Update");
static constexpr HASH_t EVENT_HASH_RconConnect = __H("RconConnect");
static constexpr HASH_t EVENT_HASH_RconDisconnect = __H("RconDisconnect");
static constexpr HASH_t EVENT_HASH_PeerConnect = __H("PeerConnect");
static constexpr HASH_t EVENT_HASH_PeerQuit = __H("PeerQuit");

static constexpr HASH_t EVENT_HASH_POST = VUtils::String::GetStableHashCode("POST");

int GetCurrentLuaLine(lua_State* L);

class ModManager {
    friend VServer;

    class Mod {
    public:
        const std::string m_name;
        const std::string m_version;
        const int m_apiVersion;

        sol::state m_state;

        //void Throw(const std::string& msg);

        void Throw(const char* msg);

        Mod(std::string name, std::string version, int apiVersion)
                : m_name(std::move(name)), m_version(std::move(version)), m_apiVersion(apiVersion), m_state() {}
    };

    struct EventHandler {
        Mod* m_mod;
        sol::function m_func;
        int m_priority;
    };

private:
    static robin_hood::unordered_map<std::string, std::unique_ptr<Mod>> mods;
    static robin_hood::unordered_map<HASH_t, std::vector<EventHandler>> m_callbacks;

    static EventStatus m_eventStatus; // = EventStatus::DEFAULT;

private:
    static void Init();
    static void UnInit();

    static void RunModInfoFrom(const std::string& dirname,
                        std::string& outName,
                        std::string& outVersion,
                        int &outApiVersion,
                        std::string& outEntry);

    static std::unique_ptr<Mod> PrepareModEnvironment(const std::string& name,
                                               const std::string& version,
                                               int apiVersion);

    static bool EventHandlerSort(const EventHandler &a,
                                 const EventHandler &b);

public:
    // Dispatch a event for capture by any registered mod event handlers
    // Returns whether the event-delegate is cancelled
    template <typename... Args>
    static EventStatus CallEvent(HASH_t name, const Args&... params) {
        OPTICK_EVENT();

        auto&& find = m_callbacks.find(name);
        if (find != m_callbacks.end()) {
            auto &&callbacks = find->second;

            this->m_eventStatus = EventStatus::DEFAULT;

            for (auto &&callback : callbacks) {
                try {
                    //auto dbg(GetDebugInfo(callback))
                    callback.m_func(params...);
                } catch (const sol::error &e) {
                    LOG(ERROR) << e.what();
                }
            }
        }
        return m_eventStatus;
    }

    template <typename... Args>
    static auto CallEvent(const char* name, const Args&... params) {
        return CallEvent(VUtils::String::GetStableHashCode(name), params...);
    }

    template <typename... Args>
    static auto CallEvent(std::string& name, const Args&... params) {
        return CallEvent(name.c_str(), params...);
    }



private:
    template<class Tuple, size_t... Is>
    static auto CallEventTupleImpl(HASH_t name, const Tuple& t, std::index_sequence<Is...>) {
        return CallEvent(name, std::get<Is>(t)...);
    }

public:
    template <class Tuple>
    static auto CallEventTuple(HASH_t name, const Tuple& t) {
        return CallEventTupleImpl(name, t,
                                  std::make_index_sequence < std::tuple_size<Tuple>{} > {});
    }

    template <class Tuple>
    static auto CallEventTuple(const char* name, const Tuple& t) {
        return CallEventTuple(VUtils::String::GetStableHashCode(name), t);
    }

    template <class Tuple>
    static auto CallEventTuple(std::string& name, const Tuple& t) {
        return CallEventTuple(name.c_str(), t);
    }
};

//VModManager* ModManager();

#define CALL_EVENT(name, ...) ModManager::CallEvent(name, ##__VA_ARGS__)
#define CALL_EVENT_TUPLE(name, ...) ModManager::CallEventTuple(name, ##__VA_ARGS__)
