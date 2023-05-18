#pragma once

#include <tuple>
#include <functional>
#include <type_traits>
#include <concepts>

#include "VUtils.h"
#include "VUtilsTraits.h"
#include "DataReader.h"

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
    virtual bool Invoke(T t, DataReader reader) = 0;
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
    MethodImpl(F func, HASH_t categoryHash, HASH_t methodHash)
        : m_func(func), m_categoryHash(categoryHash), m_methodHash(methodHash) {}

    bool Invoke(T t, DataReader reader) override {
        auto tuple = std::tuple_cat(std::forward_as_tuple(t),
            //NetPackage::Deserialize<Args...>(pkg));
            impl_tail<args_type>(reader,
                (std::make_index_sequence < std::tuple_size<args_type>{} - 1 > {})));

        if (reader.Position() != reader.size()) {
            LOG_WARNING(LOGGER, "Peer Rpc Invoke has more data than expected {}/{}", reader.size(), reader.Position());
        }

        bool result = true;
        
        // interestingly gcc requires typename here
        if constexpr (std::is_same_v<bool, typename VUtils::Traits::func_traits<F>::result_type>) {
            result = std::apply(m_func, tuple);
        } else
            std::apply(m_func, tuple);

        return result;
    }
};

template<typename F>
MethodImpl(F, HASH_t, HASH_t) -> MethodImpl<
    std::tuple_element_t<0, typename VUtils::Traits::func_traits<F>::args_type>,
    F
>;
