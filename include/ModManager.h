#pragma once 

#include <sol/sol.hpp>

#include "VUtils.h"
#include "VUtilsString.h"
#include "HashUtils.h"

//int GetCurrentLuaLine(lua_State* L);

class IModManager {
public:
    enum class Type {
        BOOL,

        STRING,
        STRINGS,

        BYTES,

        ZDOID,
        VECTOR3f,
        VECTOR2i,
        QUATERNION,

        INT8,
        INT16,
        INT32,
        INT64,

        UINT8,
        UINT16,
        UINT32,
        UINT64,

        FLOAT,
        DOUBLE,

        CHAR, // utf8

        max
    };

    enum class EventStatus {
        NONE,
        UNSUBSCRIBE, // Set only when calling function self unsubscribes
    };

    using Types = std::vector<Type>;

    class MethodSig {
    public:
        HASH_t m_hash;
        Types m_types;
    };

    class Events {
    public:
        // Game state events
        static constexpr HASH_t Enable = __H("Enable");
        static constexpr HASH_t Disable = __H("Disable");
        static constexpr HASH_t Update = __H("Update");
        static constexpr HASH_t PeriodicUpdate = __H("Periodic");

        // Connecting peer events
        static constexpr HASH_t Connect = __H("Connect");
        static constexpr HASH_t Disconnect = __H("Disconnect");

        // Connected peer events
        static constexpr HASH_t Join = __H("Join");
        static constexpr HASH_t Quit = __H("Quit");

        // Rpc events (some unimplemented)
        static constexpr HASH_t RpcIn = __H("RpcIn");       // Peer -> Server
        static constexpr HASH_t RpcOut = __H("RpcOut");     // Peer -> Server
        
        // Routed events (this is complicated)
        static constexpr HASH_t RouteIn = __H("RouteIn");           // Peer -> Server
        static constexpr HASH_t RouteInAll = __H("RouteInAll");     // Peer -> Server
        static constexpr HASH_t RouteOut = __H("RouteOut");         // Server -> Peer
        static constexpr HASH_t RouteOutAll = __H("RouteOutAll");   // Server -> Peer
        static constexpr HASH_t Routed = __H("Routed");             // Peer -> Server -> Peer

        // General game events
        static constexpr HASH_t PlayerList = __H("PlayerList");

        static constexpr HASH_t ZDOCreated = __H("ZDOCreated");
        static constexpr HASH_t ZDOModified = __H("ZDOModified");
        static constexpr HASH_t SendingZDO = __H("SendingZDO");
        //static constexpr HASH_t ZDODestroyed = __H("ZDODestroyed");

        // Socket methods events
        static constexpr HASH_t Send = __H("Send");
        static constexpr HASH_t Recv = __H("Recv");

        // Event postfix handler
        //static constexpr HASH_t POSTFIX = __H("POST");
    };

    struct Mod {
        std::string m_name;

        fs::path m_entry;

        std::string m_version;
        std::string m_apiVersion;
        std::string m_description;
        std::list<std::string> m_authors;

        Mod(std::string name,
            fs::path entry) 
            : m_name(name),
            m_entry(entry) {}

    };

    struct EventHandle {
        sol::protected_function m_func;
        int m_priority;

        EventHandle(sol::function func, int priority)
            : m_func(func), m_priority(priority) {}
    };

private:
    UNORDERED_MAP_t<std::string, std::unique_ptr<Mod>, ankerl::unordered_dense::string_hash, std::equal_to<>> m_mods;
    UNORDERED_MAP_t<HASH_t, std::list<EventHandle>> m_callbacks;

    bool m_unsubscribeCurrentEvent;

public:
    sol::state m_state;

private:
    Mod& LoadModInfo(std::string_view folderName);

    void LoadAPI();
    void LoadMod(Mod& mod);

public:
    void PostInit();
    void Uninit();

    // Dispatch a Lua event
    //  Returns false if the event requested cancellation
    template <class... Args>
    bool CallEvent(HASH_t name, Args&&... params) {
        ZoneScoped;

        this->m_unsubscribeCurrentEvent = false;

        auto&& find = m_callbacks.find(name);
        if (find != m_callbacks.end()) {
            auto&& callbacks = find->second;

            for (auto&& itr = callbacks.begin(); itr != callbacks.end(); ) {
                sol::protected_function_result result = itr->m_func(Args(params)...);
                if (!result.valid()) {
                    LOG_WARNING(LOGGER, "Event error: ");

                    sol::error error = result;
                    LOG_ERROR(LOGGER, "{}", error.what());
                    this->m_unsubscribeCurrentEvent = true;
                }
                else {
                    // whether cancelled-events should follow Harmony prefix cancellation with bools
                    if (result.get_type() == sol::type::boolean) {
                        if (!result.get<bool>())
                            return false;
                    }
                }

                if (this->m_unsubscribeCurrentEvent) {
                    itr = callbacks.erase(itr);
                }
                else {
                    ++itr;
                }
            }
        }

        return true;
    }

    // Dispatch a Lua event
    //  Returns whether the event was requested for cancellation
    template <typename... Args>
    auto CallEvent(std::string_view name, Args&&... params) {
        return CallEvent(VUtils::String::GetStableHashCode(name), std::forward<Args>(params)...);
    }

private:
    // Dispatch a Lua event
    //  Returns whether the event was requested for cancellation
    template<class Tuple, size_t... Is>
    auto CallEventTupleImpl(HASH_t name, const Tuple& t, std::index_sequence<Is...>) {
        return CallEvent(name, std::get<Is>(t)...); // TODO use forward
    }

public:
    // Dispatch a Lua event
    //  Returns whether the event was requested for cancellation
    template <class Tuple>
    auto CallEventTuple(HASH_t name, const Tuple& t) {
        return CallEventTupleImpl(name,
            t,
            std::make_index_sequence < std::tuple_size<Tuple>{} > {});
    }

    // Dispatch a Lua event
    //  Returns whether the event was requested for cancellation
    template <class Tuple>
    auto CallEventTuple(std::string_view name, const Tuple& t) {
        return CallEventTupleImpl(VUtils::String::GetStableHashCode(name),
            t,
            std::make_index_sequence < std::tuple_size<Tuple>{} > {});
    }
};

#ifdef VH_OPTION_ENABLE_MODS
#define VH_DISPATCH_MOD_EVENT(name, ...) ModManager()->CallEvent((name), __VA_ARGS__)
#define VH_DISPATCH_MOD_EVENT_TUPLE(name, ...) ModManager()->CallEventTuple((name), __VA_ARGS__)
#else
#define VH_DISPATCH_MOD_EVENT(name, ...) true
#define VH_DISPATCH_MOD_EVENT_TUPLE(name, ...) true
#endif

// Manager class for everything related to mods which affect server functionality
IModManager* ModManager();
