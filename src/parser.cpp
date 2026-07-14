#include "include/parser.h"
#include "include/logger.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <sys/stat.h>
#include <vector>

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
        double timestamp = std::stod(line.substr(delim + 1));

        if (key == "elapsed") {
            result.currentElapsed = timestamp;
        }
        else {
            result.cues[key].push_back(timestamp);
            result.timeline.push_back({timestamp, key});
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
