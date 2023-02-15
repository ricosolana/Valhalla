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

static constexpr HASH_t EVENT_HASH_RpcIn = __H("RpcIn");
static constexpr HASH_t EVENT_HASH_RpcOut = __H("RpcOut");
static constexpr HASH_t EVENT_HASH_RouteIn = __H("RouteIn");
static constexpr HASH_t EVENT_HASH_Update = __H("Update");
static constexpr HASH_t EVENT_HASH_Join = __H("Join");
static constexpr HASH_t EVENT_HASH_Quit = __H("Quit");

static constexpr HASH_t EVENT_HASH_POST = VUtils::String::GetStableHashCode("POST");

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

    template <class Tuple>
    auto CallEventTuple(HASH_t name, Tuple&& t) {
        OPTICK_EVENT();

        auto&& find = m_callbacks.find(name);
        if (find != m_callbacks.end()) {
            auto&& callbacks = find->second;

            this->m_eventStatus = EventStatus::DEFAULT;

            for (auto&& callback : callbacks) {
                try {
                    sol::protected_function_result result = std::apply(callback.m_func, t);
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

    template <class Tuple>
    auto CallEventTuple(const std::string& name, Tuple&& t) {
        return CallEventTuple(VUtils::String::GetStableHashCode(name), std::forward<Tuple>(t));
    }

    // Dispatch a event for capture by any registered mod event handlers
    // Returns whether the event-delegate is cancelled
    template <typename... Args>
    EventStatus CallEvent(HASH_t name, Args&&... params) {
        return CallEventTuple(name, std::forward_as_tuple(params...));
    }

    template <typename... Args>
    auto CallEvent(const std::string& name, Args&&... params) {
        return CallEventTuple(name, std::forward_as_tuple(params...));
    }
};

IModManager* ModManager();
