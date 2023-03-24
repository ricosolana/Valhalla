#pragma once 

#include <sol/sol.hpp>
#include <robin_hood.h>

#include "VUtils.h"
#include "VUtilsString.h"


//int GetCurrentLuaLine(lua_State* L);

class IModManager {
public:
    enum class Type {
        BOOL,

        STRING,
        STRINGS,

        BYTES,

        ZDOID,
        VECTOR3,
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
        DEFAULT = 0,
        CANCEL = 1 << 0,
        UNSUBSCRIBE = 1 << 1, // Set only when calling function self unsubscribes
    };

    using Types = std::vector<Type>;

    class MethodSig {
    public:
        HASH_t m_hash;
        Types m_types;
    };

    static constexpr HASH_t EVENT_RpcIn = __H("RpcIn");        // Server receives the lowest-layer RPC
    static constexpr HASH_t EVENT_RpcOut = __H("RpcOut");      // Server sends the lowest-layer RPC
    static constexpr HASH_t EVENT_RouteIn = __H("RouteIn");    // Server receives the middle-layer RoutedRPC
    static constexpr HASH_t EVENT_RouteOut = __H("RouteOut");   // Server receives the middle-layer RoutedRPC
    static constexpr HASH_t EVENT_Routed = __H("Routed");      // Server relays a message from peer to peer

    static constexpr HASH_t EVENT_Update = __H("Update");
    static constexpr HASH_t EVENT_Join = __H("Join");
    static constexpr HASH_t EVENT_Quit = __H("Quit");

    static constexpr HASH_t EVENT_POST = __H("POST");

private:
    struct Mod {
        std::string m_name;
        sol::environment m_env;

        fs::path m_entry;

        std::string m_version;
        std::string m_apiVersion;
        std::string m_description;
        std::list<std::string> m_authors;

        Mod(std::string name,
            fs::path entry) 
            : m_name(name),
            m_entry(entry) {}

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
    robin_hood::unordered_map<std::string, std::unique_ptr<Mod>> m_mods;
    robin_hood::unordered_map<HASH_t, std::list<EventHandle>> m_callbacks;

    EventStatus m_eventStatus = EventStatus::DEFAULT;

public:
    sol::state m_state;

private:
    std::unique_ptr<Mod> LoadModInfo(const std::string &folderName);

    void LoadAPI();
    void LoadMod(Mod& mod);

public:
    void Init();
    void Uninit();
    void Update();

    void recurse(sol::table table) {
        auto&& tostring = m_state["tostring"];

        for (int i = 0; i < table.size(); i++) {
            auto&& element = table[i];
            auto&& type = element.get_type();
            if (type == sol::type::table)
                recurse(element);
            else
                LOG(INFO) << tostring(table).get<std::string>();
        }
    };

    // Dispatch a Lua event
    //  Returns whether the event was requested for cancellation
    template <class... Args>
    bool CallEvent(HASH_t name, Args&&... params) {
        OPTICK_EVENT();

        auto&& find = m_callbacks.find(name);
        if (find != m_callbacks.end()) {
            auto&& callbacks = find->second;

            this->m_eventStatus = EventStatus::DEFAULT;

            for (auto&& itr = callbacks.begin(); itr != callbacks.end(); ) {
                try {
                    sol::protected_function_result result = itr->m_func(Args(params)...);
                    if (!result.valid()) {
                        LOG(ERROR) << "Event error: ";

                        //auto&& tostring = m_state["tostring"];



                        //LOG(ERROR) << tostring(result).get<std::string>();
                        //recurse(result);

                        sol::error error = result;
                        LOG(ERROR) << error.what();
                        m_eventStatus |= EventStatus::UNSUBSCRIBE;
                    }
                }
                catch (const std::exception& e) {
                    LOG(ERROR) << "Unhandled sol exception: " << e.what();
                    m_eventStatus |= EventStatus::UNSUBSCRIBE;
                }

                if ((m_eventStatus & EventStatus::UNSUBSCRIBE) == EventStatus::UNSUBSCRIBE) {
                    LOG(INFO) << "Unsubscribed event";
                    itr = callbacks.erase(itr);
                }
                else {
                    ++itr;
                }
            }
        }
        return (m_eventStatus & IModManager::EventStatus::CANCEL) == IModManager::EventStatus::CANCEL;
    }

    // Dispatch a Lua event
    //  Returns whether the event was requested for cancellation
    template <typename... Args>
    auto CallEvent(const std::string& name, Args&&... params) {
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
    auto CallEventTuple(const std::string& name, const Tuple& t) {
        return CallEventTupleImpl(VUtils::String::GetStableHashCode(name),
            t,
            std::make_index_sequence < std::tuple_size<Tuple>{} > {});
    }
};

// Manager class for everything related to mods which affect server functionality
IModManager* ModManager();
