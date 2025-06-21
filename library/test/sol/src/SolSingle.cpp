#include <exception>
#include <functional>
#include <gtest/gtest.h>

#include <sol/environment.hpp>
#include <sol/forward.hpp>
#include <sol/state.hpp>
#include <sol/table.hpp>
#include <sol/types.hpp>
#include <sol/variadic_args.hpp>
#include <stdexcept>
#include <string_view>

struct Globaline
{
    Globaline()                             = default;
    Globaline(Globaline const &)            = delete;
    Globaline &operator=(Globaline const &) = delete;

    void subscribe()
    {
        std::cout << "Subscribed!\n";
    }
};

//https://sol2.readthedocs.io/en/latest/api/readonly.html
//int

void test_independent_envs_users()
{
    auto globaline = Globaline();

    sol::state state;
    state.open_libraries(sol::lib::base);


    //auto meta_table = state.create_table_with();

    //// Properly self-index metatable to block things
    //meta_table[sol::meta_function::new_index] = deny;
    //meta_table[sol::meta_function::index] = obj_metatable;

    //// Set it on the actual table
    //obj_table[sol::metatable_key] = obj_metatable;

    state.new_usertype<Globaline>(
            "Globaline",
            //"subscribe", [](Globaline &self, sol::variadic_args args, sol::this_environment te) {},
            "subscribe", &Globaline::subscribe, sol::meta_method::static_new_index,
            [](sol::variadic_args) -> sol::object { throw std::runtime_error("nope static"); }

            //sol::meta_method::new_index,
            //[](Globaline &self, sol::variadic_args) -> sol::object {
            //    throw std::runtime_error("nope instance");
            //}


    );

    /*
        First env
    */

    // First assign...
    auto env1  = sol::environment(state, sol::create, state.globals());
    env1["_G"] = env1;
    //env1.new_usertype<Globaline>(
    //        "Globaline", "subscribe",
    //        [](Globaline &self, sol::variadic_args args, sol::this_environment te) {},
    //        sol::meta_method::static_new_index,
    //        [](std::string_view _) -> sol::object { throw std::runtime_error("fuck off"); });

    //decltype(env1)::basic_table_core::g

    env1["globaline"] = std::ref(globaline);

    /*
        Second env
    */

    auto env2  = sol::environment(state, sol::create, state.globals());
    env2["_G"] = env2;
    //env2.new_usertype<Globaline>("Globaline", "subscribe",
    //                             [](Globaline &self, sol::variadic_args args, sol::this_environment te) {});

    env2["globaline"] = std::ref(globaline);

    /*
        Calls
    */

    /*
    // screw it, call multiple times
    state.script("globaline:subscribe();"
                 //"local mt = {__index = function (t) return t.___ end};"
                 //"for k,v in pairs(getmetatable(globaline)) do print(k) end;" //interesting
                 "globaline.subscribe = function(self) print('nefarious') end;"
                 "print('mt' .. type(getmetatable(globaline)['__type']));"
                 "for k,v in pairs(getmetatable(globaline)['__type']) do print('s: ' .. k .. ', v: ' .. "
                 "tostring(v)) end;"
                 //"globaline.subscribe = function(self) print('nefarious') end;"
                 //"setmetatable(Gl)"
                 //"Globaline.subscribe = 'bsi'"
                 //"print('globaline.subscribe: ' .. tostring(globaline.subscribe))"
                 "globaline:subscribe();"
                 //"globaline:nefa()"
                 "globaline.lin = 4;",
                 env1, "test_independent_envs_users_1", sol::load_mode::text);

    state.script("globaline:subscribe();"
                 //"globaline:nefa();"
                 "print('globaline.lin = ' .. tostring(globaline.lin))"
                 //"print('1: globaline.inst' .. tostring(globaline.inst));"
                 //"Globaline.sta = 192;"
                 ,
                 env2, "test_independent_envs_users_2", sol::load_mode::text);
                 */

    try {
        state.safe_script("Globaline.nefa = 'bad'"
                          /* comma */,
                          env1, "test_independent_envs_users_1", sol::load_mode::text);
    } catch (std::exception const &e) {
    }

    state.script("assert(Globaline.nefa == nil)"
                 "print('success #2')"
                 /* comma */,
                 env2, "test_independent_envs_users_2", sol::load_mode::text);
}

TEST(StandaloneSol, PerEnvUserType)
{
    test_independent_envs_users();
}
