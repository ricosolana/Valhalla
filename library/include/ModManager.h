#pragma once

#include "CompileSettings.h"
#include <sol/protected_function_result.hpp>

#if VH_IS_ON(VH_USE_MODS)

    #include "DataStream.h"
    #include "Hashes.h"
    #include "Quaternion.h"
    #include "Types.h"
    #include "ValhallaServer.h"
    #include "Vector.h"
    #include "VUtils.h"
    #include "ZDOID.h"
    #include <cmath>
    #include <cstdint>
    #include <list>
    #include <lua.h>
    #include <magic_enum.hpp>
    #include <sol/forward.hpp>
    #include <sol/sol.hpp>
    #include <vector>

class IModManager
{
  public:
    enum class StreamType
    {
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

        CHAR16,// utf8

        max
    };

    using StreamTypes = std::vector<StreamType>;

    class MethodSig
    {
      public:
        StreamTypes m_types;
        avledet::util::Hash m_hash;

        MethodSig(StreamTypes types, avledet::util::Hash hash) :
            m_types(std::move(types)),
            m_hash(hash)
        {
        }

        MethodSig(std::string_view name, sol::variadic_args types) :
            m_types(types.begin(), types.end()),
            m_hash(avledet::util::get_stable_hash(name))
        {
        }
    };

    class Events
    {
      public:
        // Game state events
        static constexpr avledet::util::Hash Enable         = __H("Enable");
        static constexpr avledet::util::Hash Disable        = __H("Disable");
        static constexpr avledet::util::Hash Update         = __H("Update");
        static constexpr avledet::util::Hash PeriodicUpdate = __H("Periodic");

        // Connecting peer events
        static constexpr avledet::util::Hash Connect    = __H("Connect");
        static constexpr avledet::util::Hash Disconnect = __H("Disconnect");

        // Connected peer events
        static constexpr avledet::util::Hash Join = __H("Join");
        static constexpr avledet::util::Hash Quit = __H("Quit");

        // Rpc events (some unimplemented)
        static constexpr avledet::util::Hash RpcIn  = __H("RpcIn"); // Peer -> Server
        static constexpr avledet::util::Hash RpcOut = __H("RpcOut");// Peer -> Server

        // Routed events (this is complicated)
        static constexpr avledet::util::Hash RouteIn     = __H("RouteIn");    // Peer -> Server
        static constexpr avledet::util::Hash RouteInAll  = __H("RouteInAll"); // Peer -> Server
        static constexpr avledet::util::Hash RouteOut    = __H("RouteOut");   // Server -> Peer
        static constexpr avledet::util::Hash RouteOutAll = __H("RouteOutAll");// Server -> Peer
        static constexpr avledet::util::Hash Routed      = __H("Routed");     // Peer -> Server -> Peer

        // General game events
        static constexpr avledet::util::Hash PlayerList = __H("PlayerList");

        static constexpr avledet::util::Hash ZDOUnpacked = __H("ZDOUnpacked");
        static constexpr avledet::util::Hash ZDOCreated  = __H("ZDOCreated");
        static constexpr avledet::util::Hash ZDOModified = __H("ZDOModified");
        static constexpr avledet::util::Hash SendingZDO  = __H("SendingZDO");
        //static constexpr avledet::util::Hash ZDODestroyed = __H("ZDODestroyed");

        // Socket methods events
        static constexpr avledet::util::Hash Send = __H("Send");
        static constexpr avledet::util::Hash Recv = __H("Recv");

        // Event postfix handler
        //static constexpr avledet::util::Hash POSTFIX = __H("POST");
    };

    class ScriptInfo
    {
      public:
        std::string m_name;

        // proto/src name of code chunk
        //  file:///home/.../script_entry.lua
        //  https://discord.com/channels/x/y/z
        std::string m_origin;

        std::string m_version;
        std::string m_apiVersion;
        std::string m_description;
        std::vector<std::string> m_authors;

      public:
        ScriptInfo(std::string name, fs::path entry) :
            m_name(name),
            m_origin(entry)
        {
        }

        ScriptInfo(std::string name, std::string const &sourceName) :
            m_name(name),
            m_origin(sourceName)//no file path, instead dynamically loaded from ?
        {
        }

        ScriptInfo(ScriptInfo const &)            = default;
        ScriptInfo(ScriptInfo &&)                 = default;
        ScriptInfo &operator=(ScriptInfo const &) = default;

        // If dynamically loaded (ie from discord); not during server initialization like all mods
        bool is_file_based() const
        {
            return m_origin.starts_with("file://");
        }

        std::string get_chunk_name() const
        {
            auto idx = m_origin.find("://") + sizeof("://");
            if (m_origin.starts_with("file")) {
                fs::path path = m_origin.substr(idx);
                return path.filename().string();
            } else {
                // assume web URL
                return m_name;//hmm
            }
        }

        fs::path get_entry_path() const
        {
            auto idx = m_origin.find("://") + sizeof("://");
            if (m_origin.starts_with("file")) {
                fs::path path = m_origin.substr(idx);
                return path;
            } else {
                // assume web URL
                throw std::runtime_error("dynamic scripts do not have a physical path");
            }
        }
    };

    struct EventHandle
    {
        sol::protected_function m_func;
        int m_priority;

        EventHandle(sol::function func, int priority) :
            m_func(func),
            m_priority(priority)
        {
        }
    };

  private:
    avledet::util::Map<std::string, std::unique_ptr<ScriptInfo>, ankerl::unordered_dense::string_hash,
                       std::equal_to<>>
            m_scripts;                                                            //64 bytes
    avledet::util::Map<avledet::util::Hash, std::vector<EventHandle>> m_callbacks;//64 bytes
    //gtl::btree_map<avledet::util::Hash, std::vector<std::pair<int, sol::function>>> m_callbacks;
    //avledet::util::Set<ScriptInfo *> m_tmp_reload_mods;                           //64 bytes
    sol::state m_state;//48 bytes
    bool m_tmp_unsubscribe {};

  private:
    void load_userdata_network();
    void load_userdata_peer();
    void load_userdata_prefab();
    void load_userdata_quaternion();
    void load_userdata_types();
    void load_userdata_vector();
    void load_userdata_zdo();
    void load_userdata_zone();

    // TODO authorize based on type
    //  ie, native, filebased, dynamic...
    sol::environment create_sandbox();

  public:
    void load_userdata();

    std::tuple<ScriptInfo, std::string> load_file_script(fs::path script_root);
    void execute(ScriptInfo const &info, std::string const &code);//dynamic or mobile script

    // my immutable usertype
    template<typename Class, typename... Args>
    sol::usertype<Class> new_usertype(Args &&...args)
    {
        return m_state.new_usertype<Class>(std::forward<Args>(args)..., sol::meta_method::static_new_index,
                                           [](sol::variadic_args) -> sol::object {
                                               throw std::runtime_error(
                                                       "cant index userdata, sandboxing is enabled!");
                                           });
    }

  public:
    void PostInit();
    void Uninit();
    void update();
    void unload_mod(ScriptInfo &mod);

    // Dispatch a Lua event
    //  Returns false if the event requested cancellation
    template<class... Args>
    bool CallEvent(avledet::util::Hash name, Args &&...params)
    {
        ZoneScoped;
        //ZoneNamed(CallEvent, true);

        auto &&find = m_callbacks.find(name);
        if (find != m_callbacks.end()) {
            auto &&callbacks = find->second;

            for (auto &&itr = callbacks.begin(); itr != callbacks.end();) {
                this->m_tmp_unsubscribe = false;// Default unsubscribe state

                // Params are COPIED
                //  this makes modifications of primitives impossible, but could still modify
                //  pointers/userdata tables...
                //sol::function_result result = itr->second(Args(params)...);
                sol::protected_function_result result = itr->m_func(Args(params)...);
                if (!result.valid()) {
                    LOG_ERROR(VH_LOGGER, "Event error: ");
                    sol::error error = result;
                    LOG_ERROR(VH_LOGGER, "{}", error.what());

                    // On error, we invalidate the event
                    this->m_tmp_unsubscribe = true;
                } else {
                    // whether cancelled-events should follow Harmony prefix cancellation with bools
                    if (result.get_type() == sol::type::boolean) {
                        if (!result.get<bool>())
                            return false;
                    }
                }

                if (this->m_tmp_unsubscribe) {
                    itr = callbacks.erase(itr);
                } else {
                    ++itr;
                }
            }
        }

        return true;
    }

    // Dispatch a Lua event
    //  Returns whether the event was requested for cancellation
    template<typename... Args>
    auto CallEvent(std::string_view name, Args &&...params)
    {
        return CallEvent(avledet::util::get_stable_hash(name), std::forward<Args>(params)...);
    }

  private:
    // Dispatch a Lua event
    //  Returns whether the event was requested for cancellation
    template<class Tuple, std::size_t... Is>
    auto CallEventTupleImpl(avledet::util::Hash name, Tuple const &t, std::index_sequence<Is...>)
    {
        return CallEvent(name, std::get<Is>(t)...);// TODO use forward
    }

  public:
    // Dispatch a Lua event
    //  Returns whether the event was requested for cancellation
    template<class Tuple>
    auto CallEventTuple(avledet::util::Hash name, Tuple const &t)
    {
        return CallEventTupleImpl(name, t, std::make_index_sequence<std::tuple_size<Tuple> {}> {});
    }

    // Dispatch a Lua event
    //  Returns whether the event was requested for cancellation
    template<class Tuple>
    auto CallEventTuple(std::string_view name, Tuple const &t)
    {
        return CallEventTupleImpl(avledet::util::get_stable_hash(name), t,
                                  std::make_index_sequence<std::tuple_size<Tuple> {}> {});
    }
};

    #define VH_DISPATCH_MOD_EVENT(name, ...) ModManager()->CallEvent((name) __VA_OPT__(, ) __VA_ARGS__)
    #define VH_DISPATCH_MOD_EVENT_TUPLE(name, ...) \
        ModManager()->CallEventTuple((name) __VA_OPT__(, ) __VA_ARGS__)

// Manager class for everything related to mods which affect server functionality
IModManager *ModManager();

template<class F, class... T>
    requires(std::is_same_v<F, IModManager::StreamType>)
struct avledet::util::Streamer<F, T...>
{

    void operator()(avledet::util::Writer &writer, IModManager::StreamType type, sol::object const &arg)
    {
        switch (type) {
            // TODO add recent unsigned types
        case IModManager::StreamType::UINT8: writer.write(arg.as<std::uint8_t>()); break;
        case IModManager::StreamType::UINT16: writer.write(arg.as<std::uint16_t>()); break;
        case IModManager::StreamType::UINT32: writer.write(arg.as<std::uint32_t>()); break;
        case IModManager::StreamType::UINT64: writer.write(arg.as<std::uint64_t>()); break;
        case IModManager::StreamType::INT8: writer.write(arg.as<std::int8_t>()); break;
        case IModManager::StreamType::INT16: writer.write(arg.as<std::int16_t>()); break;
        case IModManager::StreamType::INT32: writer.write(arg.as<std::int32_t>()); break;
        case IModManager::StreamType::INT64: writer.write(arg.as<std::int64_t>()); break;
        case IModManager::StreamType::FLOAT: writer.write(arg.as<std::float_t>()); break;
        case IModManager::StreamType::DOUBLE: writer.write(arg.as<std::double_t>()); break;
        case IModManager::StreamType::STRING: writer.write(arg.as<std::string>()); break;
        case IModManager::StreamType::BOOL: writer.write(arg.as<bool>()); break;
        case IModManager::StreamType::BYTES: writer.write(arg.as<avledet::util::Bytes>()); break;
        case IModManager::StreamType::ZDOID: writer.write(arg.as<avledet::util::ZDOID>()); break;
        case IModManager::StreamType::VECTOR3f: writer.write(arg.as<avledet::util::CSU::Vector3f>()); break;
        case IModManager::StreamType::VECTOR2i: writer.write(arg.as<avledet::util::CSU::Vector2i>()); break;
        case IModManager::StreamType::QUATERNION:
            writer.write(arg.as<avledet::util::CSU::Quaternion>());
            break;
        case IModManager::StreamType::CHAR16: writer.write(arg.as<char16_t>()); break;
        default:
            throw std::runtime_error("type <" + std::string(magic_enum::enum_name(type))
                                     + "> has no write implementation");
        }
    }

    sol::object operator()(avledet::util::Reader &reader, IModManager::StreamType type, lua_State *state)
    {
        switch (type) {
        case IModManager::StreamType::BYTES:
            // Will be interpreted as sol container type
            // see https://sol2.readthedocs.io/en/latest/containers.html
            return sol::make_object(state, reader.read<avledet::util::Bytes>());
        case IModManager::StreamType::STRING:
            // Primitive: string
            return sol::make_object(state, reader.read<std::string>());
        case IModManager::StreamType::ZDOID:
            // Userdata: ZDOID
            return sol::make_object(state, reader.read<avledet::util::ZDOID>());
        case IModManager::StreamType::VECTOR3f:
            // Userdata: Vector3f
            return sol::make_object(state, reader.read<avledet::util::CSU::Vector3f>());
        case IModManager::StreamType::VECTOR2i:
            // Userdata: Vector2i
            return sol::make_object(state, reader.read<avledet::util::CSU::Vector2i>());
        case IModManager::StreamType::QUATERNION:
            // Userdata: Quaternion
            return sol::make_object(state, reader.read<avledet::util::CSU::Quaternion>());
        case IModManager::StreamType::STRINGS:
            // Container type of Primitive: string
            //return sol::make_object(state, reader.read<avledet::util::Strings>());
            return sol::make_object(state, reader.read<std::vector<std::string>>());
        case IModManager::StreamType::BOOL:
            // Primitive: boolean
            return sol::make_object(state, reader.read<bool>());
        case IModManager::StreamType::INT8:
            // Primitive: number
            return sol::make_object(state, reader.read<std::int8_t>());
        case IModManager::StreamType::INT16:
            // Primitive: number
            return sol::make_object(state, reader.read<std::int16_t>());
        case IModManager::StreamType::INT32:
            // Primitive: number
            return sol::make_object(state, reader.read<std::int32_t>());
        case IModManager::StreamType::INT64:
            // Userdata: Int64Wrapper
            return sol::make_object(state, Int64Wrapper(reader.read<std::int64_t>()));// ReadInt64());
        case IModManager::StreamType::UINT8:
            // Primitive: number
            return sol::make_object(state, reader.read<std::uint8_t>());
        case IModManager::StreamType::UINT16:
            // Primitive: number
            return sol::make_object(state, reader.read<std::uint16_t>());
        case IModManager::StreamType::UINT32:
            // Primitive: number
            return sol::make_object(state, reader.read<std::uint32_t>());
        case IModManager::StreamType::UINT64:
            // Userdata: UInt64Wrapper
            return sol::make_object(state, UInt64Wrapper(reader.read<std::uint64_t>()));
        case IModManager::StreamType::FLOAT:
            // Primitive: number
            return sol::make_object(state, reader.read<std::float_t>());
        case IModManager::StreamType::DOUBLE:
            // Primitive: number
            return sol::make_object(state, reader.read<std::double_t>());
        case IModManager::StreamType::CHAR16:
            // Primitive: number
            return sol::make_object(state, reader.read<char16_t>());
        default: throw std::runtime_error("invalid mod DataReader type");
        }
    }
};

// TODO
template<class F, class... G>
    requires(std::is_same_v<F, IModManager::StreamTypes>)
struct avledet::util::Streamer<F, G...>
{//lua_State> {

    void operator()(avledet::util::Writer &writer, IModManager::StreamTypes const &types,
                    sol::variadic_results const &results)
    {
        for (int i = 0; i < results.size(); i++) {
            writer.write(types.at(i), results.at(i));
        }
    }

    sol::variadic_results operator()(avledet::util::Reader &reader, IModManager::StreamTypes const &types,
                                     lua_State *state)
    {
        sol::variadic_results results;

        for (auto &&type : types) {
            results.push_back(reader.read(type, state));
        }

        return results;
    }
};

#else // !VH_USE_MODS
    #define VH_DISPATCH_MOD_EVENT(name, ...)       (true)
    #define VH_DISPATCH_MOD_EVENT_TUPLE(name, ...) (true)
#endif// VH_USE_MODS
