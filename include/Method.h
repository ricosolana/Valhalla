#pragma once

#include <tuple>
#include <functional>

#include "NetPackage.h"
#include "Utils.h"
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
    // Invoke a function with variadic parameters
    // A copy of the package is made
    // You may perform a move on the package as needed
    virtual void Invoke(T t, NetPackage pkg, HASH_t name) = 0;
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

    void Invoke(T t, NetPackage pkg, HASH_t name) override {
        if constexpr (sizeof...(Args)) {
            auto tuple = NetPackage::Deserialize<Args...>(pkg);

            ModManager::CallEventTuple(name, tuple);

            Utils::InvokeTuple(lambda, object, t, std::move(tuple));
        }
        else {
            // lua
            ModManager::CallEvent(name);

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
    explicit MethodImpl(Lambda lam) : lambda(lam) {}

    void Invoke(T t, NetPackage pkg, HASH_t name) override {
        if constexpr (sizeof...(Args)) {
            auto tuple = NetPackage::Deserialize<Args...>(pkg);

            ModManager::CallEventTuple(name, tuple);

            Utils::InvokeTupleS(lambda, t, tuple);
        }
        else {
            ModManager::CallEvent(name);

            std::invoke(lambda, t);
        }
    }
};

// Specifying deduction guide
template<class T, class ...Args>
MethodImpl(void(*)(T, Args...)) -> MethodImpl<void(*)(T, Args...)>;