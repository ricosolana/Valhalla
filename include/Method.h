#pragma once

#include <tuple>
#include <functional>

#include "NetPackage.h"
#include "Utils.h"
#include "ModManager.h"

enum class NetInvoke {
    RPC,
    ROUTE,
    OBJECT
};

/* https://godbolt.org/z/MMGsa8rhr
* to implement deduction guides
* (static functions, no class object needed)
*/

// Thanks @Fux
template<class T>
class IMethod
{
public:
    virtual void Invoke(T t, NetPackage::Ptr pkg, NetInvoke type, HASH_t hash) = 0;
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

    void Invoke(T t, NetPackage::Ptr pkg, NetInvoke type, HASH_t hash) override {
        if constexpr (sizeof...(Args)) {
            auto copy(pkg);

            auto tupl = NetPackage::Deserialize<Args...>(pkg);
            //auto luaTupl = NetPackage::Deserialize<Args...>(pkg);

            for (auto&& mod : ModManager::GetMods()) {
                if (type == NetInvoke::RPC) {
                    auto&& find = mod->m_rpcCallbacks.find(hash);
                    if (find != mod->m_rpcCallbacks.end())
                        Utils::InvokeTupleS(find->second, t, tupl);
                }
                else if (type == NetInvoke::ROUTE) {
                    auto&& find = mod->m_routeCallbacks.find(hash);
                    if (find != mod->m_routeCallbacks.end())
                        Utils::InvokeTupleS(find->second, t, tupl);
                }
                else if (type == NetInvoke::OBJECT) {
                    auto&& find = mod->m_syncCallbacks.find(hash);
                    if (find != mod->m_syncCallbacks.end())
                        Utils::InvokeTupleS(find->second, t, tupl);
                }
            }

            // RPC CALL
            Utils::InvokeTuple(lambda, object, t, tupl);
        }
        else {
            for (auto&& mod : ModManager::GetMods()) {
                if (type == NetInvoke::RPC) {
                    auto&& find = mod->m_rpcCallbacks.find(hash);
                    if (find != mod->m_rpcCallbacks.end())
                        std::invoke(find->second, t);
                }
                else if (type == NetInvoke::ROUTE) {
                    auto&& find = mod->m_routeCallbacks.find(hash);
                    if (find != mod->m_routeCallbacks.end())
                        std::invoke(find->second, t);
                }
                else if (type == NetInvoke::OBJECT) {
                    auto&& find = mod->m_syncCallbacks.find(hash);
                    if (find != mod->m_syncCallbacks.end())
                        std::invoke(find->second, t);
                }
            }

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

    void Invoke(T t, NetPackage::Ptr pkg, NetInvoke type, HASH_t hash) override {
        if constexpr (sizeof...(Args)) {
            auto tupl = NetPackage::Deserialize<Args...>(pkg);

            // 3 kinds of Rpc use this template class:
            //  ZRpc
            //  ZRoutedRpc
            //  ZNetView

            //sol::function lua_callback;
            //Utils::InvokeTupleS(lua_callback, t, tupl);

            for (auto&& mod : ModManager::GetMods()) {
                if (type == NetInvoke::RPC) {
                    auto&& find = mod->m_rpcCallbacks.find(hash);
                    if (find != mod->m_rpcCallbacks.end())
                        Utils::InvokeTupleS(find->second, t, tupl);
                }
                else if (type == NetInvoke::ROUTE) {
                    auto&& find = mod->m_routeCallbacks.find(hash);
                    if (find != mod->m_routeCallbacks.end())
                        Utils::InvokeTupleS(find->second, t, tupl);
                }
                else if (type == NetInvoke::OBJECT) {
                    auto&& find = mod->m_syncCallbacks.find(hash);
                    if (find != mod->m_syncCallbacks.end())
                        Utils::InvokeTupleS(find->second, t, tupl);
                }
            }

            // RPC CALL
            Utils::InvokeTupleS(lambda, t, tupl);
        }
        else {
            for (auto&& mod : ModManager::GetMods()) {
                if (type == NetInvoke::RPC) {
                    auto&& find = mod->m_rpcCallbacks.find(hash);
                    if (find != mod->m_rpcCallbacks.end())
                        std::invoke(find->second, t);
                }
                else if (type == NetInvoke::ROUTE) {
                    auto&& find = mod->m_routeCallbacks.find(hash);
                    if (find != mod->m_routeCallbacks.end())
                        std::invoke(find->second, t);
                }
                else if (type == NetInvoke::OBJECT) {
                    auto&& find = mod->m_syncCallbacks.find(hash);
                    if (find != mod->m_syncCallbacks.end())
                        std::invoke(find->second, t);
                }
            }

            std::invoke(lambda, t);
        }
    }
};

// Specifying deduction guide
template<class T, class ...Args>
MethodImpl(void(*)(T, Args...)) -> MethodImpl<void(*)(T, Args...)>;