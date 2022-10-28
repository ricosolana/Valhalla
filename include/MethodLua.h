#pragma once

#include <tuple>
#include <functional>
#include <sol/sol.hpp>

#include "NetPackage.h"
#include "Utils.h"
#include "ModManager.h"

template<class T>
class IMethodLua
{
public:
    // Invoke a function with variadic parameters
    // A copy of the package is made
    // You may perform a move on the package as needed
    virtual void Invoke(T t, NetPackage pkg, std::vector<ModCallback>& callbacks) = 0;
};

// Base specifier
// Rpc, classes...
template<class T, class...V> class MethodLuaImpl;

// static specifier
template<class T, class...Args>
class MethodLuaImpl<void(*)(T, Args...)> : public IMethodLua<T> {
public:
    void Invoke(T t, NetPackage pkg, std::vector<ModCallback>& callbacks) override {
        if constexpr (sizeof...(Args)) {
            auto tuple = NetPackage::Deserialize<Args...>(pkg);

            for (auto&& pair : callbacks) {
                Utils::InvokeTupleS(pair.second.first, t, tuple);
            }
        }
        else {
            for (auto&& pair : callbacks) {
                std::invoke(pair.second.first, t);
            }
        }
    }
};
