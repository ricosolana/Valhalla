#pragma once

#include <tuple>
#include <functional>
#include <type_traits>
#include <concepts>

#include "VUtils.h"
#include "VUtilsTraits.h"
#include "DataReader.h"
#include "ModManager.h"

/* https://godbolt.org/z/MMGsa8rhr
* to implement deduction guides
* (static functions, no class object needed)
*/

// Thanks @Fux
template<class T>
class IMethod
{
public:
    // Calls a locally stored function
    //  Expects a passthrough parameter and serialized package
    //  Returns false if the call requested unsubscription
    virtual bool Invoke(T t, DataReader &reader) = 0;
};


// Package lambda invoker
template<class T, typename F>
class MethodImpl
    : public IMethod<T>
{
    using args_type = typename VUtils::Traits::func_traits<F>::args_type;

    template<class Tuple, size_t... Is>
    auto impl_tail(DataReader& reader, std::index_sequence<Is...>) {
        return DataReader::Deserialize<std::tuple_element_t<Is + 1u, Tuple>...>(reader);
    }

private:
    const F m_func;

    const HASH_t m_categoryHash;
    const HASH_t m_methodHash;

public:
    explicit MethodImpl(F func, HASH_t categoryHash, HASH_t methodHash)
        : m_func(func), m_categoryHash(categoryHash), m_methodHash(methodHash) {}

    bool Invoke(T t, DataReader &reader) override {
        auto tuple = std::tuple_cat(std::forward_as_tuple(t),
            //NetPackage::Deserialize<Args...>(pkg));
            impl_tail<args_type>(reader,
                (std::make_index_sequence < std::tuple_size<args_type>{} - 1 > {})));

        if (reader.Position() != reader.Length())
            LOG(WARNING) << "Peer Rpc Invoke has more data than expected "
            << reader.Length() << "/" << reader.Position();

        // Prefix
        if (!ModManager()->CallEventTuple(m_categoryHash ^ m_methodHash, tuple))
            return true;

        bool result = true;
        
        if constexpr (std::is_same_v<bool, VUtils::Traits::func_traits<F>::result_type>) {
            result = std::apply(m_func, tuple);
        } else
            std::apply(m_func, tuple);
    
        // Postfix
        ModManager()->CallEventTuple(m_categoryHash ^ m_methodHash ^ IModManager::EVENT_POST, tuple);

        return result;
    }
};

template<typename F>
MethodImpl(F, HASH_t, HASH_t) -> MethodImpl<
    std::tuple_element_t<0, typename VUtils::Traits::func_traits<F>::args_type>,
    F
>;



template<class T>
class MethodImplLua : public IMethod<T> {
    friend class IModManager;

private:
    sol::protected_function m_func;
    IModManager::Types m_types;

public:
    explicit MethodImplLua(sol::protected_function func, const IModManager::Types& types)
        : m_func(func), 
        m_types(types) {}

    bool Invoke(T t, DataReader &reader) override {
        auto&& state = m_func.lua_state();

        auto results(DataReader::DeserializeLua(state, reader, m_types));

        // Prefix
        //if (!ModManager()->CallEvent(m_categoryHash ^ m_methodHash, sol::as_args(results))) 
            //return;

        sol::protected_function_result result = m_func(t, sol::as_args(results));
        if (!result.valid()) {
            // player invocation was bad
            sol::error error = result;
            throw error;
        }

        // Postfix
        //ModManager()->CallEvent(m_categoryHash ^ m_methodHash ^ IModManager::EVENT_POST, sol::as_args(results))
        if (result.get_type() == sol::type::boolean)
            return result.get<bool>();

        return true;
    }
};

template<typename T>
MethodImplLua(sol::function, std::vector<IModManager::Type>) -> MethodImplLua<T>;
