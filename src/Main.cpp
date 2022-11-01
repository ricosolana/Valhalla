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
#include "ConsoleSSH.h"

#include "ResourceManager.h"
#include "ValhallaServer.h"

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

void initLogger() {
    el::Configurations loggerConfiguration;

    // https://github.com/amrayn/easyloggingpp#datetime-format-specifiers
    // https://github.com/amrayn/easyloggingpp/blob/master/README.md#using-configuration-file
    // [%fbase:L%line]
    //std::string format = "[%datetime{%H:%m:%s}] [%thread thread/%level]: %msg";
    std::string format = "[%datetime{%H:%m:%s}] [%thread thread/%level]: %msg";
    loggerConfiguration.set(el::Level::Info, el::ConfigurationType::Format, format);
    loggerConfiguration.set(el::Level::Error, el::ConfigurationType::Format, RED + format + RESET);
    loggerConfiguration.set(el::Level::Fatal, el::ConfigurationType::Format, RED + format + RESET);
    loggerConfiguration.set(el::Level::Warning, el::ConfigurationType::Format, GOLD + format + RESET);
    loggerConfiguration.set(el::Level::Debug, el::ConfigurationType::Format, GOLD + std::string("[%datetime{%H:%m:%s}] [%thread thread] %fbase:L%line: %msg") + RESET);
    loggerConfiguration.setGlobally(el::ConfigurationType::Filename, "log.txt");

    el::Helpers::setThreadName("main");
    el::Loggers::reconfigureAllLoggers(loggerConfiguration);
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
    initLogger();

    const char* root = "./data/";
    for (int i = 1; i < argc-1; i++) {
        if (strcmp(argv[i], "-root") == 0) {
            if (i < argc) root = argv[++i];
        }
    }
    ResourceManager::SetRoot(root);

    signal(SIGINT, on_interrupt);

    LOG(INFO) << "Press ctrl+c to exit";

    sshTest();

    return 0;

    //try {
        Valhalla()->Launch();
    //}
    //catch (std::exception& e) {
    //    LOG(ERROR) << e.what();
    //    return 1;
    //}

	return 0;
}
