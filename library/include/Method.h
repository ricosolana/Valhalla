#pragma once

#include <memory>
#include <quill/LogMacros.h>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include "Avledet.h"
#include "DataStream.h"
#include "ModManager.h"
#include "Types.h"
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
    avledet::util::Hash const m_hash;

  public:
    IMethod(avledet::util::Hash hash) :
        m_hash(hash)
    {
        assert(m_hash);
    }

    virtual ~IMethod() {}

    // Calls a locally stored function
    //  Expects a passthrough parameter and serialized package
    //  Returns false if the call requested unsubscription
    virtual bool Invoke(T t, DataReader reader) = 0;

    // unused, but will be used for method hashset
    //friend bool operator<=>(std::unique_ptr<IMethod<T>> const &lhs, std::unique_ptr<IMethod<T>> const &rhs)
    //{
    //    return lhs->m_hash <=> rhs->m_hash;
    //}
    //
    //friend bool operator<=>(std::unique_ptr<IMethod<T>> const &lhs, avledet::util::Hash rhs)
    //{
    //    return lhs->m_hash <=> rhs;
    //}
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

#if AVL_IS_ON(AVL_ENABLE_SCRIPTING)
    avledet::util::Hash const m_categoryHash {};

  public:
    MethodImpl(avledet::util::Hash hash, avledet::util::Hash categoryHash, F func) :
        IMethod<T>(hash),
        m_func(std::move(func)),
        m_categoryHash(categoryHash)//will keep this as a member for dynamic lua usages...
    {
    }
#endif

  public:
    MethodImpl(avledet::util::Hash hash, F func) :
        IMethod<T>(hash),
        m_func(std::move(func))
    {
    }

    bool Invoke(T t, DataReader reader) override
    {
        auto tuple = std::tuple_cat(
                std::forward_as_tuple(t),
                impl_tail<args_type>(reader,
                                     (std::make_index_sequence<std::tuple_size<args_type> {} - 1> {})));

        if (reader.get_pos() != reader.size()) {
            //LOG_WARNING(AVL_LOGGER, "Peer Rpc Invoke has more data than expected {}/{}", reader.size(), reader.get_pos());
            throw std::runtime_error("peer sent more data than expected");
        }

        // TODO add category solo-prefix
        //  dont know where it went... likely performance concerns... but still fast-ish

#if AVL_IS_ON(AVL_ENABLE_SCRIPTING)
        // Prefix
        if (!AVL_SCRIPT_EVENT_TUPLE(m_categoryHash ^ this->m_hash, tuple))
            return true;
#endif

        bool keep_me_mapped = true;

        LOG_TRACE_L1(AVL_LOGGER, "calling internal method, hash {}", this->m_hash);

        if constexpr (std::is_same_v<bool, typename VUtils::Traits::func_traits<F>::result_type>) {
            keep_me_mapped = std::apply(m_func, tuple);
        } else {
            std::apply(m_func, tuple);
        }

        /*
        // Postfix
        AVL_SCRIPT_EVENT_TUPLE(m_categoryHash ^ m_methodHash ^ IScriptManager::Events::POSTFIX, tuple);
        */

        return keep_me_mapped;
    }
};


#if AVL_IS_ON(AVL_ENABLE_SCRIPTING)
template<typename F>
MethodImpl(avledet::util::Hash, avledet::util::Hash,
           F) -> MethodImpl<std::tuple_element_t<0, typename VUtils::Traits::func_traits<F>::args_type>, F>;
#else
template<typename F>
MethodImpl(avledet::util::Hash,
           F) -> MethodImpl<std::tuple_element_t<0, typename VUtils::Traits::func_traits<F>::args_type>, F>;
#endif


#if AVL_IS_ON(AVL_ENABLE_SCRIPTING)

template<class T>
class MethodImplLua : public IMethod<T>
{
    friend class IScriptManager;

  private:
    sol::protected_function m_func;
    IScriptManager::StreamTypes m_types;

  public:
    MethodImplLua(avledet::util::Hash hash, sol::protected_function const &func,
                  IScriptManager::StreamTypes const &types) :
        IMethod<T>(hash),
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
    #if AVL_IS_ON(AVL_REFLECTIVE_MOD_EVENTS)
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
    #if AVL_IS_ON(AVL_REFLECTIVE_MOD_EVENTS)
        AVL_SCRIPT_EVENT(m_categoryHash ^ m_methodHash ^ IScriptManager::Events::POSTFIX,
                         sol::as_args(results));
    #endif

        if (result.get_type() == sol::type::boolean)
            return result.get<bool>();

        return true;
    }
};

template<typename T>
MethodImplLua(avledet::util::Hash, sol::function, IScriptManager::StreamTypes) -> MethodImplLua<T>;

#endif// AVL_IS_ON(AVL_ENABLE_SCRIPTING)
