#pragma once

#include "rgb_color.h"

#include <chrono>
#include <vector>

enum class TransitionMode {
    Fade,
    Snap
};

enum class TransitionCurve {
    Linear,
    EaseInOutSine
};

struct AnimatedEffectDefinition {
    std::vector<RgbColor> palette;
    double brightness;
    std::chrono::milliseconds duration;
    std::chrono::milliseconds entryDuration;
    double durationJitter;
    TransitionMode mode;
    TransitionCurve curve;
};
