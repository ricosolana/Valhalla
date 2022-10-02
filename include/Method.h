#pragma once

#include <tuple>
#include <functional>
#include "NetPackage.h"
#include "Utils.h"
//#include <sol/sol.hpp>

/* https://godbolt.org/z/MMGsa8rhr
* to implement deduction guides
* (static functions, no class object needed)
*/

// Base specifier for later deductions
//template<class...>
//class ZMethodBase;

// Thanks @Fux
template<class T>
class ZMethodBase
{
public:
    virtual void Invoke(T t, ZPackage::Ptr pkg) = 0;
};

// Base specifier
// Rpc, templates...
template<class T, class...V> class ZMethod;

// Rpc, instance, function<args...>
template<class T, class C, class...Args>
class ZMethod<T, C, void(C::*)(T, Args...)> : public ZMethodBase<T> {
    using Lambda = void(C::*)(T, Args...);

    C* object;
    Lambda lambda;

public:
    ZMethod(C* object, Lambda lam) : object(object), lambda(lam) {}

    void Invoke(T t, ZPackage::Ptr pkg) override {
        if constexpr (sizeof...(Args)) {
            auto tupl = ZPackage::Deserialize<Args...>(pkg);

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
ZMethod(C* object, void(C::*)(T, Args...)) -> ZMethod<T, C, void(C::*)(T, Args...)>;

// static specifier
template<class T, class...Args>
class ZMethod<void(*)(T, Args...)> : public ZMethodBase<T> {
    using Lambda = void(*)(T, Args...);

    Lambda lambda;

public:
    ZMethod(Lambda lam) : lambda(lam) {}

    void Invoke(T t, ZPackage::Ptr pkg) override {
        if constexpr (sizeof...(Args)) {
            auto tupl = ZPackage::Deserialize<Args...>(pkg);

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
ZMethod(void(*)(T, Args...)) -> ZMethod<void(*)(T, Args...)>;