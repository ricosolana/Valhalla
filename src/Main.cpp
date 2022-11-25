#ifdef _WIN32
//#define _WIN32_WINNT 0x0A00
//#define WIN32_LEAN_AND_MEAN
//#define WIN32_LEAN_AND_MEAN
//#include <asio.hpp>
#endif

// main.cpp
#include <csignal>
#include <optick.h>
#include <easylogging++.h>

#include "VUtils.h"
#include "VUtilsResource.h"
#include "VServer.h"
#include "VUtilsRandom.h"
#include "CompileSettings.h"

INITIALIZE_EASYLOGGINGPP

// https://stackoverflow.com/a/17350413
// Colors for output
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
    std::string format = "[%datetime{%H:%m:%s}] [%thread thread/%level]: %msg";
    loggerConfiguration.set(el::Level::Info, el::ConfigurationType::Format, format);
    loggerConfiguration.set(el::Level::Error, el::ConfigurationType::Format, (colors ? RED : "") + format + (colors ? RESET : ""));
    loggerConfiguration.set(el::Level::Fatal, el::ConfigurationType::Format, (colors ? RED : "") + format + (colors ? RESET : ""));
    loggerConfiguration.set(el::Level::Warning, el::ConfigurationType::Format, (colors ? GOLD : "") + format + (colors ? RESET : ""));
    loggerConfiguration.set(el::Level::Debug, el::ConfigurationType::Format, 
        (colors ? GOLD : "") + std::string("[%datetime{%H:%m:%s}] [%thread thread] %fbase:L%line: %msg") + (colors ? RESET : ""));
    loggerConfiguration.setGlobally(el::ConfigurationType::Filename, LOGFILE_NAME);

    el::Helpers::setThreadName("main");
    el::Loggers::reconfigureAllLoggers(loggerConfiguration);
    if (colors)
        el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);
    LOG(INFO) << "Logger is configured";
}

static void on_interrupt(int num) {
    el::Helpers::setThreadName("kernal");
    LOG(INFO) << "Interrupt caught";
    Valhalla()->Terminate();
}

// Steam Documentation https://partner.steamgames.com/doc/sdk/api
int main(int argc, char **argv) {
    OPTICK_THREAD("main");

    const char* root = "./data/";
    bool colors = true;
    bool backup_logs = true;
    for (int i = 1; i < argc-1; i++) {
        if (strcmp(argv[i], "-root") == 0) {
            if (i < argc) root = argv[++i];
        }
        else if (strcmp(argv[i], "-colors") == 0) {
            if (i < argc) colors = strcmp(argv[++i], "false") == 0 || strcmp(argv[i], "0") == 0 ? false : true;
        }
        else if (strcmp(argv[i], "-backup-logs") == 0) {
            if (i < argc) backup_logs = strcmp(argv[++i], "false") == 0 || strcmp(argv[i], "0") == 0 ? false : true;
        }
    }

    // Copy any old file
    if (backup_logs)
        fs::copy_file(LOGFILE_NAME, 
            std::to_string(steady_clock::now().time_since_epoch().count()) + LOGFILE_NAME);

    initLogger(colors);

    VUtils::Resource::SetRoot(root);

    signal(SIGINT, on_interrupt);

    LOG(INFO) << "Press ctrl+c to exit";

    //for (float i = 0; i < 2; i += .13f)
    //{
    //    LOG(INFO) << "Perlin " << i << ", " << (i + .18f) << ": " << VUtils::Random::PerlinNoise(i, i + .18f);
    //}

    /*
    VUtils::Random::State state(69420);
    LOG(INFO) << ("Random.Range (0.0f, 420.f)");
    for (int i = 0; i < 3; i++)
    {
        LOG(INFO) << (state.Range(0.0f, 420.0f));
    }
    LOG(INFO) << ("Random.Range (0, 420)");
    for (int i = 0; i < 3; i++)
    {
        LOG(INFO) << (state.Range(0, 420));
    }
    LOG(INFO) << ("Random.insideUnitCircle");
    for (int i = 0; i < 2; i++)
    {
        auto vec = state.GetRandomUnitCircle();
        LOG(INFO) << vec.x << " " << vec.y;
    }
    LOG(INFO) << ("Random.insideUnitSphere");
    for (int i = 0; i < 2; i++)
    {
        auto vec = state.InsideUnitSphere();
        LOG(INFO) << vec.x << " " << vec.y << " " << vec.z;
    }
    LOG(INFO) << ("Random.onUnitSphere");
    for (int i = 0; i < 2; i++)
    {
        auto vec = state.OnUnitSphere();
        LOG(INFO) << vec.x << " " << vec.y << " " << vec.z;
    }

    return 0;*/

#ifndef _DEBUG
    try {
#endif
        Valhalla()->Launch();
#ifndef _DEBUG
    }
    catch (std::exception& e) {
        LOG(ERROR) << e.what();
        return 1;
    }
#endif

	return 0;
}
