#include "Game.hpp"
#include <openssl/md5.h>
//#include <steam>
//#include <nlohmann/json.hpp>
//#include <robin_hood.h>
#include <steam/steamnetworkingsockets.h>

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

// use this instead of steamapi
// https://github.com/ValveSoftware/GameNetworkingSockets/blob/e0d33386903202d9af61e76d69c54e46ece2f457/tests/test_p2p.cpp#L233

// webapi to auth ticket
// // https://partner.steamgames.com/doc/features/auth#client_to_backend_webapi
// https://partner.steamgames.com/doc/webapi/ISteamUserAuth#AuthenticateUserTicket

#ifdef _MSC_VER
#pragma warning( disable: 4702 ) /* unreachable code */
#endif

int main(int argc, char **argv) {

	// Initialize logger
	initLogger();

    //SteamNetworkingSockets()->CreateListenSocketP2P
    SteamNetworkingIdentity identity;
    
    identity.ParseString("str:peer-server");

	//Game::Run();

    //ZPackage pkg1;
    //pkg1.Write("1.0.0");
    //pkg1.GetStream().ResetPos();
    //
    //std::vector<byte> bytes;
    //auto len = pkg1.GetStream().Length();
    //
    //pkg1.GetStream().Read(bytes, len);

    //std::string password = "raspberry";
    //
    //ZPackage pkg;
    //
    //auto b = MD5(reinterpret_cast<const unsigned char*>(password.data()), password.length(), nullptr);
    //std::vector<byte> digest;
    //digest.insert(digest.begin(), b, b + 16);
    //
    //std::vector<byte> bytes;
    //pkg.Write(std::string(reinterpret_cast<char*>(
    //        MD5(reinterpret_cast<const unsigned char*>(password.data()), password.length(), nullptr)), 16));
    //bytes.insert(bytes.begin(), pkg.GetStream().Bytes(), pkg.GetStream().Bytes() + pkg.GetStream().Length());
    

    //ZPackage pkg(21);
    //pkg.Write(4);
    //auto mov = std::move(pkg);

	return 0;
}
