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
        NONE = 0,
        CANCEL = 1 << 0,
        UNSUBSCRIBE = 1 << 1, // Set only when calling function self unsubscribes
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

        // Socket methods events
        static constexpr HASH_t Send = __H("Send");
        static constexpr HASH_t Recv = __H("Recv");

        // Event postfix handler
        static constexpr HASH_t POSTFIX = __H("POST");
    };

    struct Mod {
        std::string m_name;
        sol::environment m_env;

        fs::path m_entry;

        std::string m_version;
        std::string m_apiVersion;
        std::string m_description;
        std::list<std::string> m_authors;

        bool m_reload = false;

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

    EventStatus m_eventStatus = EventStatus::NONE;

    bool m_reload = false;

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


    //bool IsEventCancelled();

    /*
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
    };*/

    // Dispatch a Lua event
    //  Returns false if the event requested cancellation
    template <class... Args>
    bool CallEvent(HASH_t name, Args&&... params) {
        OPTICK_EVENT();

        this->m_eventStatus = EventStatus::NONE;

        auto&& find = m_callbacks.find(name);
        if (find != m_callbacks.end()) {
            auto&& callbacks = find->second;

            for (auto&& itr = callbacks.begin(); itr != callbacks.end(); ) {
                bool err = false;
                try {
                    sol::protected_function_result result = itr->m_func(Args(params)...);
                    if (!result.valid()) {
                        LOG(ERROR) << "Event error: ";
                        //auto&& tostring = m_state["tostring"];

                        //LOG(ERROR) << tostring(result).get<std::string>();
                        //recurse(result);

                        err = true;

                        sol::error error = result;
                        LOG(ERROR) << error.what();
                        m_eventStatus |= EventStatus::UNSUBSCRIBE;
                    }
                    else {
                        // whether cancelled-events should follow Harmony prefix cancellation with bools
                        if (result.get_type() == sol::type::boolean) {
                            if (!result.get<bool>())
                                m_eventStatus |= EventStatus::CANCEL;
                        }
                    }
                }
                catch (const std::exception& e) {
                    LOG(ERROR) << "Unexpected sol exception: " << e.what();
                    m_eventStatus |= EventStatus::UNSUBSCRIBE;
                }

                if ((m_eventStatus & EventStatus::UNSUBSCRIBE) == EventStatus::UNSUBSCRIBE) {
                    if (err)
                        LOG(ERROR) << "Unsubscribed event";
                    else
                        LOG(INFO) << "Unsubscribed event";
                    itr = callbacks.erase(itr);
                }
                else {
                    ++itr;
                }
            }
        }
        return (m_eventStatus & IModManager::EventStatus::CANCEL) != IModManager::EventStatus::CANCEL;
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
