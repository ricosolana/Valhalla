#pragma once

#include <tuple>
#include <functional>

#include "NetPackage.h"
#include "VUtils.h"
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
    virtual void Invoke(T t, NetPackage pkg) = 0;
};

// Base specifier
// Rpc, classes...
template<class T, class...V> class MethodImpl;

// Rpc, instance, function<args...>
// Lua notes*
//  Register an event handler like:
//      OnEvent("Rpc", "PeerInfo", "POST")
template<class T, class C, class...Args>
class MethodImpl<T, C, void(C::*)(T, Args...)> : public IMethod<T> {
    using FuncPtr = void(C::*)(T, Args...);

    const C* object;
    const FuncPtr lambda;
    const HASH_t m_invokerHash;
    const HASH_t m_methodHash;

public:
    MethodImpl(C* object, FuncPtr lam,
               HASH_t invokerHash = 0,
               HASH_t methodHash = 0)
            : object(object), lambda(lam),
              m_invokerHash(invokerHash), m_methodHash(methodHash) {}
    // TODO add global invoke calls (i.e. invoke all lua rpcs events)

    void Invoke(T t, NetPackage pkg) override {
        //assert(false);
        if constexpr (sizeof...(Args)) {
            auto tuple = std::tuple_cat(std::forward_as_tuple(t),
                                        NetPackage::Deserialize<Args...>(pkg));

            // Pre events
            CALL_EVENT_TUPLE(m_invokerHash, tuple);
            auto result = CALL_EVENT_TUPLE(m_invokerHash ^ m_methodHash, tuple);

            if (result != EventStatus::CANCEL)
                std::apply(lambda, std::tuple_cat(std::forward_as_tuple(object),
                                                  tuple));

            // Post events
            CALL_EVENT_TUPLE(m_invokerHash ^ EVENT_HASH_POST, tuple);
            CALL_EVENT_TUPLE(m_invokerHash ^ m_methodHash ^ EVENT_HASH_POST, tuple);
        }
        else {

            // lua
            //auto result =
            //if (CALL_EVENT(m_luaEventHash) != EventStatus::CANCEL)
            //    std::invoke(lambda, object, t);
            //CALL_EVENT(m_luaEventHash ^ EVENT_HASH_POST);
        }
    }
};

// Specifying deduction guide
template<class T, class C, class ...Args>
MethodImpl(C* object, void(C::*)(T, Args...), HASH_t) -> MethodImpl<T, C, void(C::*)(T, Args...)>;



// static specifier
template<class T, class...Args>
class MethodImpl<void(*)(T, Args...)> : public IMethod<T> {
    using FuncPtr = void(*)(T, Args...);

    const FuncPtr lambda;
    const HASH_t m_luaEventHash;

public:
    explicit MethodImpl(FuncPtr lam, HASH_t luaEventHash) : lambda(lam), m_luaEventHash(luaEventHash) {}

    void Invoke(T t, NetPackage pkg) override {
        assert(false);
        if constexpr (sizeof...(Args)) {
            auto tuple = std::tuple_cat(std::forward_as_tuple(t),
                                        NetPackage::Deserialize<Args...>(pkg));

            if (CALL_EVENT_TUPLE(m_luaEventHash, tuple) != EventStatus::CANCEL)
                std::apply(lambda, tuple);
            CALL_EVENT_TUPLE(m_luaEventHash ^ EVENT_HASH_POST, tuple);
        }
        else {
            if (CALL_EVENT(m_luaEventHash) != EventStatus::CANCEL)
                std::invoke(lambda, t);
            CALL_EVENT(m_luaEventHash ^ EVENT_HASH_POST);
        }
    }
};

// Specifying deduction guide
template<class T, class ...Args>
MethodImpl(void(*)(T, Args...), HASH_t) -> MethodImpl<void(*)(T, Args...)>;



// Lua callbacks
template<class T>
class MethodImpl<T> : public IMethod<T> {
    sol::function m_func;
    std::vector<PkgType> m_types;

public:
    explicit MethodImpl(sol::function func, std::vector<PkgType> types)
        : m_func(std::move(func)), m_types(std::move(types)) {}

    void Invoke(T t, NetPackage pkg) override {
        sol::variadic_results results;
        auto&& state(m_func.lua_state());

        // Pass the rpc always
        results.push_back(sol::make_object(state, t));

        for (auto&& pkgType : m_types) {
            switch (pkgType) {
                case PkgType::BYTE_ARRAY:
                    // Will be interpreted as sol container type
                    // see https://sol2.readthedocs.io/en/latest/containers.html
                    results.push_back(sol::make_object(state, pkg.Read<BYTES_t>()));
                    break;
                case PkgType::PKG:
                    results.push_back(sol::make_object(state, pkg.Read<NetPackage>()));
                    break;
                case PkgType::STRING:
                    // Primitive: string
                    results.push_back(sol::make_object(state, pkg.Read<std::string>()));
                    break;
                case PkgType::NET_ID:
                    results.push_back(sol::make_object(state, pkg.Read<NetID>()));
                    break;
                case PkgType::VECTOR3:
                    results.push_back(sol::make_object(state, pkg.Read<Vector3>()));
                    break;
                case PkgType::VECTOR2i:
                    results.push_back(sol::make_object(state, pkg.Read<Vector2i>()));
                    break;
                case PkgType::QUATERNION:
                    results.push_back(sol::make_object(state, pkg.Read<Quaternion>()));
                    break;
                case PkgType::STRING_ARRAY:
                    // Container type of Primitive: string
                    results.push_back(sol::make_object(state, pkg.Read<std::vector<std::string>>()));
                    break;
                case PkgType::BOOL:
                    // Primitive: boolean
                    results.push_back(sol::make_object(state, pkg.Read<bool>()));
                    break;
                case PkgType::INT8:
                    // Primitive: number
                    results.push_back(sol::make_object(state, pkg.Read<int8_t>()));
                    break;
                case PkgType::UINT8:
                    // Primitive: number
                    results.push_back(sol::make_object(state, pkg.Read<uint8_t>()));
                    break;
                case PkgType::INT16:
                    // Primitive: number
                    results.push_back(sol::make_object(state, pkg.Read<int16_t>()));
                    break;
                case PkgType::UINT16:
                    // Primitive: number
                    results.push_back(sol::make_object(state, pkg.Read<uint16_t>()));
                    break;
                case PkgType::INT32:
                    // Primitive: number
                    results.push_back(sol::make_object(state, pkg.Read<int32_t>()));
                    break;
                case PkgType::UINT32:
                    // Primitive: number
                    results.push_back(sol::make_object(state, pkg.Read<uint32_t>()));
                    break;
                case PkgType::INT64:
                    // Primitive: number
                    results.push_back(sol::make_object(state, pkg.Read<int64_t>()));
                    break;
                case PkgType::UINT64:
                    // Primitive: number
                    results.push_back(sol::make_object(state, pkg.Read<uint64_t>()));
                    break;
                case PkgType::FLOAT:
                    // Primitive: number
                    results.push_back(sol::make_object(state, pkg.Read<float>()));
                    break;
                case PkgType::DOUBLE:
                    // Primitive: number
                    results.push_back(sol::make_object(state, pkg.Read<double>()));
                    break;
            }
        }

        m_func(results);

        //auto args(sol::as_args(results));
        //m_func(args);
    }
};

//template<class T, class ...Args>
//MethodImpl(sol::function, std::vector<PkgType>) -> MethodImpl<T>;






