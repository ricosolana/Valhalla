#pragma once

#include <tuple>
#include <functional>
#include "ZPackage.h"
#include "Utils.h"

/* https://godbolt.org/z/MMGsa8rhr
* to implement deduction guides
* (static functions, no class object needed)
*/

class ZRpc;

// Thanks @Fux
template<class T>
class ZMethodBase
{
public:
    virtual void Invoke(T t, ZPackage::Ptr pkg) = 0;
};

template<class T, class C, class...Args>
class ZMethod final : public ZMethodBase<T> {
    using Lambda = void(C::*)(T, Args...);

    C* object;
    Lambda lambda;

public:
    ZMethod(C* object, Lambda lam) : object(object), lambda(lam) {}

    //template<class T>
    void Invoke(T t, ZPackage::Ptr pkg) override {
        if constexpr (sizeof...(Args)) {
            auto tupl = ZPackage::Deserialize<Args...>(pkg);

            Utils::InvokeTuple(lambda, object, t, tupl);
        }
        else
            std::invoke(lambda, object, t);
    }
};
