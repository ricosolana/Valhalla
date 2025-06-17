// main.cpp
#include <filesystem>

#include <memory>
#include <quill/Backend.h>
#include <quill/backend/BackendOptions.h>
#include <quill/core/LogLevel.h>
#include <quill/core/PatternFormatterOptions.h>
#include <quill/filters/Filter.h>
#include <quill/Frontend.h>
#include <quill/Logger.h>
#include <quill/sinks/ConsoleSink.h>
#include <quill/sinks/FileSink.h>
#include <quill/sinks/RotatingFileSink.h>
#include <quill/sinks/RotatingSink.h>
#include <quill/sinks/Sink.h>
#include <quill/std/FilesystemPath.h>

#include <tracy/Tracy.hpp>
#include <vector>

#define SOL_ALL_SAFETIES_ON 1

// this doesnt seem to do a thing
//#undef TRACY_ENABLE

#include "CompileSettings.h"
#include "ValhallaServer.h"

//#include "Tests.h"

/*
* Example command line args:
*   .\Valhalla.exe -vmodule=VUtilsResource=1
*   .\Valhalla.exe --no-colors "-vmodule=Peer=2,PrefabManager=2"
*   .\Valhalla.exe --no-log-backup --v=2
*   .\Valhalla.exe -v
*/

quill::Logger *VH_LOGGER {};

int main(int argc, char **argv)
{
    tracy::SetThreadName("main");

    std::filesystem::current_path("./data/");

    {
        quill::BackendOptions options;
        options.enable_yield_when_idle = false;
        options.sleep_duration         = 1ms;

        quill::Backend::start(options);
    }

    {
        std::vector<std::shared_ptr<quill::Sink>> sinks {
                quill::Frontend::create_or_get_sink<quill::ConsoleSink>(
                        "server_con",
                        []() {
                            // See RotatingFileSinkConfig for more options

                            quill::ConsoleSinkConfig cfg;
                            //cfg.set_colour_mode(quill::ConsoleSinkConfig::ColourMode::Automatic);
                            //cfg

                            //cfg.set_open_mode('w');
                            //cfg.set_filename_append_option(quill::FilenameAppendOption::StartDateTime);
                            //cfg.set_rotation_time_daily("24:00");
                            //cfg.set_rotation_max_file_size(1024); // small value to demonstrate the example

                            return cfg;
                        }()),
                quill::Frontend::create_or_get_sink<quill::RotatingFileSink>(
                        "server.log",
                        []() {
                            // See RotatingFileSinkConfig for more options

                            quill::RotatingFileSinkConfig cfg;

                            cfg.set_open_mode('w');
                            cfg.set_filename_append_option(quill::FilenameAppendOption::StartDateTime);
                            cfg.set_rotation_time_daily("00:00");
                            //cfg.set_rotation_max_file_size(1024); // small value to demonstrate the example
                            //cfg.set_minimum_fsync_interval(); //fsync forces a disk write
                            //cfg.set_write_buffer_size()
                            //cfg.set_fsync_enabled(bool value)
                            //cfg.set_write_buffer_size(size_t value)

                            return cfg;
                        }()),
        };

        quill::PatternFormatterOptions options {
                "%(time) [%(thread_name)] %(short_source_location:<30) %(log_level:<9) %(message)",// format
                //"%D %H:%M:%S.%Qms",                                              // timestamp format
                "%H:%M:%S.%Qms",// timestamp format
                quill::Timezone::LocalTime};

        auto logger = quill::Frontend::create_or_get_logger("main", std::move(sinks), options);

        logger->set_log_level(quill::LogLevel::TraceL3);

        /* global assigned */ VH_LOGGER = logger;
    }

#ifdef RUN_TESTS
    fs::current_path("./tests");

    VHTest().Test_ZDO_LoadSave();

    LOG_INFO(VH_LOGGER, "All tests passed!");
#else// !RUN_TESTS

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
    #endif// _DEBUG
        Valhalla()->Start();
    #ifndef _DEBUG
    } catch (std::exception const &e) {
        LOG_ERROR(VH_LOGGER, "{}", e.what());
        return 1;
    }
    #endif// _DEBUG

    return 0;
#endif    // !RUN_TESTS
}
