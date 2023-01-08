#pragma once

#include <tuple>
#include <functional>

#include "NetPackage.h"
#include "VUtils.h"

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
template<class T, class...V> class MethodImpl;



// Package lambda invoker
template<class T, class...Args>
//class MethodImpl<void(*)(T, Args...)> : public IMethod<T> {
    //using Fn = void(*)(T, Args...);
class MethodImpl<std::function<void(T, Args...)>> : public IMethod<T> {
    using Fn = std::function<void(T, Args...)>;

    const Fn m_lambda;
    const HASH_t m_invokerHash;
    const HASH_t m_methodHash;

public:
    explicit MethodImpl(Fn f,
                        HASH_t invokerHash, // TODO remove defaults
                        HASH_t methodHash)
                        : m_lambda(f),
                          m_invokerHash(invokerHash), m_methodHash(methodHash) {}

    void Invoke(T t, NetPackage pkg) override {
        auto tuple = std::tuple_cat(std::forward_as_tuple(t),
                                    NetPackage::Deserialize<Args...>(pkg));

        if (pkg.m_stream.Position() != pkg.m_stream.Length())
            LOG(ERROR) << "Peer Rpc Invoke has more data than expected " 
                << pkg.m_stream.Length() << "/" << pkg.m_stream.Position();

            std::apply(m_lambda, tuple);

    }
};

// Specifying deduction guide
template<class T, class ...Args>
//MethodImpl(void(*)(T, Args...), HASH_t, HASH_t) -> MethodImpl<void(*)(T, Args...)>;
MethodImpl(std::function<void(T, Args...)>, HASH_t, HASH_t) -> MethodImpl<std::function<void(T, Args...)>>;

//template<class T, class ...Args>
//MethodImpl(void(*)(T, Args...), HASH_t, HASH_t) -> MethodImpl<std::function<void(T, Args...)>>;




//template<class T, class ...Args>
//MethodImpl(sol::function, std::vector<PkgType>) -> MethodImpl<T>;






