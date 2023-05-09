// main.cpp
#define SOL_ALL_SAFETIES_ON 1

//#define WIN32_LEAN_AND_MEAN
//#include <WinSock2.h>

#include "VUtils.h"

#include "VUtilsResource.h"
#include "ValhallaServer.h"
#include "VUtilsRandom.h"
#include "CompileSettings.h"
#include "PrefabManager.h"

#include "Tests.h"

INITIALIZE_EASYLOGGINGPP

/*
class GlobalLogDispatcher : public el::LogDispatchCallback
{
protected:
    void handle(const el::LogDispatchData* data) noexcept override {
        //if (data->dispatchAction())
        //data->logMessage()->
        el::Level::Info
        // Dispatch using default log builder of logger
        dispatch(data->logMessage()->logger()->logBuilder()->build(data->logMessage(),
            data->dispatchAction() == el::base::DispatchAction::NormalLog));
    }
private:
    boost::asio::io_service m_svc;
    std::unique_ptr<Client> m_client;

    void dispatch(el::base::type::string_t&& logLine) noexcept
    {
        m_client->send(logLine);
    }
};*/

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
    loggerConfiguration.set(el::Level::Error, el::ConfigurationType::Format, (colors ? COLOR_RED : "") + format + (colors ? COLOR_RESET : ""));
    loggerConfiguration.set(el::Level::Fatal, el::ConfigurationType::Format, (colors ? COLOR_RED : "") + format + (colors ? COLOR_RESET : ""));
    loggerConfiguration.set(el::Level::Warning, el::ConfigurationType::Format, (colors ? COLOR_GOLD : "") + format + (colors ? COLOR_RESET : ""));
    
    loggerConfiguration.set(el::Level::Verbose, el::ConfigurationType::Format, (colors ? COLOR_GRAY : "") +
        std::string("[%datetime{%H:%m:%s}] [%thread thread/v%vlevel] %fbase:L%line: %msg") + (colors ? COLOR_RESET : ""));

    el::Helpers::setThreadName("main");
    el::Loggers::reconfigureAllLoggers(loggerConfiguration);
    el::Loggers::removeFlag(el::LoggingFlag::AllowVerboseIfModuleNotSpecified);

    // Load overrides prior to any logging
    START_EASYLOGGINGPP(argc, argv);



    // backup old logs
    if (log_backup) {
        std::error_code ec;

        auto now(std::to_string(steady_clock::now().time_since_epoch().count()));

        fs::path path = std::string(VH_LOGFILE_PATH) + "-" + now + ".zstd";

        fs::create_directories(path.parent_path(), ec);

        if (auto log = VUtils::Resource::ReadFile<BYTES_t>(VH_LOGFILE_PATH)) {
            if (auto opt = ZStdCompressor().Compress(*log))
                VUtils::Resource::WriteFile(path, *opt);
        }
    }

    loggerConfiguration.setGlobally(el::ConfigurationType::Filename, VH_LOGFILE_PATH);
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
    tracy::SetThreadName("main");
    
    fs::current_path("./data/");
    
    {
        std::string path = (fs::current_path() / VH_LUA_PATH).string();
        std::string path2 = (fs::current_path() / VH_MOD_PATH).string();
        std::string env = "LUA_PATH=" 
            + path + "/?.lua;" 
            + path + "/?/?.lua;"
            + path2 + "/?.lua;"
            + path2 + "/?/?.lua";
        putenv(env.c_str());
    }

    {
        std::string path = (fs::current_path() / VH_LUA_CPATH).string();
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
