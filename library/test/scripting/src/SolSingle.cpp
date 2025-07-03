#include <exception>
#include <filesystem>
#include <functional>
#include <quill/core/LogLevel.h>
#include <stdexcept>
#include <string_view>

#include <gtest/gtest.h>

#include <sol/environment.hpp>
#include <sol/forward.hpp>
#include <sol/state.hpp>
#include <sol/table.hpp>
#include <sol/types.hpp>
#include <sol/variadic_args.hpp>

#include "Avledet.h"
#include "ModManager.h"
#include "ZDO.h"

TEST(Scripting, blah)
{

    // server.yml logging is none until later, for easier test

    std::filesystem::current_path("test/scripting");

    //auto path = std::filesystem::current_path();

    Avledet()->init();

    // run game once
    Avledet()->update();

    AVL_LOGGER->set_log_level(quill::LogLevel::Info);

    std::string code = ""
                       "";

    // perform LUA test
    //  state test
    ScriptManager()->execute(IScriptManager::ScriptInfo("test1", "chunk1", ""), code, false);

    // cleanup
    Avledet()->uninit();
}
