#include <exception>
#include <filesystem>
#include <functional>
#include <quill/core/LogLevel.h>
#include <sol/property.hpp>
#include <sol/raii.hpp>
#include <stdexcept>
#include <string_view>

#include <gtest/gtest.h>

#include <sol/environment.hpp>
#include <sol/forward.hpp>
#include <sol/state.hpp>
#include <sol/table.hpp>
#include <sol/types.hpp>
#include <sol/variadic_args.hpp>

struct A
{
    int tamper_proof = 23;
};

struct Box
{
    A super_secret;
};

TEST(Scripting, ReadOnlySubImmutable)
{
    Box box;

    sol::state state;
    state.open_libraries(sol::lib::base);

    // clang-format off

    state.new_usertype<A>("A",
        "tamper_proof", &A::tamper_proof
    );

    state.new_usertype<Box>("Box",
        "super_secret", sol::readonly(&Box::super_secret), //(1)
        "true_super_secret", [](Box& self) { return self.super_secret; }
    );

    //testing if immutable
    state["box"] = std::ref(box);

    ASSERT_NO_THROW({
        state.script("box.super_secret.tamper_proof = 4");
    });

    ASSERT_ANY_THROW({
        state.script("box.true_super_secret.tamper_proof = 4");
    });

    //ASSERT_EQ(box.true_super_secret.tamper_proof, 23);
}
