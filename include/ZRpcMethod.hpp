#pragma once

#include <tuple>
#include <functional>
#include "ZPackage.hpp"
#include "Utils.hpp"

/* https://godbolt.org/z/MMGsa8rhr
* to implement deduction guides
* (static functions, no class object needed)
*/

class ZRpc;

// Thanks @Fux
template <class C, class Tuple, class F, size_t... Is>
constexpr auto invoke_tuple_impl(F f, C& c, ZRpc* rpc, Tuple t,
    std::index_sequence<Is...>) {
    return std::invoke(f, c, rpc, std::get<Is>(t)...);
}

template <class C, class Tuple, class F>
constexpr auto invoke_tuple(F f, C& c, ZRpc* rpc, Tuple t) {
    return invoke_tuple_impl(f, c, rpc, t, std::make_index_sequence < std::tuple_size<Tuple>{} > {});
}

class ZRpcMethodBase
{
public:
    virtual void Invoke(ZRpc* pao, ZPackage &pkg) = 0;
};

template<class C, class...Args>
class ZRpcMethod final : public ZRpcMethodBase {
    using Lambda = void(C::*)(ZRpc*, Args...);

    C* object;
    Lambda lambda;

    template<class F>
    auto Invoke_impl(ZPackage &pkg) {
        //F f; p->Read(f);
        F f = pkg.Read<F>();
        std::tuple<F> a{ f };
        return a;
    }

    // Split a param,
    // Add that param from Packet into tuple
    template<class F, class S, class...R>
    auto Invoke_impl(ZPackage &pkg) {
        auto f = pkg.Read<F>();
        std::tuple<F> a{ f };
        std::tuple<S, R...> b = Invoke_impl<S, R...>(pkg);
        return std::tuple_cat(a, b);
    }

public:
    ZRpcMethod(C* object, Lambda lam) : object(object), lambda(lam) {}

    void Invoke(ZRpc* rpc, ZPackage &pkg) override {
        // Invoke_impl returns a tuple of types by recursion
        if constexpr (sizeof...(Args))
        {
            auto tupl = Invoke_impl<Args...>(pkg);
            invoke_tuple(lambda, object, rpc, tupl);
        }
        else
        {
            // works like ~magic~
            std::invoke(lambda, object, rpc);
        }
    }
};
