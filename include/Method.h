#pragma once

#include <tuple>
#include <functional>

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
    virtual void Invoke(T t, NetPackage pkg) = 0;
};

// Base forward declaration
//template<class T, class...V> class MethodImpl;



// Package lambda invoker
template<class T, typename F> //, class...Args>
//class MethodImpl<void(*)(T, Args...)> : public IMethod<T> {
    //using Fn = void(*)(T, Args...);
class MethodImpl //<std::function<void(T, Args...)>> 
    : public IMethod<T> 
{
    //using F = std::function<void(T, Args...)>;

    template<class Tuple, size_t... Is>
    Tuple impl(NetPackage& pkg, std::index_sequence<Is...>) {
        return NetPackage::Deserialize<std::tuple_element_t<Is, Tuple>...>(pkg);
    }

private:
    const F m_func;

public:
    explicit MethodImpl(F func)
                        : m_func(func) {}

    void Invoke(T t, NetPackage pkg) override {

        using VUtils::Traits;
        using args_type = typename func_traits<F>::args_type;

        auto tuple = std::tuple_cat(std::forward_as_tuple(t),
                                    //NetPackage::Deserialize<Args...>(pkg));
                                    impl<args_type>(pkg, std::make_index_sequence < std::tuple_size<args_type>{} > {}));

        if (pkg.m_stream.Position() != pkg.m_stream.Length())
            LOG(ERROR) << "Peer Rpc Invoke has more data than expected " 
                << pkg.m_stream.Length() << "/" << pkg.m_stream.Position();

        std::apply(m_func, tuple);
    }
};

// Specifying deduction guide
//template<class T, class ...Args>
////MethodImpl(void(*)(T, Args...), HASH_t, HASH_t) -> MethodImpl<void(*)(T, Args...)>;
//MethodImpl(std::function<void(T, Args...)>, HASH_t, HASH_t) -> MethodImpl<std::function<void(T, Args...)>>;

//template<class T, class ...Args>
//MethodImpl(void(*)(T, Args...), HASH_t, HASH_t) -> MethodImpl<std::function<void(T, Args...)>>;




//template<class T, class ...Args>
//MethodImpl(sol::function, std::vector<PkgType>) -> MethodImpl<T>;






