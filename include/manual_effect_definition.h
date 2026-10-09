#pragma once

#include "rgb_color.h"

#include <chrono>
#include <vector>

struct ManualEffectDefinition {
    std::vector<RgbColor> palette;
    double brightness;
    std::chrono::milliseconds transitionDuration;
    std::chrono::milliseconds entryDuration;
    bool shuffle;
};