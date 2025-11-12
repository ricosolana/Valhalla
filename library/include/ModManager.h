#pragma once

#include "CompileSettings.h"
#include <filesystem>
#include <sol/as_args.hpp>
#include <sol/as_returns.hpp>
#include <sol/object.hpp>
#include <sol/stack_reference.hpp>
#include <sol/variadic_args.hpp>
#include <stdexcept>
#include <string>
#include <variant>

#if AVL_IS_ON(AVL_ENABLE_SCRIPTING)

    #include <cmath>
    #include <cstddef>
    #include <cstdint>
    #include <list>
    #include <utility>
    #include <vector>

    // Important order // Must be before lua.h
    #include <sol/state.hpp>

    #include <intrusive_shared_ptr/intrusive_shared_ptr.h>
    #include <lua.h>
    //#include <magic_enum.hpp> // TODO magic
    #include <magic_enum/magic_enum.hpp>
    #include <sol/forward.hpp>
    #include <sol/protected_function_result.hpp>
    #include <sol/sol.hpp>

    #include "Avledet.h"
    #include "DataStream.h"
    #include "Hashes.h"
    #include "Quaternion.h"
    #include "Types.h"
    #include "Vector.h"
    #include "VUtils.h"
    #include "VUtilsRandom.h"
    #include "ZDOID.h"

namespace sol {
    template<typename T, typename Traits>
    struct unique_usertype_traits<isptr::intrusive_shared_ptr<T, Traits>>
    {
        typedef T type;
        typedef isptr::intrusive_shared_ptr<T, Traits> actual_type;
        static bool const value = true;

        static bool is_null(actual_type const &ptr)
        {
            return ptr == nullptr;
        }

        static type *get(actual_type const &ptr)
        {
            return ptr.get();
        }
    };
}// namespace sol

class IScriptManager
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
        friend class IScriptManager;

      public:
        std::string m_name;
        std::string m_chunk_name;

      private:
        std::string m_version;
        std::string m_apiVersion;
        std::string m_description;
        std::vector<std::string> m_authors;
        sol::environment m_env;

        // root or the script url
        std::variant<std::filesystem::path, std::string> m_uri;

        // only if a file-script
        //  TODO consider making paired with fs::path / uri above ^^^
        std::filesystem::file_time_type m_last_change;

      private:
        //ScriptInfo(std::string name, std::string chunk_name, sol::environment env) :
        //    m_name(std::move(name)),
        //    m_chunk_name(std::move(chunk_name)),
        //    m_env(std::move(env))
        //{
        //}

      public:
        ScriptInfo(std::string name, std::string chunk_name, std::filesystem::path path,
                   std::filesystem::file_time_type last_change) :
            m_name(std::move(name)),
            m_chunk_name(std::move(chunk_name)),
            m_uri(std::move(path)),
            m_last_change(last_change)
        {
        }

        ScriptInfo(std::string name, std::string chunk_name, std::string uri) :
            m_name(std::move(name)),
            m_chunk_name(std::move(chunk_name)),
            m_uri(std::move(uri)),
            m_last_change()
        {
        }

        //ScriptInfo(std::string name) :
        //    ScriptInfo(std::move(name), avledet::util::generate("abcdefghijklmnopqrstuvwxyz", 12))
        //{
        //}

        ScriptInfo(ScriptInfo const &)            = default;
        ScriptInfo(ScriptInfo &&)                 = default;
        ScriptInfo &operator=(ScriptInfo const &) = default;

        bool is_fs_script() const
        {
            auto &&get = std::get_if<std::filesystem::path>(&m_uri);
            return get != nullptr;
        }

        std::filesystem::path get_info_dir() const
        {
            return std::get<std::filesystem::path>(m_uri) / "scriptInfo.yml";
        }

        // If dynamically loaded (ie from discord); not during server initialization like all scripts
        //bool is_file_based() const
        //{
        //    return m_origin.starts_with("file://");
        //}

        //std::string get_chunk_name() const
        //{
        //    auto idx = m_origin.find("://") + sizeof("://");
        //    if (m_origin.starts_with("file://")) {
        //        std::filesystem::path path = m_origin.substr(idx);
        //        return path.filename().string();
        //    } else {
        //        // assume web URL
        //        return m_name;//hmm
        //    }
        //}

        //std::filesystem::path get_entry_path() const
        //{
        //    auto idx = m_origin.find("://") + sizeof("://");
        //    if (m_origin.starts_with("file")) {
        //        std::filesystem::path path = m_origin.substr(idx);
        //        return path;
        //    } else {
        //        // assume web URL
        //        throw std::runtime_error("dynamic scripts do not have a physical path");
        //    }
        //}
    };

    struct EventHandle
    {
        sol::protected_function m_func;//32 bytes
        sol::environment m_env;        //16 bytes
        int m_priority;                //4 bytes

        EventHandle(sol::function func, sol::environment env, int priority) :
            m_func(std::move(func)),
            m_env(std::move(env)),
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

    void unload_script(decltype(m_scripts)::iterator &script_itr, bool gc, bool pop);

    void reload_script(decltype(m_scripts)::iterator &script_itr);

  public:
    void load_userdata();

    std::tuple<ScriptInfo, std::string> load_file_script(std::filesystem::path script_root);
    void execute(ScriptInfo const &info, std::string const &code, bool replace);//dynamic or mobile script

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
    ~IScriptManager();

    void Init();
    void Uninit();
    void update();

    bool reload_script(std::string_view name);

    void reload_all();

    //decltype(m_callbacks)::iterator::value_type::second_type::iterator
    // Returns if early cancel requested by script
    bool _CallEvent(std::vector<EventHandle> &evts, std::vector<EventHandle>::iterator &evt_itr,
                    sol::variadic_results args)
    {
        this->m_tmp_unsubscribe = false;// Default unsubscribe state

        // Params are COPIED
        //  this makes modifications of primitives impossible, but could still modify
        //  pointers/userdata tables...
        //sol::function_result result = itr->second(Args(params)...);
        sol::protected_function_result result = evt_itr->m_func(sol::as_args(args));
        if (!result.valid()) {
            sol::error error = result;
            LOG_ERROR(AVL_LOGGER, "{}", error.what());

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
            evt_itr = evts.erase(evt_itr);
        } else {
            ++evt_itr;
        }

        return true;
    }

    // script_info is optionally null
    // call events NOT globally (across all scripts), but instead on given script
    template<class... Args>
    bool CallEventOn(ScriptInfo *script_info, avledet::util::Hash name, Args &&...params)
    {
        ZoneScoped;
        //ZoneNamed(CallEvent, true);

        auto &&find = m_callbacks.find(name);
        if (find != m_callbacks.end()) {
            auto &&callbacks = find->second;

            for (auto &&itr = callbacks.begin(); itr != callbacks.end();) {
                if (script_info && itr->m_env != script_info->m_env) {
                    ++itr;
                    continue;
                }

                sol::variadic_results results;
                results.reserve(sizeof...(params));

                // folding
                ((results.emplace_back(sol::make_object(m_state.lua_state(), std::forward<Args>(params)))),
                 ...);

                //sol::variadic_results results(
                //        sol::make_object(m_state.lua_state(), std::forward<Args>(params))...);

                //sol::variadic_results results(std::forward<Args>(params)...);

                if (!this->_CallEvent(callbacks, itr, std::move(results))) {
                    return false;
                }
            }
        }

        return true;
    }

    // Dispatch a Lua event
    //  Returns false if the event requested cancellation
    template<class... Args>
    bool CallEvent(avledet::util::Hash name, Args &&...params)
    {
        return CallEventOn(nullptr, name, std::forward<Args>(params)...);
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

    #define AVL_SCRIPT_EVENT(name, ...) ScriptManager()->CallEvent((name) __VA_OPT__(, ) __VA_ARGS__)
    #define AVL_SCRIPT_EVENT_TUPLE(name, ...) \
        ScriptManager()->CallEventTuple((name) __VA_OPT__(, ) __VA_ARGS__)

// Manager class for everything related to lua scripts
IScriptManager *ScriptManager();

template<class F, class... T>
    requires(std::is_same_v<F, IScriptManager::StreamType>)
struct avledet::util::Streamer<F, T...>
{

    template<class ObjectOrProxy>
    void operator()(avledet::util::Writer &writer, IScriptManager::StreamType type, ObjectOrProxy const &arg)
    {
        switch (type) {
            // TODO add recent unsigned types
        case IScriptManager::StreamType::UINT8: writer.write(arg.template as<std::uint8_t>()); break;
        case IScriptManager::StreamType::UINT16: writer.write(arg.template as<std::uint16_t>()); break;
        case IScriptManager::StreamType::UINT32: writer.write(arg.template as<std::uint32_t>()); break;
        case IScriptManager::StreamType::UINT64: writer.write(arg.template as<std::uint64_t>()); break;
        case IScriptManager::StreamType::INT8: writer.write(arg.template as<std::int8_t>()); break;
        case IScriptManager::StreamType::INT16: writer.write(arg.template as<std::int16_t>()); break;
        case IScriptManager::StreamType::INT32: writer.write(arg.template as<std::int32_t>()); break;
        case IScriptManager::StreamType::INT64: writer.write(arg.template as<std::int64_t>()); break;
        case IScriptManager::StreamType::FLOAT: writer.write(arg.template as<std::float_t>()); break;
        case IScriptManager::StreamType::DOUBLE: writer.write(arg.template as<std::double_t>()); break;
        case IScriptManager::StreamType::STRING: writer.write(arg.template as<std::string>()); break;
        case IScriptManager::StreamType::BOOL: writer.write(arg.template as<bool>()); break;
        case IScriptManager::StreamType::BYTES: writer.write(arg.template as<avledet::util::Bytes>()); break;
        case IScriptManager::StreamType::ZDOID: writer.write(arg.template as<avledet::util::ZDOID>()); break;
        case IScriptManager::StreamType::VECTOR3f:
            writer.write(arg.template as<avledet::util::CSU::Vector3f>());
            break;
        case IScriptManager::StreamType::VECTOR2i:
            writer.write(arg.template as<avledet::util::CSU::Vector2i>());
            break;
        case IScriptManager::StreamType::QUATERNION:
            writer.write(arg.template as<avledet::util::CSU::Quaternion>());
            break;
        case IScriptManager::StreamType::CHAR16: writer.write(arg.template as<char16_t>()); break;
        default:
            throw std::runtime_error("type <" + std::string(magic_enum::enum_name(type))
                                     + "> has no write implementation");
        }
    }

    sol::object operator()(avledet::util::Reader &reader, IScriptManager::StreamType type, lua_State *state)
    {
        switch (type) {
        case IScriptManager::StreamType::BYTES:
            // Will be interpreted as sol container type
            // see https://sol2.readthedocs.io/en/latest/containers.html
            return sol::make_object(state, reader.read<avledet::util::Bytes>());
        case IScriptManager::StreamType::STRING:
            // Primitive: string
            return sol::make_object(state, reader.read<std::string>());
        case IScriptManager::StreamType::ZDOID:
            // Userdata: ZDOID
            return sol::make_object(state, reader.read<avledet::util::ZDOID>());
        case IScriptManager::StreamType::VECTOR3f:
            // Userdata: Vector3f
            return sol::make_object(state, reader.read<avledet::util::CSU::Vector3f>());
        case IScriptManager::StreamType::VECTOR2i:
            // Userdata: Vector2i
            return sol::make_object(state, reader.read<avledet::util::CSU::Vector2i>());
        case IScriptManager::StreamType::QUATERNION:
            // Userdata: Quaternion
            return sol::make_object(state, reader.read<avledet::util::CSU::Quaternion>());
        case IScriptManager::StreamType::STRINGS:
            // Container type of Primitive: string
            //return sol::make_object(state, reader.read<avledet::util::Strings>());
            return sol::make_object(state, reader.read<std::vector<std::string>>());
        case IScriptManager::StreamType::BOOL:
            // Primitive: boolean
            return sol::make_object(state, reader.read<bool>());
        case IScriptManager::StreamType::INT8:
            // Primitive: number
            return sol::make_object(state, reader.read<std::int8_t>());
        case IScriptManager::StreamType::INT16:
            // Primitive: number
            return sol::make_object(state, reader.read<std::int16_t>());
        case IScriptManager::StreamType::INT32:
            // Primitive: number
            return sol::make_object(state, reader.read<std::int32_t>());
        case IScriptManager::StreamType::INT64:
            // Userdata: Int64Wrapper
            return sol::make_object(state, Int64Wrapper(reader.read<std::int64_t>()));// ReadInt64());
        case IScriptManager::StreamType::UINT8:
            // Primitive: number
            return sol::make_object(state, reader.read<std::uint8_t>());
        case IScriptManager::StreamType::UINT16:
            // Primitive: number
            return sol::make_object(state, reader.read<std::uint16_t>());
        case IScriptManager::StreamType::UINT32:
            // Primitive: number
            return sol::make_object(state, reader.read<std::uint32_t>());
        case IScriptManager::StreamType::UINT64:
            // Userdata: UInt64Wrapper
            return sol::make_object(state, UInt64Wrapper(reader.read<std::uint64_t>()));
        case IScriptManager::StreamType::FLOAT:
            // Primitive: number
            return sol::make_object(state, reader.read<std::float_t>());
        case IScriptManager::StreamType::DOUBLE:
            // Primitive: number
            return sol::make_object(state, reader.read<std::double_t>());
        case IScriptManager::StreamType::CHAR16:
            // Primitive: number
            return sol::make_object(state, reader.read<char16_t>());
        default: throw std::runtime_error("invalid DataReader type");
        }
    }
};

// TODO
template<class F, class... G>
    requires(std::is_same_v<F, IScriptManager::StreamTypes>)
struct avledet::util::Streamer<F, G...>
{//lua_State> {

    template<typename ArgsOrResults>
    void operator()(avledet::util::Writer &writer, IScriptManager::StreamTypes const &types,
                    ArgsOrResults const &args)
    {
        if (types.size() != args.size()) {
            throw std::runtime_error("types size must match the # of passed write arguments");
        }

        for (std::size_t i = 0; i < args.size(); i++) {
            writer.write(types[i], args[(int) i]);
        }
    }

    sol::variadic_results operator()(avledet::util::Reader &reader, IScriptManager::StreamTypes const &types,
                                     lua_State *state)
    {
        sol::variadic_results results;

        for (auto &&type : types) {
            results.push_back(reader.read(type, state));
        }

        return results;
    }
};

#else // !AVL_ENABLE_SCRIPTING
    #define AVL_SCRIPT_EVENT(name, ...)       (true)
    #define AVL_SCRIPT_EVENT_TUPLE(name, ...) (true)
#endif// AVL_ENABLE_SCRIPTING
