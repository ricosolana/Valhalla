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

#include <thread>
#include <tracy/Tracy.hpp>
#include <vector>

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

int main(int argc, char **argv)
{
    std::filesystem::current_path("./data/");

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
#endif

    return 0;
}
