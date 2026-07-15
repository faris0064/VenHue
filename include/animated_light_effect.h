#pragma once

#include <array>
#include <chrono>
#include <map>
#include <random>
#include <string>

#include <huestream/effect/effects/base/Effect.h>

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
    std::array<RgbColor, 4> palette;
    double brightness;
    std::chrono::milliseconds duration;
    std::chrono::milliseconds entryDuration;
    double durationJitter;
    TransitionMode mode;
    TransitionCurve curve;
};

class AnimatedLightEffect : public huestream::Effect {
public:
    AnimatedLightEffect(std::string name, unsigned int layer, const AnimatedEffectDefinition &definition);

    void UpdateGroup(huestream::GroupPtr group) override;
    void Render() override;
    huestream::Color GetColor(huestream::LightPtr light) override;
    std::string GetTypeName() const override;

private:
    struct ChannelState {
        RgbColor startColor;
        RgbColor renderedColor;
        std::size_t targetIndex;
        std::chrono::steady_clock::time_point transitionStart;
        std::chrono::milliseconds transitionDuration;
        bool entering;
    };

    RgbColor paletteColor(std::size_t index) const;
    std::size_t chooseNextTarget(std::size_t currentTarget);
    std::chrono::milliseconds nextDuration();
    double transitionProgress(const ChannelState &state, std::chrono::steady_clock::time_point now) const;
    RgbColor interpolate(const RgbColor &start, const RgbColor &end, double progress) const;

    AnimatedEffectDefinition m_definition;
    std::map<std::string, ChannelState> m_channels;
    std::mt19937 m_random;
};
