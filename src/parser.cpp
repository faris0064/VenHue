#include "include/parser.h"
#include "include/logger.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace {

std::optional<double> parseNumber(const std::string &value) {
    try {
        std::size_t parsedLength = 0;
        const double number = std::stod(value, &parsedLength);
        if (parsedLength != value.size() || !std::isfinite(number)) {
            return std::nullopt;
        }
        return number;
    }
    catch (...) {
        return std::nullopt;
    }
}

} // namespace

std::string Parser::stripQuotes(const std::string &input) {
    if (input.size() >= 2 && input.front() == '"' && input.back() == '"') {
        return input.substr(1, input.size() - 2);
    }
    return input;
}

VenueData Parser::parseDataFile(const std::string &filename) {
    VenueData result;
    std::ifstream file(filename);

    if (!file.is_open()) {
        Logger::error("Failed to open dx lighting file: " + filename);
        return result;
    }

    struct stat fileInfo;
    if (stat(filename.c_str(), &fileInfo) == 0) {
        result.lastModified = fileInfo.st_mtime;
    }

    std::string rawLine;
    while (std::getline(file, rawLine)) {
        if (!rawLine.empty() && rawLine.back() == '\r') {
            rawLine.pop_back();
        }
        std::string line = stripQuotes(rawLine);
        size_t delim = line.find("|");
        if (delim == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, delim);
        const auto value = parseNumber(line.substr(delim + 1));

        if (key == "elapsed") {
            if (value) {
                result.currentElapsed = *value;
            }
        }
        else if (key == "bpm") {
            if (value && *value > 0.0) {
                result.bpm = *value;
            }
        }
        else if (key == "beat") {
            if (value) {
                result.beat = *value;
            }
        }
        else if (value) {
            result.cues[key].push_back(*value);
            result.timeline.push_back({*value, key});
        }
    }

    std::sort(result.timeline.begin(), result.timeline.end());

    return result;
}

// Check if file has beeen modified, and update if so
bool Parser::updateDataFile(const std::string &filename, VenueData &data) {

    struct stat fileInfo;
    if (stat(filename.c_str(), &fileInfo) != 0) {
        Logger::error("Failed to get file info for: " + filename);
        return false;
    }

    if (fileInfo.st_mtime <= data.lastModified) {
        return false;
    }

    data = parseDataFile(filename);
    return true;
}

std::string Parser::findActiveEffect(const VenueData &data, double currentTime) {
    // Binary search for the first event that greater than currentTime, then go one event back
    if (data.timeline.empty() || currentTime < data.timeline.front().timestamp) {
        return "";
    }

    LightingEvent target{currentTime, ""};

    auto it = std::upper_bound(
        data.timeline.begin(),
        data.timeline.end(),
        target);

    if (it != data.timeline.begin()) {
        --it;
        return it->effect;
    }

    return "";
}
