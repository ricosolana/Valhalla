#pragma once 

#include <sol/sol.hpp>
#include <robin_hood.h>

#include "VUtilsString.h"
#include "VUtils.h"

enum class DataType {
    BYTES,
    STRING,
    ZDOID,
    VECTOR3,
    VECTOR2i,
    QUATERNION,
    STRINGS,
    BOOL,
    INT8,
    INT16,
    INT32,
    INT64,
    FLOAT,
    DOUBLE
};

enum class EventStatus {
    DEFAULT,
    PROCEED,
    CANCEL,
};

static constexpr HASH_t EVENT_HASH_RpcIn = __H("RpcIn");        // Server receives the lowest-layer RPC
static constexpr HASH_t EVENT_HASH_RpcOut = __H("RpcOut");      // Server sends the lowest-layer RPC
static constexpr HASH_t EVENT_HASH_RouteIn = __H("RouteIn");    // Server receives the middle-layer RoutedRPC
static constexpr HASH_t EVENT_HASH_RouteOut = __H("RouteOut");   // Server receives the middle-layer RoutedRPC
static constexpr HASH_t EVENT_HASH_Routed = __H("Routed");      // Server relays a message from peer to peer

static constexpr HASH_t EVENT_HASH_Update = __H("Update");      
static constexpr HASH_t EVENT_HASH_Join = __H("Join");
static constexpr HASH_t EVENT_HASH_Quit = __H("Quit");

static constexpr HASH_t EVENT_HASH_POST = __H("POST");

//int GetCurrentLuaLine(lua_State* L);

class IModManager {
    struct Mod {
        std::string m_name;
        sol::environment m_env;

        std::string m_version;
        std::string m_apiVersion;
        std::string m_description;
        std::list<std::string> m_authors;

        Mod(std::string name,
            sol::environment env) 
            : m_name(name), m_env(std::move(env)) {}

        void Error(const std::string& s) {
            LOG(ERROR) << "mod [" << m_name << "]: " << s << " (L" << GetCurrentLine() << ")";
        }

        int GetCurrentLine() {
            lua_Debug ar;
            lua_getstack(m_env.lua_state(), 1, &ar);
            lua_getinfo(m_env.lua_state(), "nSl", &ar);

            return ar.currentline;
        }
    };

    struct EventHandler {
        sol::protected_function m_func;
        int m_priority;

        EventHandler(sol::function func, int priority)
            : m_func(func), m_priority(priority) {}
    };

private:
    sol::state m_state;

    robin_hood::unordered_map<std::string, std::unique_ptr<Mod>> mods;
    robin_hood::unordered_map<HASH_t, std::vector<EventHandler>> m_callbacks;

    EventStatus m_eventStatus;

private:
    std::unique_ptr<Mod> LoadModInfo(const std::string &folderName, std::string& outEntry);

    void LoadAPI();
    void LoadMod(Mod* mod);

public:
    void Init();

    void Uninit();

    template <class... Args>
    auto CallEvent(HASH_t name, Args&&... params) {
        OPTICK_EVENT();

        auto&& find = m_callbacks.find(name);
        if (find != m_callbacks.end()) {
            auto&& callbacks = find->second;

            this->m_eventStatus = EventStatus::DEFAULT;

            // Multiple tables
            auto &&lutup = std::make_tuple(m_state.create_table_with("value", sol::make_reference(m_state, params))...);

            // Single table
            //auto&& lutup = std::make_tuple(m_state.create_table_with("value", sol::make_reference(m_state, params)...));

            for (auto&& callback : callbacks) {
                try {
                    //sol::protected_function_result result = std::apply(callback.m_func, t);
                    
                    // Pass each type in a proxy reference (will be variadic unpacked to function)
                    //sol::protected_function_result result = callback.m_func(m_state.create_table_with("value", sol::make_reference(m_state, params))...);

                    // Pass a single param to function with reference types
                    //sol::protected_function_result result = callback.m_func(m_state.create_table_with("value", sol::make_reference(m_state, params)...));

                    //sol::protected_function_result result = callback.m_func(m_state.create_table_with("value", sol::make_reference(m_state, params))...);

                    // TODO test whether the values in table are mutable (whether reassignments directly affect 'params' vararg)
                    sol::protected_function_result result = std::apply(callback.m_func, lutup);
                    if (!result.valid()) {
                        sol::error error = result;
                        LOG(ERROR) << error.what();
                    }
                }
                catch (const sol::error& e) {
                    LOG(ERROR) << e.what();
                }
            }
        }
        return m_eventStatus;
    }

    template <typename... Args>
    auto CallEvent(const std::string& name, Args&&... params) {
        return CallEvent(VUtils::String::GetStableHashCode(name), std::forward<Args>(params)...);
    }

private:
    template<class Tuple, size_t... Is>
    auto CallEventTupleImpl(HASH_t name, const Tuple& t, std::index_sequence<Is...>) {
        return CallEvent(name, std::get<Is>(t)...); // TODO use forward
    }

public:
    // Dispatch a event for capture by any registered mod event handlers
    // Returns whether the event-delegate is cancelled
    template <class Tuple>
    EventStatus CallEventTuple(HASH_t name, const Tuple& t) {
        return CallEventTupleImpl(name,
            t,
            std::make_index_sequence < std::tuple_size<Tuple>{} > {});
    }

    template <class Tuple>
    auto CallEventTuple(const std::string& name, const Tuple& t) {
        return CallEventTupleImpl(VUtils::String::GetStableHashCode(name),
            t,
            std::make_index_sequence < std::tuple_size<Tuple>{} > {});
    }
};

IModManager* ModManager();
