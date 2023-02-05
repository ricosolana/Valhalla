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
    // Invoke a function with variadic parameters
    // A copy of the package is made
    // You may perform a move on the package as needed
    // TODO accept a different stream type, readonly, with referenced vector
    virtual void Invoke(T t, DataReader reader) = 0;
};

// Base forward declaration
//template<class T, class...V> class MethodImpl;



// Package lambda invoker
template<class T, typename F> 
    //requires std::same_as<T,
    //std::tuple_element_t<0, typename VUtils::Traits::func_traits<F>::args_type>>
class MethodImpl
    : public IMethod<T> 
{
    //using VUtils::Traits::func_traits;
    //using args_type = typename func_traits<F>::args_type;
    using args_type = typename VUtils::Traits::func_traits<F>::args_type;

    //static_assert(std::is_same<T, std::tuple_element_t<0, args_type>>, "Lambda first type does not match declared type");



    template<class Tuple, size_t... Is> //requires (sizeof...(Is))
    auto impl_tail(DataReader& pkg, std::index_sequence<Is...>) {
        //using Tail = std::tuple<std::tuple_element_t<Is + 1, Tuple>...>;
        return DataReader::Deserialize<std::tuple_element_t<Is + 1u, Tuple>...>(pkg);
        //return NetPackage::Deserialize<Tail>(pkg);
    }

private:
    const F m_func;

public:
    explicit MethodImpl(F func)
                        : m_func(func) {}

    // shouldnt pass around a vector in container
    // wherre only pos/marker increases
    void Invoke(T t, DataReader reader) override {

        

        auto tuple = std::tuple_cat(std::forward_as_tuple(t),
                                    //NetPackage::Deserialize<Args...>(pkg));
                                    impl_tail<args_type>(reader,
                                        (std::make_index_sequence < std::tuple_size<args_type>{} - 1> {})));

        if (reader.Position() != reader.Length())
            LOG(ERROR) << "Peer Rpc Invoke has more data than expected " 
                << reader.Length() << "/" << reader.Position();

        std::apply(m_func, tuple);
    }
};

template<typename F>
MethodImpl(F) ->  MethodImpl<
    std::tuple_element_t<0, typename VUtils::Traits::func_traits<F>::args_type>, 
    F
>;
