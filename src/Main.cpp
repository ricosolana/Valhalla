// main.cpp
#include <asio.hpp>

#include <iostream>
#include <thread>
#include <chrono>
#include <assert.h>
#include <bitset>
#include <signal.h>
#include <optick.h>
#include "ResourceManager.h"

#include "easylogging++.h"
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
    //el::Helpers::fi
    el::Helpers::setThreadName("main");
    el::Loggers::reconfigureAllLoggers(loggerConfiguration);
    el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);
    LOG(INFO) << "Logger is configured";
}

static void on_interrupt(int num) {
    el::Helpers::setThreadName("kernal");
    // get char
    LOG(INFO) << "Interrupt caught";
    Valhalla()->Terminate();
    //LOG(INFO) << "Press any key to exit...";
    //fgetc(stdin);
    //std::getchar();
    //getc(stdin); // pause
    //getc(stdin); // pause
    //getc(stdin); // pause
}

// See https://partner.steamgames.com/doc/sdk/api for documentation
int main(int argc, char **argv) {
    OPTICK_THREAD("main");
    initLogger();

    signal(SIGINT, on_interrupt);

    LOG(INFO) << "Press ctrl+c to exit";

    //ResourceManager::SetRoot("data");
    //bytes_t buf;

    //ResourceManager::ReadFileBytes("pic.jpg", buf);
    //auto compressed = Utils::Compress(buf.data(), buf.size());
    //ResourceManager::WriteFileBytes("pic", compressed);

    //return 0;

    //try {
        Valhalla()->Launch();
    //}
    //catch (std::exception& e) {
    //    LOG(ERROR) << e.what();
    //    return 1;
    //}

	return 0;
}
