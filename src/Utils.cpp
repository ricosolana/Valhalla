#include "Utils.hpp"

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

namespace Alchyme {

    namespace Utils {
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

        UID stringToUID(std::string_view sv) {
            std::string s(sv);
            std::stringstream ss(s);
            UID uid;
            ss >> uid;
            return uid;
        }

        bool isAddress(std::string_view s) {
            //address make_address(const char* str,
            //    asio::error_code & ec) ASIO_NOEXCEPT
            asio::error_code ec;
            asio::ip::make_address(s, ec);
            return ec ? false : true;
        }

        std::string join(std::vector<std::string_view>& strings) {
            std::string result;
            for (auto& s : strings) {
                result += s;
            }
            return result;
        }

        std::vector<std::string_view> split(std::string_view s, char ch) {
            //std::string s = "scott>=tiger>=mushroom";

            // split in Java appears to be a recursive decay function (for the pattern)

            int off = 0;
            int next = 0;
            std::vector<std::string_view> list;
            while ((next = s.find(ch, off)) != std::string::npos) {
                list.push_back(s.substr(off, next));
                off = next + 1;
            }
            // If no match was found, return this
            if (off == 0)
                return { s };

            // Add remaining segment
            list.push_back(s.substr(off));

            // Construct result
            int resultSize = list.size();
            while (resultSize > 0 && list[resultSize - 1].empty()) {
                resultSize--;
            }

            return std::vector<std::string_view>(list.begin(), list.begin() + resultSize);






            //std::vector<std::string_view> res;
            //
            //size_t pos = 0;
            //while ((pos = s.find(delimiter)) != std::string::npos) {
            //    //std::cout << token << std::endl;
            //    res.push_back(s.substr(0, pos));
            //    //s.erase(0, pos + delimiter.length());
            //    if (pos + delimiter.length() == s.length())
            //        s = s.substr(pos + delimiter.length());
            //    else s = s.substr(pos + delimiter.length());
            //}
            //if (res.empty())
            //    res.push_back(s);
            ////std::cout << s << std::endl;
            //return res;
        }
    }
}