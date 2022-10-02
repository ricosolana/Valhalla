// main.cpp
#include <iostream>
#include <thread>
#include <chrono>
#include <assert.h>
#include <bitset>
#include <signal.h>

//#include "SteamManager.h"
#include "ValhallaServer.h"
#include "easylogging++.h"

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
    // [%fbase:L%line]
    std::string format = "[%datetime{%H:%m:%s}] [%thread thread/%level]: %msg";
    loggerConfiguration.set(el::Level::Info, el::ConfigurationType::Format, format);
    loggerConfiguration.set(el::Level::Error, el::ConfigurationType::Format, RED + format + RESET);
    loggerConfiguration.set(el::Level::Fatal, el::ConfigurationType::Format, RED + format + RESET);
    loggerConfiguration.set(el::Level::Warning, el::ConfigurationType::Format, GOLD + format + RESET);
    loggerConfiguration.set(el::Level::Debug, el::ConfigurationType::Format, GOLD + format + RESET);
    el::Helpers::setThreadName("main");
    el::Loggers::reconfigureAllLoggers(loggerConfiguration);
    el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);
    LOG(INFO) << "Logger is configured";
}

// https://www.reddit.com/r/cpp_questions/comments/dhrahr/how_do_you_make_a_weak_map_that_doesnt_leak_weak/
//template<typename K, typename V>
//struct weak_deleter {
//private:
//    //friend robin_hood::unordered_map<K, V>;
//
//    robin_hood::unordered_map<K, V> *pMap;
//    K key;
//
//public:
//    weak_deleter(robin_hood::unordered_map<K, V>& pMap) 
//        : pMap(&pMap) {}
//
//    ~weak_deleter() {
//        pMap->erase()
//    }
//};

//struct socket_t {
//    UUID uuid;
//    std::string hostname;
//};

static void on_interrupt(int num) {
    LOG(ERROR) << "Interrupt caught";

}

// See https://partner.steamgames.com/doc/sdk/api for documentation
int main(int argc, char **argv) {

    // try to parse settings one by one

    initLogger();

    signal(SIGINT, on_interrupt);

    //robin_hood::unordered_map<UUID, std::weak_ptr<socket_t>> map;
    //
    //std::shared_ptr<socket_t> strong{ 
    //    new socket_t {69420, "127.0.0.1"}, 
    //    [&](socket_t* s) { map.erase(s->uuid); }
    //};





    //SteamUtils()->SetWarningMessageHook([](int severity, const char* text) {
    //    std::cout << "severity: " << severity << ", " << text << "\n";
    //});

    //InitSteamGameServer();

    //try {
        Valhalla()->Launch();
    //}
    //catch (std::exception& e) {
    //    LOG(ERROR) << e.what();
    //    return 1;
    //}

    //LOG(INFO) << "znet: " << Valhalla()->m_znet << "\n";

	return 0;
}
