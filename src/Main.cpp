// main.cpp
#define SOL_ALL_SAFETIES_ON 1

#include <csignal>
#include <optick.h>
#include <easylogging++.h>

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

void initLogger(bool colors) {
    el::Configurations loggerConfiguration;

    // https://github.com/amrayn/easyloggingpp#datetime-format-specifiers
    // https://github.com/amrayn/easyloggingpp/blob/master/README.md#using-configuration-file
    // [%fbase:L%line]
    //std::string format = "[%datetime{%H:%m:%s}] [%thread thread/%level]: %msg";
    loggerConfiguration.setGlobally(el::ConfigurationType::Filename, VALHALLA_LOGFILE_NAME);
    
    std::string format = "[%datetime{%H:%m:%s}] [%thread thread/%level]: %msg";
    loggerConfiguration.set(el::Level::Info, el::ConfigurationType::Format, format);
    loggerConfiguration.set(el::Level::Error, el::ConfigurationType::Format, (colors ? RED : "") + format + (colors ? RESET : ""));
    loggerConfiguration.set(el::Level::Fatal, el::ConfigurationType::Format, (colors ? RED : "") + format + (colors ? RESET : ""));
    loggerConfiguration.set(el::Level::Warning, el::ConfigurationType::Format, (colors ? GOLD : "") + format + (colors ? RESET : ""));
    //loggerConfiguration.set(el::Level::Debug, el::ConfigurationType::Format,
        //(colors ? GOLD : "") + std::string("[%datetime{%H:%m:%s}] [%thread thread] %file:L%line: %msg") + (colors ? RESET : ""));

#ifndef ELPP_DISABLE_VERBOSE_LOGS
    //loggerConfiguration.set(el::Level::Verbose, el::ConfigurationType::Format, (colors ? GRAY : "") + format + (colors ? RESET : ""));
    //loggerConfiguration.set(el::Level::Verbose, el::ConfigurationType::Format, (colors ? GRAY : "") + 
        //std::string("[%datetime{%H:%m:%s}] [%thread thread/v%vlevel] %file:L%line: %msg") + (colors ? RESET : ""));
    //loggerConfiguration.set(el::Level::Verbose, el::ConfigurationType::Format, (colors ? GRAY : "") +
        //std::string("[%datetime{%H:%m:%s}] [%thread thread/v%vlevel] %loc: %msg") + (colors ? RESET : ""));
    loggerConfiguration.set(el::Level::Verbose, el::ConfigurationType::Format, (colors ? GRAY : "") +
        std::string("[%datetime{%H:%m:%s}] [%thread thread/v%vlevel] %fbase:L%line: %msg") + (colors ? RESET : ""));
#endif
    // Enable all modules unless otherwise specified?
    //  kinda vague
    //el::Loggers::addFlag(el::LoggingFlag::AllowVerboseIfModuleNotSpecified);

    el::Helpers::setThreadName("main");
    el::Loggers::reconfigureAllLoggers(loggerConfiguration);
    if (colors) // not sure whether this does anything
        el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);

    LOG(INFO) << "Logger is configured";
}

/*
* Example command line args:
*   .\Valhalla.exe -vmodule=VUtilsResource=1
*   .\Valhalla.exe --no-colors "-vmodule=Peer=2,PrefabManager=2"
*   .\Valhalla.exe --no-log-backups --v=2
*   .\Valhalla.exe -v
*/
int main(int argc, char **argv) {
#ifdef RUN_TESTS
    Tests().RunTests();
#else // !RUN_TESTS
    OPTICK_THREAD("main");
    
    fs::current_path("./data/");

    bool colors = true;
    bool log_backups = true;
    for (int i = 1; i < argc-1; i++) {
        auto&& arg = std::string(argv[i]);

        auto&& NEXT_ARG = [&]() { return i < argc ? std::string(argv[++i]) : ""; };
        //auto&& NEXT_ARG_ENABLED = [&]() { auto&& arg = NEXT_ARG(); return arg == "true" || arg == "1"; };
        //auto&& NEXT_ARG_NUMBER = [&]() { return std::stoi(NEXT_ARG()); };
        if (arg == "--no-colors") {
            colors = false;
        }
        else if (arg == "--no-log-backups") {
            log_backups = false;
        }
        //else if (arg == "-verbose") {
            /*
            * Verbosity levels: (1-9)
            *   - just some ideas on what to log:
            *   - how to order and prioritize what things in what way for verbose
            *   - Rare events should be printed first? Then often events printed last
            *       in order of increasing verbosity as assigned
            *       
            *       - 1 (Important rare prints)
            *           VUtils Resource read/written, 
            *           peer ctor/dtor
            *           socket ctor/dtor
            *           
            * 
            *       - 2 (Occasional prints):
            *           basic world loading data
            *               # of each location?
            *               global keys
            *               current random event
            *               
            * 
            *       - 3 ()
            * 
            *           
            *           
            *           
            *       - 4
            *           dungeon generation, 
            *           heightmap building 
            * 
            *       - 5 (Features being generated)
            * 
            *       - 7 (IMethod calls, and types, names...)
            * 
            *       - 8 (ZDOs being updated)
            * 
            *       - 8 (Partial packet data)
            * 
            *       - 9 (Exact packet data)
            *           
            *           
            * 
            *   - 1 (log somewhat descriptive infos)...
            *   - 5 (log some mild descriptive infos)...
            *   - 9 (log highly descriptive packets, rpc calls, lua event handlers)...
            */
            
            //el::Loggers::setVerboseLevel(NEXT_ARG_NUMBER());
        //}
    }

    // set basic defaults
    initLogger(colors);

    // now load overrides
    START_EASYLOGGINGPP(argc, argv);

    el::Loggers::removeFlag(el::LoggingFlag::AllowVerboseIfModuleNotSpecified);

    // backup old logs
    if (log_backups) {
        std::error_code ec;
        fs::create_directories("logs", ec);

        auto path = fs::path("logs") / (std::to_string(steady_clock::now().time_since_epoch().count()) + "-" + VALHALLA_LOGFILE_NAME + ".gz");

        if (auto log = VUtils::Resource::ReadFile<BYTES_t>(VALHALLA_LOGFILE_NAME)) {
            if (auto opt = ZStdCompressor().Compress(*log))
                VUtils::Resource::WriteFile(path, *opt);
        }
    }
    
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
