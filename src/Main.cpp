#include "Game.hpp"

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
    //el::Helpers::installCustomFormatSpecifier(el::CustomFormatSpecifier("%startTime", std::bind(getTimeSinceProgramStart)));
    //std::string format = "%s [%startTime][%level][%thread][%fbase]: %msg";

    // https://github.com/amrayn/easyloggingpp#datetime-format-specifiers
    // [%fbase:L%line]
    std::string format = "[%datetime{%H:%m:%s.%g}] [%thread thread/%level]: %msg";
    loggerConfiguration.set(el::Level::Info, el::ConfigurationType::Format, format);
    loggerConfiguration.set(el::Level::Error, el::ConfigurationType::Format, RED + format + RESET);
    loggerConfiguration.set(el::Level::Fatal, el::ConfigurationType::Format, RED + format + RESET);
    loggerConfiguration.set(el::Level::Warning, el::ConfigurationType::Format, GOLD + format + RESET);
    loggerConfiguration.set(el::Level::Debug, el::ConfigurationType::Format, GOLD + format + RESET);
    el::Helpers::setThreadName("main");
    el::Loggers::reconfigureAllLoggers(loggerConfiguration);
    el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);
    //el::Loggers::file
    LOG(INFO) << "Logger is configured";
}

struct obyte {
    unsigned char m;
    obyte() {}
};

int main(int argc, char **argv) {

	// Initialize logger
	initLogger();

	Game::Run();

    //for (int trial = 1; trial <= 5; trial++) {
    //    auto startTime(std::chrono::steady_clock::now());
    //    std::vector<byte> vec_byte(1000000000);
    //    auto dur = std::chrono::steady_clock::now() - startTime;
    //
    //    LOG(INFO) << "Byte vec: " << std::chrono::duration_cast<std::chrono::milliseconds>(dur).count() << "ms (trial " << trial << ")";
    //}
    //
    //for (int trial = 1; trial <= 5; trial++) {
    //    auto startTime(std::chrono::steady_clock::now());
    //    std::vector<byte> vec_byte(1000000000);
    //    auto dur = std::chrono::steady_clock::now() - startTime;
    //
    //    LOG(INFO) << "oByte vec: " << std::chrono::duration_cast<std::chrono::milliseconds>(dur).count() << "ms (trial " << trial << ")";
    //}

	return 0;
}
