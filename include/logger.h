#pragma once
#include <string>

namespace Logger {
    enum LogLevel {
        INFO,
        WARNING,
        ERROR,
        FATAL,
        HUE
    };

    void log(const std::string &message, const LogLevel level);
    void clear();
    void info(const std::string &message);
    void warning(const std::string &message);
    void error(const std::string &message);
    void fatal(const std::string &message);
    void hue(const std::string &message);

} // namespace Logger
