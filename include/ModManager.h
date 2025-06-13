#pragma once 

#include "CompileSettings.h"

#if VH_IS_ON(VH_USE_MODS)

#include <cmath>
#include <cstdint>
#include <vector>
#include <list>
#include <magic_enum.hpp>
#include <sol/sol.hpp>
#include <lua.h>
#include "Hashes.h"
#include "DataStream.h"
#include "Quaternion.h"
#include "Types.h"
#include "VUtils.h"
#include "Vector.h"
#include "ZDOID.h"
#include "ValhallaServer.h"


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

        CHAR16, // utf8

        max
    };

    //enum class EventStatus {
    //    NONE,
    //    UNSUBSCRIBE, // Set only when calling function self unsubscribes
    //};

    using Types = std::vector<Type>;

    class MethodSig {
    public:
        Types m_types;
        avledet::util::Hash m_hash;

        MethodSig(Types types, avledet::util::Hash hash)
            : m_types(std::move(types)), m_hash(hash) {}

        MethodSig(std::string_view name, sol::variadic_args types)
            : m_types(types.begin(), types.end()), m_hash(avledet::util::get_stable_hash(name)) {}
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
                    LOG_WARNING(VH_LOGGER, "Event error: ");

                    sol::error error = result;
                    LOG_ERROR(VH_LOGGER, "{}", error.what());
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
        return CallEvent(avledet::util::get_stable_hash(name), std::forward<Args>(params)...);
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
        return CallEventTupleImpl(avledet::util::get_stable_hash(name),
            t,
            std::make_index_sequence < std::tuple_size<Tuple>{} > {});
    }
};

#define VH_DISPATCH_MOD_EVENT(name, ...) \
    ModManager()->CallEvent((name) __VA_OPT__(,) __VA_ARGS__)
#define VH_DISPATCH_MOD_EVENT_TUPLE(name, ...) \
    ModManager()->CallEventTuple((name) __VA_OPT__(,) __VA_ARGS__)

// Manager class for everything related to mods which affect server functionality
IModManager* ModManager();

template <class F, class ...T>
    requires (std::is_same_v<F, IModManager::Type>)
struct avledet::util::Streamer<F, T...>{ //lua_State> {
    //TODO create mapper to jun
    // perhaps tuple of ordered types by Type ordinal valud
    //using vals = std::tuple<
    //    avledet::util::Bytes,
    //    std::string,
    //    std::vector<std::string>,
    //    avledet::util::Bytes,
    //    avledet::util::ZDOID,
    //    avledet::util::CSU::Vector3f,
    //    avledet::util::CSU::Vector2i,
    //    avledet::util::CSU::Quaternion,
    //    std::int8_t,
    //    std::int16_t,
    //    std::int32_t,
    //    std::int64_t,
    //    std::uint8_t,
    //    std::uint16_t,
    //    std::uint32_t,
    //    std::uint64_t,
    //    std::float_t,
    //    std::double_t,
    //    char16_t>;

    void operator()(avledet::util::Writer& writer, IModManager::Type type, sol::object const& arg) {
        switch (type) {
            // TODO add recent unsigned types
        case IModManager::Type::UINT8:
            writer.write(arg.as<std::uint8_t>());
            break;
        case IModManager::Type::UINT16:
            writer.write(arg.as<std::uint16_t>());
            break;
        case IModManager::Type::UINT32:
            writer.write(arg.as<std::uint32_t>());
            break;
        case IModManager::Type::UINT64:
            writer.write(arg.as<std::uint64_t>());
            break;
        case IModManager::Type::INT8:
            writer.write(arg.as<std::int8_t>());
            break;
        case IModManager::Type::INT16:
            writer.write(arg.as<std::int16_t>());
            break;
        case IModManager::Type::INT32:
            writer.write(arg.as<std::int32_t>());
            break;
        case IModManager::Type::INT64:
            writer.write(arg.as<std::int64_t>());
            break;
        case IModManager::Type::FLOAT:
            writer.write(arg.as<std::float_t>());
            break;
        case IModManager::Type::DOUBLE:
            writer.write(arg.as<std::double_t>());
            break;
        case IModManager::Type::STRING:
            writer.write(arg.as<std::string>());
            break;
        case IModManager::Type::BOOL:
            writer.write(arg.as<bool>());
            break;
        case IModManager::Type::BYTES:
            writer.write(arg.as<avledet::util::Bytes>());
            break;
        case IModManager::Type::ZDOID:
            writer.write(arg.as<avledet::util::ZDOID>());
            break;
        case IModManager::Type::VECTOR3f:
            writer.write(arg.as<avledet::util::CSU::Vector3f>());
            break;
        case IModManager::Type::VECTOR2i:
            writer.write(arg.as<avledet::util::CSU::Vector2i>());
            break;
        case IModManager::Type::QUATERNION:
            writer.write(arg.as<avledet::util::CSU::Quaternion>());
            break;
        case IModManager::Type::CHAR16:
            writer.write(arg.as<char16_t>());
            break;
        default:
            throw std::runtime_error("type <" + std::string(magic_enum::enum_name(type)) + "> has no write implementation");
        }
    }

    sol::object operator()(avledet::util::Reader& reader, IModManager::Type type, lua_State* state) {
        switch (type) {
            case IModManager::Type::BYTES:
                // Will be interpreted as sol container type
                // see https://sol2.readthedocs.io/en/latest/containers.html
                return sol::make_object(state, reader.read<avledet::util::Bytes>());
            case IModManager::Type::STRING:
                // Primitive: string
                return sol::make_object(state, reader.read<std::string>());
            case IModManager::Type::ZDOID:
                // Userdata: ZDOID
                return sol::make_object(state, reader.read<avledet::util::ZDOID>());
            case IModManager::Type::VECTOR3f:
                // Userdata: Vector3f
                return sol::make_object(state, reader.read<avledet::util::CSU::Vector3f>());
            case IModManager::Type::VECTOR2i:
                // Userdata: Vector2i
                return sol::make_object(state, reader.read<avledet::util::CSU::Vector2i>());
            case IModManager::Type::QUATERNION:
                // Userdata: Quaternion
                return sol::make_object(state, reader.read<avledet::util::CSU::Quaternion>());
            case IModManager::Type::STRINGS:
                // Container type of Primitive: string
                return sol::make_object(state, reader.read<std::vector<std::string>>());
            case IModManager::Type::BOOL:
                // Primitive: boolean
                return sol::make_object(state, reader.read<bool>());
            case IModManager::Type::INT8:
                // Primitive: number
                return sol::make_object(state, reader.read<std::int8_t>());
            case IModManager::Type::INT16:
                // Primitive: number
                return sol::make_object(state, reader.read<std::int16_t>());
            case IModManager::Type::INT32:
                // Primitive: number
                return sol::make_object(state, reader.read<std::int32_t>());
            case IModManager::Type::INT64:
                // Userdata: Int64Wrapper
                return sol::make_object(state, Int64Wrapper(reader.read<std::int64_t>())); // ReadInt64());
            case IModManager::Type::UINT8:
                // Primitive: number
                return sol::make_object(state, reader.read<std::uint8_t>());
            case IModManager::Type::UINT16:
                // Primitive: number
                return sol::make_object(state, reader.read<std::uint16_t>());
            case IModManager::Type::UINT32:
                // Primitive: number
                return sol::make_object(state, reader.read<std::uint32_t>());
            case IModManager::Type::UINT64:
                // Userdata: UInt64Wrapper
                return sol::make_object(state, UInt64Wrapper(reader.read<std::uint64_t>()));
            case IModManager::Type::FLOAT:
                // Primitive: number
                return sol::make_object(state, reader.read<std::float_t>());
            case IModManager::Type::DOUBLE:
                // Primitive: number
                return sol::make_object(state, reader.read<std::double_t>());
            case IModManager::Type::CHAR16:
                // Primitive: number
                return sol::make_object(state, reader.read<char16_t>());
            default:
                throw std::runtime_error("invalid mod DataReader type");
        }
    }
};

// TODO
template <class F, class ...G>
    requires (std::is_same_v<F, IModManager::Types>)
struct avledet::util::Streamer<F, G...>{ //lua_State> {
    void operator()(avledet::util::Writer& writer, IModManager::Types const& types, sol::variadic_results const& results) {
        for (int i = 0; i < results.size(); i++) {
            writer.write(types.at(i), results.at(i));
        }
    }

    sol::variadic_results operator()(avledet::util::Reader& reader, IModManager::Types const& types, lua_State* state) {
        sol::variadic_results results;

        for (auto&& type : types) {
            results.push_back(reader.read(type, state));
        }

        return results;
    }
};

#else // !VH_USE_MODS
#define VH_DISPATCH_MOD_EVENT(name, ...) (true)
#define VH_DISPATCH_MOD_EVENT_TUPLE(name, ...) (true)
#endif // VH_USE_MODS
