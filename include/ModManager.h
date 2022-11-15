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

static constexpr bool EVENT_PROCEED = false;
static constexpr bool EVENT_CANCEL = true;

static constexpr HASH_t EVENT_HASH_RpcIn = __H("RpcIn");
static constexpr HASH_t EVENT_HASH_RpcOut = __H("RpcOut");
static constexpr HASH_t EVENT_HASH_RouteIn = __H("RouteIn");
static constexpr HASH_t EVENT_HASH_RconIn = __H("RconIn");

static constexpr HASH_t EVENT_HASH_POST = VUtils::String::GetStableHashCode("POST");

class VModManager {
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
    robin_hood::unordered_map<std::string, std::unique_ptr<Mod>> mods;
    robin_hood::unordered_map<HASH_t, std::vector<EventHandler>> m_callbacks;

    bool m_eventStatus = EVENT_PROCEED;

private:
    void Init();
    void UnInit();

    void RunModInfoFrom(const std::string& dirname,
                        std::string& outName,
                        std::string& outVersion,
                        int &outApiVersion,
                        std::string& outEntry);

    std::unique_ptr<Mod> PrepareModEnvironment(const std::string& name,
                                               const std::string& version,
                                               int apiVersion);

    static bool EventHandlerSort(const EventHandler &a,
                                 const EventHandler &b);

public:
    // Dispatch a event for capture by any registered mod event handlers
    // Returns whether the event-delegate is cancelled
    template <typename... Args>
    bool CallEvent(HASH_t name, const Args&... params) {
        OPTICK_EVENT();
        auto&& find = m_callbacks.find(name);
        if (find == m_callbacks.end())
            return false;

        auto &&callbacks = find->second;

        this->m_eventStatus = EVENT_PROCEED;
        for (auto&& callback : callbacks) {
            try {
                callback.m_func(params...);
            } catch (const sol::error& e) {
                LOG(ERROR) << e.what();
            }
        }
        return m_eventStatus;
    }

    template <typename... Args>
    auto CallEvent(const char* name, const Args&... params) {
        return CallEvent(VUtils::String::GetStableHashCode(name), params...);
    }

    template <typename... Args>
    auto CallEvent(std::string& name, const Args&... params) {
        return CallEvent(name.c_str(), params...);
    }



private:
    template<class Tuple, size_t... Is>
    auto CallEventTupleImpl(HASH_t name, const Tuple& t, std::index_sequence<Is...>) {
        return CallEvent(name, std::get<Is>(t)...);
    }

public:
    template <class Tuple>
    auto CallEventTuple(HASH_t name, const Tuple& t) {
        return CallEventTupleImpl(name, t,
                                  std::make_index_sequence < std::tuple_size<Tuple>{} > {});
    }

    template <class Tuple>
    auto CallEventTuple(const char* name, const Tuple& t) {
        return CallEventTuple(VUtils::String::GetStableHashCode(name), t);
    }

    template <class Tuple>
    auto CallEventTuple(std::string& name, const Tuple& t) {
        return CallEventTuple(name.c_str(), t);
    }
};

VModManager* ModManager();

#define CALL_EVENT(name, ...) ModManager()->CallEvent(name, ##__VA_ARGS__)
#define CALL_EVENT_TUPLE(name, ...) ModManager()->CallEventTuple(name, ##__VA_ARGS__)
