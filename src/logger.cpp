#include "include/logger.h"
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace Logger {
    std::string getCurrentTime() {
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        auto now_tm = *std::localtime(&now_time_t);

        std::ostringstream oss;
        oss << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    void clear() {
        std::ofstream logFile("log.txt", std::ios::out | std::ios::trunc);
        logFile.close();
    }

    void log(const std::string &message, const std::string level) {
        std::string logMessage = getCurrentTime() + " [" + level + "] " + message;
        std::cout << logMessage << std::endl;
        std::ofstream logFile("log.txt", std::ios::app);
        logFile << logMessage << std::endl;
        logFile.close();
    }

    void info(const std::string &message) {
        log(message, "INFO");
    }

    void warning(const std::string &message) {
        log(message, "WARNING");
    }

    void error(const std::string &message) {
        log(message, "ERROR");
    }

    void fatal(const std::string &message) {
        log(message, "FATAL");
    }

    void hue(const std::string& message) {
        log(message, "HUE");
    }
} // namespace Logger
