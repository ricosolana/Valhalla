// main.cpp
#define SOL_ALL_SAFETIES_ON 1

#include <csignal>
#include <stdlib.h>

#include <optick.h>
#include <easylogging++.h>
#include <toml++/toml.h>

#include "VUtils.h"
#include "VUtilsResource.h"
#include "ValhallaServer.h"
#include "VUtilsRandom.h"
#include "CompileSettings.h"
#include "PrefabManager.h"

#include "Tests.h"

INITIALIZE_EASYLOGGINGPP

// https://stackoverflow.com/a/17350413
#define RESET "\033[0m"
#define BLACK "\033[30m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define GOLD "\033[33m"
#define BLUE "\033[34m"
#define PURPLE "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"
#define GRAY "\033[90m"

// https://github.com/amrayn/easyloggingpp
void initLogger(int argc, char** argv) {
    bool colors = true;
    bool log_backup = true;
    for (int i = 1; i < argc - 1; i++) {
        auto&& arg = std::string(argv[i]);

        if (arg == "--no-colors") {
            colors = false;
        }
        else if (arg == "--no-log-backup") {
            log_backup = false;
        }
    }

    el::Configurations loggerConfiguration;
    
    std::string format = "[%datetime{%H:%m:%s}] [%thread thread/%level]: %msg";
    loggerConfiguration.set(el::Level::Info, el::ConfigurationType::Format, format);
    loggerConfiguration.set(el::Level::Error, el::ConfigurationType::Format, (colors ? RED : "") + format + (colors ? RESET : ""));
    loggerConfiguration.set(el::Level::Fatal, el::ConfigurationType::Format, (colors ? RED : "") + format + (colors ? RESET : ""));
    loggerConfiguration.set(el::Level::Warning, el::ConfigurationType::Format, (colors ? GOLD : "") + format + (colors ? RESET : ""));
    
    loggerConfiguration.set(el::Level::Verbose, el::ConfigurationType::Format, (colors ? GRAY : "") +
        std::string("[%datetime{%H:%m:%s}] [%thread thread/v%vlevel] %fbase:L%line: %msg") + (colors ? RESET : ""));

    el::Helpers::setThreadName("main");
    el::Loggers::reconfigureAllLoggers(loggerConfiguration);
    el::Loggers::removeFlag(el::LoggingFlag::AllowVerboseIfModuleNotSpecified);

    // Load overrides prior to any logging
    START_EASYLOGGINGPP(argc, argv);

    // backup old logs
    if (log_backup) {
        std::error_code ec;

        auto now(std::to_string(steady_clock::now().time_since_epoch().count()));

        fs::path path = std::string(VALHALLA_LOGFILE_NAME) + "-" + now + ".zstd";

        fs::create_directories(path.parent_path(), ec);

        if (auto log = VUtils::Resource::ReadFile<BYTES_t>(VALHALLA_LOGFILE_NAME)) {
            if (auto opt = ZStdCompressor().Compress(*log))
                VUtils::Resource::WriteFile(path, *opt);
        }
    }

    loggerConfiguration.setGlobally(el::ConfigurationType::Filename, VALHALLA_LOGFILE_NAME);
    loggerConfiguration.setGlobally(el::ConfigurationType::ToFile, "true");
    loggerConfiguration.setGlobally(el::ConfigurationType::Enabled, "true");
    el::Loggers::reconfigureAllLoggers(loggerConfiguration);

    LOG(INFO) << "Configured loggers";
}

/*
* Example command line args:
*   .\Valhalla.exe -vmodule=VUtilsResource=1
*   .\Valhalla.exe --no-colors "-vmodule=Peer=2,PrefabManager=2"
*   .\Valhalla.exe --no-log-backup --v=2
*   .\Valhalla.exe -v
*/
int main(int argc, char **argv) {
#ifdef RUN_TESTS
    Tests().RunTests();
#else // !RUN_TESTS
    OPTICK_THREAD("main");
    
    fs::current_path("./data/");
    
    std::pair<int, int> p;
    auto p1 = std::pair<int, int>(0, 0);
    p = p1;

    // this throws because toml is cool
    //  why doesnt bepin use yaml or json...
    //auto opt = VUtils::Resource::ReadFile<std::string>(
    //    R"(\mods\RareMagicPortal\WackyMole.RareMagicPortal.cfg)");
    //auto cfg = toml::parse(*opt);
    //std::string env = "LUA_PATH=" + (fs::current_path() / VALHALLA_LUA_PATH).string() + "/?.lua";
    {
        std::string path = (fs::current_path() / VALHALLA_LUA_PATH).string();
        std::string path2 = (fs::current_path() / VALHALLA_MOD_PATH).string();
        std::string env = "LUA_PATH=" 
            + path + "/?.lua;" 
            + path + "/?/?.lua;"
            + path2 + "/?.lua;"
            + path2 + "/?/?.lua";
        putenv(env.c_str());
    }

    {
        std::string path = (fs::current_path() / VALHALLA_LUA_CPATH).string();
        std::string env = "LUA_CPATH=" 
            + path + "/?.dll;" 
            + path + "/?/?.dll";
        putenv(env.c_str());
    }

    // set basic defaults
    initLogger(argc, argv);

#ifndef _DEBUG
    try {
#endif // _DEBUG
        Valhalla()->Start();
#ifndef _DEBUG
    }
    catch (const std::exception& e) {
        LOG(ERROR) << e.what();
        return 1;
    }
#endif // _DEBUG

    return 0;
#endif // !RUN_TESTS
}
