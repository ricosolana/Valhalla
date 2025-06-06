#pragma once 

#include "VUtils.h"
#include "VUtilsString.h"
#include "HashUtils.h"

#if VH_IS_ON(VH_USE_MODS)

#include <sol/sol.hpp>

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
        avledet::util::Hash m_hash;
        Types m_types;
    };

    class Events {
    public:
        // Game state events
        static constexpr avledet::util::Hash Enable = __H("Enable");
        static constexpr avledet::util::Hash Disable = __H("Disable");
        static constexpr avledet::util::Hash Update = __H("Update");
        static constexpr avledet::util::Hash PeriodicUpdate = __H("Periodic");

        // Connecting peer events
        static constexpr avledet::util::Hash Connect = __H("Connect");
        static constexpr avledet::util::Hash Disconnect = __H("Disconnect");

        // Connected peer events
        static constexpr avledet::util::Hash Join = __H("Join");
        static constexpr avledet::util::Hash Quit = __H("Quit");

        // Rpc events (some unimplemented)
        static constexpr avledet::util::Hash RpcIn = __H("RpcIn");       // Peer -> Server
        static constexpr avledet::util::Hash RpcOut = __H("RpcOut");     // Peer -> Server
        
        // Routed events (this is complicated)
        static constexpr avledet::util::Hash RouteIn = __H("RouteIn");           // Peer -> Server
        static constexpr avledet::util::Hash RouteInAll = __H("RouteInAll");     // Peer -> Server
        static constexpr avledet::util::Hash RouteOut = __H("RouteOut");         // Server -> Peer
        static constexpr avledet::util::Hash RouteOutAll = __H("RouteOutAll");   // Server -> Peer
        static constexpr avledet::util::Hash Routed = __H("Routed");             // Peer -> Server -> Peer

        // General game events
        static constexpr avledet::util::Hash PlayerList = __H("PlayerList");

        static constexpr avledet::util::Hash ZDOUnpacked = __H("ZDOUnpacked");
        static constexpr avledet::util::Hash ZDOCreated = __H("ZDOCreated");
        static constexpr avledet::util::Hash ZDOModified = __H("ZDOModified");
        static constexpr avledet::util::Hash SendingZDO = __H("SendingZDO");
        //static constexpr avledet::util::Hash ZDODestroyed = __H("ZDODestroyed");

        // Socket methods events
        static constexpr avledet::util::Hash Send = __H("Send");
        static constexpr avledet::util::Hash Recv = __H("Recv");

        // Event postfix handler
        //static constexpr avledet::util::Hash POSTFIX = __H("POST");
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
    avledet::util::Map<std::string, std::unique_ptr<Mod>, ankerl::unordered_dense::string_hash, std::equal_to<>> m_mods;
    avledet::util::Map<avledet::util::Hash, std::list<EventHandle>> m_callbacks;

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
    bool CallEvent(avledet::util::Hash name, Args&&... params) {
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
    template<class Tuple, std::size_t... Is>
    auto CallEventTupleImpl(avledet::util::Hash name, const Tuple& t, std::index_sequence<Is...>) {
        return CallEvent(name, std::get<Is>(t)...); // TODO use forward
    }

public:
    // Dispatch a Lua event
    //  Returns whether the event was requested for cancellation
    template <class Tuple>
    auto CallEventTuple(avledet::util::Hash name, const Tuple& t) {
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

#define VH_DISPATCH_MOD_EVENT(name, ...) ModManager()->CallEvent((name), __VA_ARGS__)
#define VH_DISPATCH_MOD_EVENT_TUPLE(name, ...) ModManager()->CallEventTuple((name), __VA_ARGS__)

// Manager class for everything related to mods which affect server functionality
IModManager* ModManager();

#else // !VH_USE_MODS
#define VH_DISPATCH_MOD_EVENT(name, ...) true
#define VH_DISPATCH_MOD_EVENT_TUPLE(name, ...) true
#endif // VH_USE_MODS