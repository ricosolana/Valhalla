#pragma once 

#include <sol/sol.hpp>
#include <robin_hood.h>

#include "VUtils.h"
#include "VUtilsString.h"

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
    UNSUBSCRIBE, // Set only when calling function self unsubscribes
};

struct MethodSig {
    //std::string m_name;
    HASH_t m_hash;
    std::vector<DataType> m_types;
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

        fs::path m_entry;
        //bool m_reload;
        //fs::file_time_type m_lastModified;

        std::string m_version;
        std::string m_apiVersion;
        std::string m_description;
        std::list<std::string> m_authors;

        Mod(std::string name,
            //sol::environment env,
            fs::path entry) 
            : m_name(name),// m_env(std::move(env)), 
            m_entry(entry)/*, m_lastModified(fs::last_write_time(entry))*/ {}

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

    struct EventHandle {
        std::reference_wrapper<Mod> m_mod;
        sol::protected_function m_func;
        int m_priority;

        EventHandle(Mod& mod, sol::function func, int priority)
            : m_mod(mod), m_func(func), m_priority(priority) {}
    };

private:
    sol::state m_state;

    robin_hood::unordered_map<std::string, std::unique_ptr<Mod>> m_mods;
    robin_hood::unordered_map<HASH_t, std::list<EventHandle>> m_callbacks;

    //EventHandle* m_currentCallback = nullptr;

    EventStatus m_eventStatus = EventStatus::DEFAULT;

private:
    std::unique_ptr<Mod> LoadModInfo(const std::string &folderName);

    void LoadAPI();
    void LoadMod(Mod& mod);

public:
    void Init();
    void Uninit();
    void Update();

    template <class... Args>
    auto CallEvent(HASH_t name, Args&&... params) {
        OPTICK_EVENT();

        auto&& find = m_callbacks.find(name);
        if (find != m_callbacks.end()) {
            auto&& callbacks = find->second;

            this->m_eventStatus = EventStatus::DEFAULT;

            // Multiple tables
            // TODO it turns out that strings are weird (i think), 
            //  go back to the old method... this will break some functionality from past
            //auto &&lutup = std::make_tuple(m_state.create_table_with("value", sol::make_object(m_state, params))...);

            // Single table
            //auto&& lutup = std::make_tuple(m_state.create_table_with("value", sol::make_reference(m_state, params)...));

            for (auto&& itr = callbacks.begin(); itr != callbacks.end(); ) {
            //for (auto&& callback : callbacks) {
                //try {
                    //sol::protected_function_result result = std::apply(itr->m_func, params);
                
                //m_currentCallback = itr->get();

                //sol::protected_function_result result = (*itr)(params);
                sol::protected_function_result result = itr->m_func(params...);
                    
                    // Pass each type in a proxy reference (will be variadic unpacked to function)
                    //sol::protected_function_result result = callback.m_func(m_state.create_table_with("value", sol::make_reference(m_state, params))...);

                    // Pass a single param to function with reference types
                    //sol::protected_function_result result = callback.m_func(m_state.create_table_with("value", sol::make_reference(m_state, params)...));

                    //sol::protected_function_result result = callback.m_func(m_state.create_table_with("value", sol::make_reference(m_state, params))...);

                    // TODO test whether the values in table are mutable (whether reassignments directly affect 'params' vararg)

                //sol::protected_function_result result = std::apply(itr->m_func, lutup);
                if (!result.valid()) {
                    LOG(ERROR) << "Callback error: ";

                    sol::error error = result;
                    LOG(ERROR) << error.what();

                    LOG(ERROR) << "Callback disabling...";
                    itr = callbacks.erase(itr);
                }
                else if (m_eventStatus == EventStatus::UNSUBSCRIBE) {
                    LOG(INFO) << "Event unsubscribed";
                    itr = callbacks.erase(itr);
                } else {
                    ++itr;
                }
                //}
                //catch (const sol::error& e) {
                //    LOG(ERROR) << e.what();
                //}
            }
            //m_currentCallback = nullptr;
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

// Manager class for everything related to mods which affect server functionality
IModManager* ModManager();
