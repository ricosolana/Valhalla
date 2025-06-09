// main.cpp
#include <filesystem>

#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/Logger.h>
#include <quill/backend/BackendOptions.h>
#include <quill/core/LogLevel.h>
#include <quill/core/PatternFormatterOptions.h>
#include <quill/sinks/FileSink.h>
#include <quill/sinks/RotatingFileSink.h>
#include <quill/sinks/ConsoleSink.h>
#include <quill/std/FilesystemPath.h>

#include <tracy/Tracy.hpp>

#define SOL_ALL_SAFETIES_ON 1

// this doesnt seem to do a thing
//#undef TRACY_ENABLE

#include "ValhallaServer.h"
#include "CompileSettings.h"

//#include "Tests.h"

/*
* Example command line args:
*   .\Valhalla.exe -vmodule=VUtilsResource=1
*   .\Valhalla.exe --no-colors "-vmodule=Peer=2,PrefabManager=2"
*   .\Valhalla.exe --no-log-backup --v=2
*   .\Valhalla.exe -v
*/

quill::Logger* VH_LOGGER {};

int main(int argc, char **argv) {
    tracy::SetThreadName("main");
    
    std::filesystem::current_path("./data/");

    {
        quill::BackendOptions options;
        options.enable_yield_when_idle = false;
        options.sleep_duration = 1ms;

        quill::Backend::start(options);
    }

    {
        auto sink = quill::Frontend::create_or_get_sink<quill::FileSink>(
            "server.log",
            [](){
                // See RotatingFileSinkConfig for more options

                quill::FileSinkConfig cfg;

                cfg.set_open_mode('w');
                cfg.set_filename_append_option(quill::FilenameAppendOption::StartDateTime);
                //cfg.set_rotation_time_daily("24:00");
                //cfg.set_rotation_max_file_size(1024); // small value to demonstrate the example

                return cfg;
            }());

        quill::PatternFormatterOptions options {
            "%(time) [%(thread_name)] %(short_source_location) %(log_level) %(message)", // format
            "%D %H:%M:%S.%Qms %z",                                              // timestamp format
            quill::Timezone::GmtTime
        };

        auto logger = quill::Frontend::create_or_get_logger(
            "main", std::move(sink),
            options
        );

        logger->set_log_level(quill::LogLevel::TraceL3);

        /* global assigned */ VH_LOGGER = logger;
    }

#ifdef RUN_TESTS
    fs::current_path("./tests");

    VHTest().Test_ZDO_LoadSave();

    LOG_INFO(VH_LOGGER, "All tests passed!");
#else // !RUN_TESTS

/*
    // I think ONLY windows requires this...
    {
        std::string path = (fs::current_path() / VH_LUA_PATH).string();
        std::string path2 = (fs::current_path() / VH_MOD_PATH).string();
        if (!VUtils::SetEnv("LUA_PATH",
            path + "/?.lua;"
            + path + "/?/?.lua;"
            + path2 + "/?.lua;"
            + path2 + "/?/?.lua"))
            LOG_ERROR(VH_LOGGER, "Failed to set Lua path");
    }

    {
        std::string path = (fs::current_path() / VH_LUA_CPATH).string();
        if (!VUtils::SetEnv("LUA_CPATH",
            path + "/?.dll;"
            + path + "/?/?.dll"))
            LOG_ERROR(VH_LOGGER, "Failed to set Lua cpath");
    }
*/


    
#ifndef _DEBUG
    try {
#endif // _DEBUG
        Valhalla()->Start();
#ifndef _DEBUG
    }
    catch (const std::exception& e) {
        LOG_ERROR(VH_LOGGER, "{}", e.what());
        return 1;
    }
#endif // _DEBUG

    return 0;
#endif // !RUN_TESTS
}
