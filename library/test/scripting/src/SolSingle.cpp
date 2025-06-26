#include "ModManager.h"
#include "ValhallaServer.h"
#include <exception>
#include <filesystem>
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

TEST(Scripting, Sandboxing)
{
    std::filesystem::current_path("test/scripting");

    //auto path = std::filesystem::current_path();

    Avledet()->init();

    // run game once
    Avledet()->update();

    // perform LUA test
    //  state test
    ScriptManager()->execute(IScriptManager::ScriptInfo("test1", "chunk1"), ""
                                                                            "");

    // cleanup
    Avledet()->uninit();
}
