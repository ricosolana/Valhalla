#include <functional>
#include <gtest/gtest.h>

#include <sol/environment.hpp>
#include <sol/forward.hpp>
#include <sol/state.hpp>
#include <sol/table.hpp>
#include <sol/types.hpp>
#include <sol/variadic_args.hpp>

struct Globaline
{
    Globaline()                             = default;
    Globaline(Globaline const &)            = delete;
    Globaline &operator=(Globaline const &) = delete;

    void subscribe() {}
};

// Control; state with simple usertype
void test_state_once()
{
    auto globaline = Globaline();

    sol::state state;
    state.open_libraries(sol::lib::base);

    state.new_usertype<Globaline>("Globaline", "subscribe",
                                  [](Globaline &self, sol::variadic_args args, sol::this_environment te) {});

    state["globaline"] = std::ref(globaline);

    state.script("globaline:subscribe()", "test_state_once", sol::load_mode::text);
}

// State global registered usertype ||| env global instance
void test_base_state_global_env_once()
{
    auto globaline = Globaline();

    sol::state state;
    state.open_libraries(sol::lib::base);

    state.new_usertype<Globaline>("Globaline", "subscribe",
                                  [](Globaline &self, sol::variadic_args args, sol::this_environment te) {});

    /*
        Env
    */

    auto env         = sol::environment(state, sol::create, state.globals());
    env["_G"]        = env;
    env["globaline"] = std::ref(globaline);

    /*
        Calls
    */

    state.script("globaline:subscribe()", env, "test_base_state_global_env_once", sol::load_mode::text);
}

void test_base_state_global_env_multiple()
{
    auto globaline = Globaline();

    sol::state state;
    state.open_libraries(sol::lib::base);

    state.new_usertype<Globaline>("Globaline", "subscribe",
                                  [](Globaline &self, sol::variadic_args args, sol::this_environment te) {});

    /*
        First env
    */

    auto env1         = sol::environment(state, sol::create, state.globals());
    env1["_G"]        = env1;
    env1["globaline"] = std::ref(globaline);

    /*
        Second env
    */

    auto env2         = sol::environment(state, sol::create, state.globals());
    env2["_G"]        = env2;
    env2["globaline"] = std::ref(globaline);

    /*
        Calls
    */

    state.script("globaline:subscribe()", env1, "test_base_state_global_env_multiple_1",
                 sol::load_mode::text);

    state.script("globaline:subscribe()", env2, "test_base_state_global_env_multiple_2",
                 sol::load_mode::text);
}

void test_totally_env_multiple()
{
    auto globaline = Globaline();

    sol::state state;
    state.open_libraries(sol::lib::base);

    /*
        First env
    */

    auto env1  = sol::environment(state, sol::create, state.globals());
    env1["_G"] = env1;
    env1.new_usertype<Globaline>("Globaline", "subscribe",
                                 [](Globaline &self, sol::variadic_args args, sol::this_environment te) {});
    env1["globaline"] = std::ref(globaline);

    /*
        Second env
    */

    auto env2  = sol::environment(state, sol::create, state.globals());
    env2["_G"] = env2;
    env2.new_usertype<Globaline>("Globaline", "subscribe",
                                 [](Globaline &self, sol::variadic_args args, sol::this_environment te) {});
    env2["globaline"] = std::ref(globaline);

    /*
        Calls
    */

    state.script("globaline:subscribe()", env1, "test_totally_env_multiple_1", sol::load_mode::text);

    state.script("globaline:subscribe()", env2, "test_totally_env_multiple_2", sol::load_mode::text);
}

void test_state_overwrite_env_multiple()
{
    auto globaline = Globaline();

    sol::state state;
    state.open_libraries(sol::lib::base);

    /*
        First env
    */

    // First assign...
    state.new_usertype<Globaline>("Globaline", "subscribe",
                                  [](Globaline &self, sol::variadic_args args, sol::this_environment te) {});

    auto env1         = sol::environment(state, sol::create, state.globals());
    env1["_G"]        = env1;
    env1["globaline"] = std::ref(globaline);

    /*
        Second env
    */

    // Second, OVERWRITE!   // This statement causes the error below, do NOT know why...
    //state.new_usertype<Globaline>("Globaline", "subscribe",
    //                              [](Globaline &self, sol::variadic_args args, sol::this_environment te) {});

    auto env2         = sol::environment(state, sol::create, state.globals());
    env2["_G"]        = env2;
    env2["globaline"] = std::ref(globaline);

    /*
        Calls
    */

    // screw it, call multiple times
    state.script("globaline:subscribe()"
                 "globaline:subscribe()",
                 env1, "test_state_overwrite_env_multiple_1", sol::load_mode::text);

    state.script("globaline:subscribe()"
                 "globaline:subscribe()",
                 env2, "test_state_overwrite_env_multiple_2", sol::load_mode::text);
}

// Per env usertype registration does not work
void test_independent_envs_users()
{
    auto globaline = Globaline();

    sol::state state;
    state.open_libraries(sol::lib::base);

    /*
        First env
    */

    // First assign...
    auto env1  = sol::environment(state, sol::create, state.globals());
    env1["_G"] = env1;
    env1.new_usertype<Globaline>("Globaline", "subscribe",
                                 [](Globaline &self, sol::variadic_args args, sol::this_environment te) {});

    env1["globaline"] = std::ref(globaline);

    /*
        Second env
    */

    auto env2  = sol::environment(state, sol::create, state.globals());
    env2["_G"] = env2;
    env2.new_usertype<Globaline>("Globaline", "subscribe",
                                 [](Globaline &self, sol::variadic_args args, sol::this_environment te) {});

    env2["globaline"] = std::ref(globaline);

    /*
        Calls
    */

    // screw it, call multiple times
    state.script("globaline:subscribe()"
                 "globaline:subscribe()",
                 env1, "test_independent_envs_users_1", sol::load_mode::text);

    state.script("globaline:subscribe()"
                 "globaline:subscribe()",
                 env2, "test_independent_envs_users_2", sol::load_mode::text);
}

// Obviously fails, because new_usertype is commented out
void test_no_user()
{
    auto globaline = Globaline();

    sol::state state;
    state.open_libraries(sol::lib::base);

    /*
        First env
    */

    // First assign...
    auto env1  = sol::environment(state, sol::create, state.globals());
    env1["_G"] = env1;
    //env1.new_usertype<Globaline>("Globaline", "subscribe",
    //                             [](Globaline &self, sol::variadic_args args, sol::this_environment te) {});

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

    // screw it, call multiple times
    state.script("globaline:subscribe()"
                 "globaline:subscribe()",
                 env1, "test_no_user_1", sol::load_mode::text);

    state.script("globaline:subscribe()"
                 "globaline:subscribe()",
                 env2, "test_no_user_2", sol::load_mode::text);
}

void test_env_user_isolation()
{
    auto globaline = Globaline();

    sol::state state;
    state.open_libraries(sol::lib::base);

    state.new_usertype<Globaline>("Globaline", "static_var", sol::var("bup"), "subscribe",
                                  [](Globaline &self, sol::variadic_args args, sol::this_environment te) {});

    /*
        First env
    */

    auto env1         = sol::environment(state, sol::create, state.globals());
    env1["_G"]        = env1;
    env1["globaline"] = std::ref(globaline);

    /*
        Second env
    */

    auto env2         = sol::environment(state, sol::create, state.globals());
    env2["_G"]        = env2;
    env2["globaline"] = std::ref(globaline);

    /*
        Calls
    */

    state.script("globaline:subscribe();"
                 //"Globaline.static_var = nil"
                 //"for k, v in pairs(_G) do print(k) end;" //globaline, _G
                 //"for k, v in pairs(_G['_G']) do print(k) end;" //^ same thing!
                 //"sol.Globaline.static_var = nil;" //error, maybe I typo'd... or sol table is invisible...
                 //"print('Type: ' .. type(globaline));"//Type: userdata
                 //"for k, v in pairs(_G['globaline']) do print(k) end;" //error
                 "print('1: globaline.static_var = ' .. globaline.static_var)"
                 "globaline.static_var = 'OVERWROTE!';",

                 //"globaline.static_var = nil;", //complaining 'expected str, got nil...'
                 env1, "test_env_user_isolation_1", sol::load_mode::text);

    state.script("globaline:subscribe();"
                 //"assert(Globaline.static_var ~= nil);",
                 "print('2: globaline.static_var = ' .. globaline.static_var)",
                 env2, "test_env_user_isolation_2", sol::load_mode::text);
}

void test_env_user_perfect_isolation()
{
    auto globaline = Globaline();

    sol::state state;
    state.open_libraries(sol::lib::base);


    /*
        First env
    */
    auto table1     = sol::table(state, sol::create);
    table1["print"] = state["print"];
    //table1.new_usertype<Globaline>("Globaline", "static_var", sol::var("bup"), "subscribe",
    //                               [](Globaline &self, sol::variadic_args args, sol::this_environment te) {});

    auto env1         = sol::environment(state, sol::create, table1);
    env1["_G"]        = env1;
    env1["globaline"] = std::ref(globaline);
    env1.new_usertype<Globaline>("Globaline", "static_var", sol::var("bup"), "subscribe",
                                 [](Globaline &self, sol::variadic_args args, sol::this_environment te) {});

    /*
        Second env
    */

    auto table2     = sol::table(state, sol::create);
    table2["print"] = state["print"];
    table2.new_usertype<Globaline>("Globaline", "static_var", sol::var("bup"), "subscribe",
                                   [](Globaline &self, sol::variadic_args args, sol::this_environment te) {});

    auto env2         = sol::environment(state, sol::create, table2);
    env2["_G"]        = env2;
    env2["globaline"] = std::ref(globaline);

    /*
        Calls
    */

    state.script("globaline:subscribe();"
                 //"Globaline.static_var = nil"
                 //"for k, v in pairs(_G) do print(k) end;" //globaline, _G
                 //"for k, v in pairs(_G['_G']) do print(k) end;" //^ same thing!
                 //"sol.Globaline.static_var = nil;" //error, maybe I typo'd... or sol table is invisible...
                 //"print('Type: ' .. type(globaline));"//Type: userdata
                 //"for k, v in pairs(_G['globaline']) do print(k) end;" //error
                 "print('1: globaline.static_var = ' .. globaline.static_var)"
                 "globaline.static_var = 'OVERWROTE!';",

                 //"globaline.static_var = nil;", //complaining 'expected str, got nil...'
                 env1, "test_env_user_perfect_isolation_1", sol::load_mode::text);

    state.script("globaline:subscribe();"
                 //"assert(Globaline.static_var ~= nil);",
                 "print('2: globaline.static_var = ' .. globaline.static_var)",
                 env2, "test_env_user_perfect_isolation_2", sol::load_mode::text);
}

TEST(StandaloneSol, UserTypeEnvGlobalTest)
{
    test_state_once();
    test_base_state_global_env_once();
    test_base_state_global_env_multiple();
    //test_totally_env_multiple();    // Error; envs do not like registering .usertype
    test_state_overwrite_env_multiple();
    test_independent_envs_users();//Error; same env user register error
    //test_no_user();   //Good, expected to fail, because no def for usertype
    test_env_user_isolation();
    test_env_user_perfect_isolation();
}
