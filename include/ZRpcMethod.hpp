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

public:
    ZRpcMethod(C* object, Lambda lam) : object(object), lambda(lam) {}

    void Invoke(ZRpc* rpc, ZPackage &pkg) override {
        if constexpr (sizeof...(Args)) {
            auto tupl = ZPackage::Deserialize<Args...>(pkg);

            Utils::InvokeTuple(lambda, object, rpc, tupl);
        }
        else
            std::invoke(lambda, object, rpc);
    }
};
