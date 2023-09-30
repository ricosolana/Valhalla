// main.cpp
#define SOL_ALL_SAFETIES_ON 1

#include "VUtils.h"

#include "VUtilsResource.h"
#include "ValhallaServer.h"
#include "VUtilsRandom.h"
#include "CompileSettings.h"
#include "PrefabManager.h"
#include "ZDOManager.h"

#include "Tests.h"

/*
* Example command line args:
*   .\Valhalla.exe -vmodule=VUtilsResource=1
*   .\Valhalla.exe --no-colors "-vmodule=Peer=2,PrefabManager=2"
*   .\Valhalla.exe --no-log-backup --v=2
*   .\Valhalla.exe -v
*/
int main(int argc, char **argv) {

    fs::current_path("./data/");

    tracy::SetThreadName("main");

    {
        quill::Config cfg;
        cfg.enable_console_colours = true;
        cfg.backend_thread_yield = false;
        cfg.backend_thread_sleep_duration = 1ms;

        //auto&& handler = quill::stdout_handler();
        //handler->set_pattern("%(ascii_time) [%(process)] [%(thread)] %(logger_name) - %(message)", // format
        //    "%D %H:%M:%S.%Qms %z",     // timestamp format
        //    quill::Timezone::GmtTime); // timestamp's timezone
        //
        //cfg.default_handlers.emplace_back(std::move(handler));
        auto&& colours = quill::ConsoleColours();
        colours.set_default_colours();
        cfg.default_handlers.push_back(quill::stdout_handler("colourout", std::move(colours)));

        cfg.default_handlers.push_back(
            quill::time_rotating_file_handler("server.log", "w", quill::FilenameAppend::Date, "daily"));

        //cfg.default_handlers.push_back(quill::file_handler("server.log", "w"));

        quill::configure(cfg);
        quill::start();

        LOGGER = quill::get_logger();
        LOGGER->set_log_level(quill::LogLevel::TraceL3);
    }

#ifdef RUN_TESTS
    fs::current_path("./tests");

    VHTest().Test_ZDO_LoadSave();

    LOG_INFO(LOGGER, "All tests passed!");
#else // !RUN_TESTS

    {
        std::string path = (fs::current_path() / VH_LUA_PATH).string();
        std::string path2 = (fs::current_path() / VH_MOD_PATH).string();
        if (!VUtils::SetEnv("LUA_PATH",
            path + "/?.lua;"
            + path + "/?/?.lua;"
            + path2 + "/?.lua;"
            + path2 + "/?/?.lua"))
            LOG_ERROR(LOGGER, "Failed to set Lua path");
    }

    {
        std::string path = (fs::current_path() / VH_LUA_CPATH).string();
        if (!VUtils::SetEnv("LUA_CPATH",
            path + "/?.dll;"
            + path + "/?/?.dll"))
            LOG_ERROR(LOGGER, "Failed to set Lua cpath");
    }
    
#ifndef _DEBUG
    try {
#endif // _DEBUG
        Valhalla()->Start();
#ifndef _DEBUG
    }
    catch (const std::exception& e) {
        LOG_ERROR(LOGGER, "{}", e.what());
        return 1;
    }
#endif // _DEBUG

    return 0;
#endif // !RUN_TESTS
}
