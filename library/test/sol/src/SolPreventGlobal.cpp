#include <functional>
#include <gtest/gtest.h>

#include <sol/environment.hpp>
#include <sol/forward.hpp>
#include <sol/property.hpp>
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

    std::string readonly_member;
};

namespace sol {
    // TODO test this
    //  Usage is intended for readonly / immutable static constants

    //template<typename R, typename W>
    //struct constant_wrapper : detail::ebco<R, 0>
    //{
    //  private:
    //    using read_base_t = detail::ebco<R, 0>;

    //  public:
    //    template<typename Rx>
    //    constant_wrapper(Rx &&r) :
    //        read_base_t(std::forward<Rx>(r))
    //    {
    //    }

    //    R &read()
    //    {
    //        return read_base_t::value();
    //    }

    //    R const &read() const
    //    {
    //        return read_base_t::value();
    //    }
    //};

    template<typename T>
    struct constant_wrapper : detail::ebco<T>
    {
      private:
        using base_t = detail::ebco<T>;

      public:
        using base_t::base_t;

        //operator T &()
        //{
        //    return base_t::value();
        //}
        //
        //operator T const &() const
        //{
        //    return base_t::value();
        //}
    };

    template<typename V>
    inline auto constant(V &&v)
    {
        //return readonly_wrapper<meta::unqualified_t<decltype(v)>>(v);

        //typedef std::decay_t<V> T;
        //return readonly_wrapper<T>(std::forward<V>(v));

        //typedef meta::unqualified_t<decltype(v)> T;
        //return readonly_wrapper<T>(std::forward<V>(v));

        using T = std::decay_t<V>;
        return constant_wrapper<T>(std::forward<V>(v));
    }


}// namespace sol

//https://sol2.readthedocs.io/en/latest/api/readonly.html
//int

void test_no_globals()
{
    sol::state state;
    state.open_libraries(sol::lib::base);

    //auto meta_table = state.create_table_with();

    //// Properly self-index metatable to block things
    //meta_table[sol::meta_function::new_index] = deny;
    //meta_table[sol::meta_function::index] = obj_metatable;

    //// Set it on the actual table
    //obj_table[sol::metatable_key] = obj_metatable;

    static constexpr char const *str = "IM IMMUTABLE CONSTANT!!!";

    //state["the_constant"] = sol::constant(str);

    state["prop_constant"] = sol::property([]() { return 109; });

    state.new_usertype<Globaline>(
            "Globaline",
            //"subscribe", [](Globaline &self, sol::variadic_args args, sol::this_environment te) {},
            "subscribe", &Globaline::subscribe,

            //sol::meta_method::static_new_index, [](sol::variadic_args) -> sol::object { throw std::runtime_error("nope static"); }
            "my_con", sol::var1("FOREVER STILL")// USES FIRST ebco<> deduciton
                                                // comment
                                                ///"my_real_con", sol::constant("YES CONST") // why brokne
            ////"prop_constant", sol::property([]() { return 67; }), // USES TAGGED 1st ebco (class-based)
            /////"constant1", sol::readonly_property("myconst")
            //sol::meta_method::new_index,
            //[](Globaline &self, sol::variadic_args) -> sol::object {
            //    throw std::runtime_error("nope instance");
            //}

            //"readonly_prop",
            //sol::readonly(
            //        &Globaline::
            //                readonly_member)
    );//yup, readonly uses first ebco, but with overridden/loaded value()...?

    /*
        First env
    */

    // First assign...
    auto env1  = sol::environment(state, sol::create, state.globals());
    env1["_G"] = env1;


    /*
        Second env
    */

    auto env2  = sol::environment(state, sol::create, state.globals());
    env2["_G"] = env2;


    /*
        Calls
    */

    // These actually do globally sandbox!!!
    //state.script("my_global = 192;"
    //             "_G['my_global_global'] = 83;"
    //             "print('[env1] my_global: ' .. tostring(my_global));"
    //             "print('[env1] my_global_global: ' .. my_global_global);"
    //             /* comma */,
    //             env1, "test_no_globals_ENV1", sol::load_mode::text);

    //state.script("print('[env2] my_global: ' .. tostring(my_global));"
    //             "print('[env2] my_global_global: ' .. tostring(_G['my_global_global']));"
    //             /* comma */,
    //             env2, "test_no_globals_ENV2", sol::load_mode::text);

    state.script(//"print('[global] prop_constant: ' .. tostring(prop_constant));"
            //"prop_constant = 'no longer constant...';"
            //"print('[global] prop_constant: ' .. tostring(prop_constant));"
            // Globaline...
            "print('[global] Globaline.prop_constant: ' .. tostring(Globaline.prop_constant));"
            "Globaline.prop_constant = 'no longer constant...';"
            "print('[global] Globaline.prop_constant: ' .. tostring(Globaline.prop_constant));"

            /* comma */,
            "test_no_globals_ENV2", sol::load_mode::text);
}

TEST(StandaloneSol, PreventGlobal)
{
    test_no_globals();
}
