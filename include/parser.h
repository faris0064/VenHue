#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

// TODO: separate LightingFX and PostFX
// will need rb3dx to print out the post fx alongside lighting

struct LightingEvent {
    double timestamp;
    std::string effect;

    bool operator<(const LightingEvent &other) const {
        return timestamp < other.timestamp;
    }
};

struct VenueData {
    double currentElapsed = 0.0;
    std::vector<LightingEvent> timeline;
    std::map<std::string, std::vector<double>> cues;
    time_t lastModified = 0;
};

class Parser {
public:

    VenueData parseDataFile(const std::string &filename);


    //Updates existing data with any new events from the file
    // only reads the file if it has been modified
    bool updateDataFile(const std::string &filename, VenueData &data);

    // Find the effect that should be active at a given time
    std::string findActiveEffect(const VenueData &data, double currentTime);

private:
    std::string stripQuotes(const std::string &input);
};
