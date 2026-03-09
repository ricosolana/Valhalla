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

#include <string>
#include <string_view>
#include <thread>
#include <tracy/Tracy.hpp>
#include <vector>

#include "Avledet.h"
#include "CompileSettings.h"
#include "VUtils.h"

int main(int argc, char **argv)
{
    std::filesystem::current_path("./data/");

    auto set_lua_paths = [](std::string_view key, std::vector<std::filesystem::path> roots, std::string_view ext, bool deep) {
        auto ext1 = "/?" + std::string(ext);
        auto ext2 = "/?/?" + std::string(ext);

        std::string val;
        for (auto const& path : roots) {
            val += path.string() + ext1 + ";";
            
            if (deep) {
                val += path.string() + ext2 + ";";
            }
        }

        avledet::util::set_env(key, val);
    };

    set_lua_paths("LUA_PATH", 
        { 
            std::filesystem::current_path() / AVL_LUA_LIBS_PATH,
            std::filesystem::current_path() / AVL_LUA_SCRIPT_PATH 
        }
        , ".lua", true);

    set_lua_paths("LUA_CPATH", 
        { 
            std::filesystem::current_path() / AVL_LUA_C_PATH,
            //"/opt/zbstudio/bin/linux/x64"
        }
        , ".so", false); // TODO; target clibs?

    // I think ONLY windows requires this...
    //{
    //    std::string path = (std::filesystem::current_path() / AVL_LUA_LIBS_PATH).string();
    //    std::string path2 = (std::filesystem::current_path() / AVL_LUA_SCRIPT_PATH).string();
    //    if (!avledet::util::set_env("LUA_PATH",
    //        path + "/?.lua;"
    //        + path + "/?/?.lua;"
    //        + path2 + "/?.lua;"
    //        + path2 + "/?/?.lua"))
    //        LOG_ERROR(AVL_LOGGER, "Failed to set Lua path");
    //}
//
    //{
    //    std::string path = (std::filesystem::current_path() / AVL_LUA_C_PATH).string();
    //    if (!avledet::util::set_env("LUA_CPATH",
    //        path + "/?.so;"
    //        + path + "/?/?.so"))
    //        LOG_ERROR(AVL_LOGGER, "Failed to set Lua cpath");
    //}

#ifndef _DEBUG
    try {
#endif// _DEBUG
        Avledet()->Start();
#ifndef _DEBUG
    } catch (std::exception const &e) {
        // technically, we handle the error here, but this is the outer-error catcher
        //  with no further recourse; we simply exit the program for issues beyond
        LOG_ERROR(AVL_LOGGER, "Unhandled crash: {}", e.what());
        //return 1;
        exit(1);
    }
#endif

    return 0;
}
