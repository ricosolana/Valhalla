#pragma once

#include <stdexcept>
#include <tuple>
#include <type_traits>

#include "DataStream.h"
#include "ModManager.h"
#include "VUtils.h"
#include "VUtilsTraits.h"

/* https://godbolt.org/z/MMGsa8rhr
* to implement deduction guides
* (static functions, no class object needed)
*/

// Thanks @Fux
template<class T>
class IMethod
{
  public:
    virtual ~IMethod() {}

    // Calls a locally stored function
    //  Expects a passthrough parameter and serialized package
    //  Returns false if the call requested unsubscription
    virtual bool Invoke(T t, DataReader reader) = 0;
};

// Package lambda invoker
template<class T, typename F>
class MethodImpl : public IMethod<T>
{
    using args_type = typename VUtils::Traits::func_traits<F>::args_type;

    template<class Tuple, std::size_t... Is>
    auto impl_tail(DataReader &reader, std::index_sequence<Is...>)
    {
        return DataReader::deserialize<std::tuple_element_t<Is + 1u, Tuple>...>(reader);
    }

  private:
    F const m_func;

#if VH_IS_ON(AVL_ENABLE_SCRIPTING)
    avledet::util::Hash const m_categoryHash;
    avledet::util::Hash const m_methodHash;
#endif

  public:
#if VH_IS_ON(AVL_ENABLE_SCRIPTING)
    MethodImpl(F func, avledet::util::Hash categoryHash, avledet::util::Hash methodHash) :
        m_func(func),
        m_categoryHash(categoryHash),
        m_methodHash(methodHash)
    {
    }
#else
    MethodImpl(F func) :
        m_func(func)
    {
    }
#endif

    bool Invoke(T t, DataReader reader) override
    {
        auto tuple = std::tuple_cat(
                std::forward_as_tuple(t),
                //NetPackage::Deserialize<Args...>(pkg));
                impl_tail<args_type>(reader,
                                     (std::make_index_sequence<std::tuple_size<args_type> {} - 1> {})));

        if (reader.get_pos() != reader.size()) {
            //LOG_WARNING(VH_LOGGER, "Peer Rpc Invoke has more data than expected {}/{}", reader.size(), reader.get_pos());
            throw std::runtime_error("peer sent more data than expected");
        }

#if VH_IS_ON(AVL_ENABLE_SCRIPTING)
        // Prefix
        if (!AVL_SCRIPT_EVENT_TUPLE(m_categoryHash ^ m_methodHash, tuple))
            return true;
#endif

        bool result = true;

        if constexpr (std::is_same_v<bool, typename VUtils::Traits::func_traits<F>::result_type>) {
            result = std::apply(m_func, tuple);
        } else
            std::apply(m_func, tuple);

        /*
        // Postfix
        AVL_SCRIPT_EVENT_TUPLE(m_categoryHash ^ m_methodHash ^ IScriptManager::Events::POSTFIX, tuple);
        */

        return result;
    }
};

template<typename F>
MethodImpl(F, avledet::util::Hash, avledet::util::Hash)
        -> MethodImpl<std::tuple_element_t<0, typename VUtils::Traits::func_traits<F>::args_type>, F>;


#if VH_IS_ON(AVL_ENABLE_SCRIPTING)

template<class T>
class MethodImplLua : public IMethod<T>
{
    friend class IScriptManager;

  private:
    sol::protected_function m_func;
    IScriptManager::StreamTypes m_types;

  public:
    MethodImplLua(sol::protected_function const &func, IScriptManager::StreamTypes const &types) :
        m_func(func),
        m_types(types)
    {
    }

    bool Invoke(T t, DataReader reader) override
    {
        auto &&state = m_func.lua_state();

        //TODO
        //  this can be a simple Streamer now,
        //  with args passed variadically using the newer read() in Stream
        auto results = reader.read(m_types, state);

        // Prefix
    #if VH_IS_ON(VH_REFLECTIVE_MOD_EVENTS)
        if (!AVL_SCRIPT_EVENT(m_categoryHash ^ m_methodHash, sol::as_args(results)))
            return;
    #endif

        sol::protected_function_result result = m_func(t, sol::as_args(results));
        if (!result.valid()) {
            // player invocation was bad
            sol::error error = result;
            throw error;
        }

        // Postfix
    #if VH_IS_ON(VH_REFLECTIVE_MOD_EVENTS)
        AVL_SCRIPT_EVENT(m_categoryHash ^ m_methodHash ^ IScriptManager::Events::POSTFIX,
                         sol::as_args(results));
    #endif

        if (result.get_type() == sol::type::boolean)
            return result.get<bool>();

        return true;
    }
};

template<typename T>
MethodImplLua(sol::function, IScriptManager::StreamTypes) -> MethodImplLua<T>;

#endif// VH_IS_ON(AVL_ENABLE_SCRIPTING)
