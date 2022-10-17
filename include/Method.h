#pragma once

#include <tuple>
#include <functional>
#include "NetPackage.h"
#include "Utils.h"

/* https://godbolt.org/z/MMGsa8rhr
* to implement deduction guides
* (static functions, no class object needed)
*/

// Thanks @Fux
template<class T>
class IMethod
{
public:
    virtual void Invoke(T t, NetPackage::Ptr pkg) = 0;
};

// Base specifier
// Rpc, classes...
template<class T, class...V> class MethodImpl;

// Rpc, instance, function<args...>
template<class T, class C, class...Args>
class MethodImpl<T, C, void(C::*)(T, Args...)> : public IMethod<T> {
    using Lambda = void(C::*)(T, Args...);

    C* object;
    Lambda lambda;

public:
    MethodImpl(C* object, Lambda lam) : object(object), lambda(lam) {}

    void Invoke(T t, NetPackage::Ptr pkg) override {
        if constexpr (sizeof...(Args)) {
            auto tupl = NetPackage::Deserialize<Args...>(pkg);

            // RPC CALL
            Utils::InvokeTuple(lambda, object, t, tupl);
        }
        else {
            std::invoke(lambda, object, t);
        }
    }
};

// Specifying deduction guide
template<class T, class C, class ...Args>
MethodImpl(C* object, void(C::*)(T, Args...)) -> MethodImpl<T, C, void(C::*)(T, Args...)>;

// static specifier
template<class T, class...Args>
class MethodImpl<void(*)(T, Args...)> : public IMethod<T> {
    using Lambda = void(*)(T, Args...);

    Lambda lambda;

public:
    MethodImpl(Lambda lam) : lambda(lam) {}

    void Invoke(T t, NetPackage::Ptr pkg) override {
        if constexpr (sizeof...(Args)) {
            auto tupl = NetPackage::Deserialize<Args...>(pkg);

            // RPC CALL
            Utils::InvokeTupleS(lambda, t, tupl);
        }
        else {
            std::invoke(lambda, t);
        }
    }
};

// Specifying deduction guide
template<class T, class ...Args>
MethodImpl(void(*)(T, Args...)) -> MethodImpl<void(*)(T, Args...)>;