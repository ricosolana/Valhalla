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
    loggerConfiguration.set(el::Level::Debug, el::ConfigurationType::Format,
        (colors ? GOLD : "") + std::string("[%datetime{%H:%m:%s}] [%thread thread] %fbase:L%line: %msg") + (colors ? RESET : ""));

    loggerConfiguration.set(el::Level::Verbose, el::ConfigurationType::Format, format);
    
    el::Helpers::setThreadName("main");
    el::Loggers::reconfigureAllLoggers(loggerConfiguration);
    if (colors)
        el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);

    LOG(INFO) << "Logger is configured";
}

// Steam Documentation https://partner.steamgames.com/doc/sdk/api
int main(int argc, char **argv) {
#ifdef RUN_TESTS
    Tests().RunTests();
#else // !RUN_TESTS
    OPTICK_THREAD("main");

    std::string root = "./data/";
    bool colors = true;
    bool log_backups = true;
    for (int i = 1; i < argc-1; i++) {
        auto&& arg = std::string(argv[i]);

        auto&& NEXT_ARG = [&]() { return i < argc ? std::string(argv[++i]) : ""; };
        auto&& NEXT_ARG_ENABLED = [&]() { auto&& arg = NEXT_ARG(); return arg == "true" || arg == "1"; };
        auto&& NEXT_ARG_NUMBER = [&]() { return std::stoi(NEXT_ARG()); };
        if (arg == "-root") {
            root = NEXT_ARG();
        }
        else if (arg == "-colors") {
            colors = NEXT_ARG_ENABLED();
        }
        else if (arg == "-log-backups") {
            log_backups = NEXT_ARG_ENABLED();
        }
        else if (arg == "-verbose") {
            el::Loggers::setVerboseLevel(NEXT_ARG_NUMBER());
        }
    }

    fs::current_path(root);

    initLogger(colors);

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
