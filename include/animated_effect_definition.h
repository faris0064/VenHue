#pragma once

#include <chrono>
#include <vector>

struct RgbColor {
    double red;
    double green;
    double blue;
};

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
