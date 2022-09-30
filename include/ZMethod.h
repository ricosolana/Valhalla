#pragma once

#include <tuple>
#include <functional>
#include "ZPackage.h"
#include "Utils.h"
#include <sol/sol.hpp>

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

    // Create a pre-event handler
    //  invoke any lua functions for this particular 
    //  invoke any captured lua handlers

    void Invoke(T t, ZPackage::Ptr pkg) override {
        // If the method has arguments, it requires some special
        // deserialization

        // invoke lua functions
        // so need to 
        // call lua is easy with sol
        auto lua = sol::state();
        lua.open_libraries();
        //lua.new_usertype<ZRpc>("ZRpc",
            //"Invoke", &ZRpc::Invoke); // dummy
        //lua.script("function f() print(\"lua handshake invoked!\") end");
        //sol::function lua_func = lua["f"];

        // this might invoke the lua functor
        
        /*
        * just add manual pre/post events, rpc handlers will have to wait
        * rpc handlers are too much on the side of dynamic catches
        * it introduces complicated indirection in either pre or post rpc event
        * when should it be called in terms of line in rpc?
        * 
        * so just insert event dispatch in rpc callbacks directly for greater intentional control
        * the base game will be too difficult to modify
        */

        if constexpr (sizeof...(Args)) {
            auto tupl = ZPackage::Deserialize<Args...>(pkg);

            // func, self, rpc, ...

            // Should REALLY CONSIDER switching to std function for 
            // functions, as sol supports

            // Pre-rpc lua handler
            //Utils::InvokeTupleS(lua_func, t, tupl);


            // RPC CALL
            Utils::InvokeTuple(lambda, object, t, tupl);

            // Pre-rpc lua handler
            //Utils::InvokeTuple(post_lua_rpc_handler, nullptr, t, tupl);
        }
        else {
            // no unpack needed
            //std::invoke(lua_func, t);
            std::invoke(lambda, object, t);
        }

        // Utils::InvokeTuple(lambda, object, t, tupl);
    }
};
