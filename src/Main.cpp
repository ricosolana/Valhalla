// main.cpp

//#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include "VUtils.h"
#include "DiscordManager.h"

quill::Logger* logger = nullptr;

int main(int argc, char **argv) {
    quill::Config cfg;
    cfg.enable_console_colours = true;
    quill::configure(cfg);
    quill::start();
    
    logger = quill::get_logger();
    logger->set_log_level(quill::LogLevel::TraceL3);

    std::string s;

    LOG_INFO(logger, "{}", s);

    //DiscordManager()->Init();

    return 0;
}
