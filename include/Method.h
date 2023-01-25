#pragma once

#include <tuple>
#include <functional>
#include <type_traits>
#include <concepts>

#include "NetPackage.h"
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
    // Invoke a function with variadic parameters
    // A copy of the package is made
    // You may perform a move on the package as needed
    // TODO accept a different stream type, readonly, with referenced vector
    virtual void Invoke(T t, NetPackage pkg) = 0;
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
    auto impl_tail(NetPackage& pkg, std::index_sequence<Is...>) {
        //using Tail = std::tuple<std::tuple_element_t<Is + 1, Tuple>...>;
        return NetPackage::Deserialize<std::tuple_element_t<Is + 1u, Tuple>...>(pkg);
        //return NetPackage::Deserialize<Tail>(pkg);
    }

    //template < std::size_t... Ns, typename... Ts >
    //auto tail_impl(std::index_sequence<Ns...>, std::tuple<Ts...> t)
    //{
    //    return  std::make_tuple(std::get<Ns + 1u>(t)...);
    //}
    //
    //template < typename... Ts >
    //auto tail(std::tuple<Ts...> t)
    //{
    //    return  tail_impl(std::make_index_sequence<sizeof...(Ts) - 1u>(), t);
    //}

private:
    const F m_func;

public:
    explicit MethodImpl(F func)
                        : m_func(func) {}

    // shouldnt pass around a vector in container
    // wherre only pos/marker increases
    void Invoke(T t, NetPackage pkg) override {

        

        auto tuple = std::tuple_cat(std::forward_as_tuple(t),
                                    //NetPackage::Deserialize<Args...>(pkg));
                                    impl_tail<args_type>(pkg,
                                        (std::make_index_sequence < std::tuple_size<args_type>{} - 1> {})));

        if (pkg.m_stream.Position() != pkg.m_stream.Length())
            LOG(ERROR) << "Peer Rpc Invoke has more data than expected " 
                << pkg.m_stream.Length() << "/" << pkg.m_stream.Position();

        std::apply(m_func, tuple);
    }
};

template<typename F>
MethodImpl(F) ->  MethodImpl<
    std::tuple_element_t<0, typename VUtils::Traits::func_traits<F>::args_type>, 
    F
>;

// Specifying deduction guide
//template<class T, class ...Args>
////MethodImpl(void(*)(T, Args...), HASH_t, HASH_t) -> MethodImpl<void(*)(T, Args...)>;
//MethodImpl(std::function<void(T, Args...)>, HASH_t, HASH_t) -> MethodImpl<std::function<void(T, Args...)>>;

//template<class T, class ...Args>
//MethodImpl(void(*)(T, Args...), HASH_t, HASH_t) -> MethodImpl<std::function<void(T, Args...)>>;




//template<class T, class ...Args>
//MethodImpl(sol::function, std::vector<PkgType>) -> MethodImpl<T>;






