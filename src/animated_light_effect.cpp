#include "animated_light_effect.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <utility>
#include <vector>

namespace {

    constexpr double PI = 3.14159265358979323846;

    struct HsvColor {
        double hue;
        double saturation;
        double value;
    };

    HsvColor rgbToHsv(const RgbColor &color) {
        const double maximum = std::max({color.red, color.green, color.blue});
        const double minimum = std::min({color.red, color.green, color.blue});
        const double delta = maximum - minimum;

        double hue = 0.0;
        if (delta > 0.0) {
            if (maximum == color.red) {
                hue = std::fmod((color.green - color.blue) / delta, 6.0);
            }
            else if (maximum == color.green) {
                hue = ((color.blue - color.red) / delta) + 2.0;
            }
            else {
                hue = ((color.red - color.green) / delta) + 4.0;
            }
            hue /= 6.0;
            if (hue < 0.0) {
                hue += 1.0;
            }
        }

        const double saturation = maximum == 0.0 ? 0.0 : delta / maximum;
        return {hue, saturation, maximum};
    }

    RgbColor hsvToRgb(const HsvColor &color) {
        const double chroma = color.value * color.saturation;
        const double hueSection = color.hue * 6.0;
        const double secondary = chroma * (1.0 - std::abs(std::fmod(hueSection, 2.0) - 1.0));

        RgbColor result{0.0, 0.0, 0.0};
        if (hueSection < 1.0) {
            result = {chroma, secondary, 0.0};
        }
        else if (hueSection < 2.0) {
            result = {secondary, chroma, 0.0};
        }
        else if (hueSection < 3.0) {
            result = {0.0, chroma, secondary};
        }
        else if (hueSection < 4.0) {
            result = {0.0, secondary, chroma};
        }
        else if (hueSection < 5.0) {
            result = {secondary, 0.0, chroma};
        }
        else {
            result = {chroma, 0.0, secondary};
        }

        const double match = color.value - chroma;
        result.red += match;
        result.green += match;
        result.blue += match;
        return result;
    }

} // namespace

AnimatedLightEffect::AnimatedLightEffect(std::string name, unsigned int layer, const AnimatedEffectDefinition &definition)
    : huestream::Effect(std::move(name), layer), m_definition(definition) {
    std::random_device randomDevice;
    std::seed_seq seed{randomDevice(), randomDevice(), randomDevice(), randomDevice()};
    m_random.seed(seed);
}

void AnimatedLightEffect::UpdateGroup(huestream::GroupPtr group) {
    m_channels.clear();
    if (!group || !group->GetLights()) {
        return;
    }

    std::vector<std::size_t> targets(m_definition.palette.size());
    std::iota(targets.begin(), targets.end(), 0);
    std::shuffle(targets.begin(), targets.end(), m_random);

    const auto now = std::chrono::steady_clock::now();
    std::size_t channelIndex = 0;
    for (const auto &light : *group->GetLights()) {
        if (!light) {
            continue;
        }
        if (channelIndex != 0 && channelIndex % targets.size() == 0) {
            std::shuffle(targets.begin(), targets.end(), m_random);
        }

        const auto currentColor = light->GetColor();
        const RgbColor startColor{currentColor.GetR(), currentColor.GetG(), currentColor.GetB()};
        const std::size_t targetIndex = targets[channelIndex % targets.size()];
        m_channels[light->GetId()] = {
            startColor,
            startColor,
            targetIndex,
            now,
            m_definition.entryDuration,
            true};
        ++channelIndex;
    }
}

void AnimatedLightEffect::Render() {
    const auto now = std::chrono::steady_clock::now();
    for (auto &[id, state] : m_channels) {
        (void)id;
        const RgbColor targetColor = paletteColor(state.targetIndex);

        if (state.entering) {
            const double progress = transitionProgress(state, now);
            state.renderedColor = interpolate(state.startColor, targetColor, progress);
            if (progress >= 1.0) {
                state.entering = false;
                state.startColor = targetColor;
                state.renderedColor = targetColor;
                state.transitionStart = now;
                state.transitionDuration = nextDuration();
                state.targetIndex = chooseNextTarget(state.targetIndex);
                if (m_definition.mode == TransitionMode::Snap) {
                    state.startColor = paletteColor(state.targetIndex);
                    state.renderedColor = state.startColor;
                }
            }
            continue;
        }

        if (m_definition.mode == TransitionMode::Snap) {
            state.renderedColor = targetColor;
            if (transitionProgress(state, now) >= 1.0) {
                state.targetIndex = chooseNextTarget(state.targetIndex);
                state.startColor = paletteColor(state.targetIndex);
                state.renderedColor = state.startColor;
                state.transitionStart = now;
                state.transitionDuration = nextDuration();
            }
            continue;
        }

        const double progress = transitionProgress(state, now);
        state.renderedColor = interpolate(state.startColor, targetColor, progress);
        if (progress >= 1.0) {
            state.startColor = targetColor;
            state.renderedColor = targetColor;
            state.targetIndex = chooseNextTarget(state.targetIndex);
            state.transitionStart = now;
            state.transitionDuration = nextDuration();
        }
    }
}

huestream::Color AnimatedLightEffect::GetColor(huestream::LightPtr light) {
    if (!light) {
        return {};
    }
    const auto channel = m_channels.find(light->GetId());
    if (channel == m_channels.end()) {
        return {};
    }
    const auto &color = channel->second.renderedColor;
    return {color.red, color.green, color.blue};
}

std::string AnimatedLightEffect::GetTypeName() const {
    return "VenHue.AnimatedLightEffect";
}

RgbColor AnimatedLightEffect::paletteColor(std::size_t index) const {
    const auto &color = m_definition.palette[index];
    return {
        color.red * m_definition.brightness,
        color.green * m_definition.brightness,
        color.blue * m_definition.brightness};
}

std::size_t AnimatedLightEffect::chooseNextTarget(std::size_t currentTarget) {
    std::uniform_int_distribution<std::size_t> distribution(0, m_definition.palette.size() - 2);
    std::size_t target = distribution(m_random);
    if (target >= currentTarget) {
        ++target;
    }
    return target;
}

std::chrono::milliseconds AnimatedLightEffect::nextDuration() {
    std::uniform_real_distribution<double> distribution(1.0 - m_definition.durationJitter,
                                                        1.0 + m_definition.durationJitter);
    const auto milliseconds = static_cast<long long>(std::llround(m_definition.duration.count() * distribution(m_random)));
    return std::chrono::milliseconds(std::max(1LL, milliseconds));
}

double AnimatedLightEffect::transitionProgress(const ChannelState &state, std::chrono::steady_clock::time_point now) const {
    if (state.transitionDuration.count() <= 0) {
        return 1.0;
    }
    const auto elapsed = std::chrono::duration<double, std::milli>(now - state.transitionStart).count();
    return std::clamp(elapsed / state.transitionDuration.count(), 0.0, 1.0);
}

RgbColor AnimatedLightEffect::interpolate(const RgbColor &start, const RgbColor &end, double progress) const {
    if (m_definition.curve == TransitionCurve::EaseInOutSine) {
        progress = -(std::cos(PI * progress) - 1.0) / 2.0;
    }

    HsvColor startHsv = rgbToHsv(start);
    HsvColor endHsv = rgbToHsv(end);
    if (startHsv.saturation == 0.0) {
        startHsv.hue = endHsv.hue;
    }
    if (endHsv.saturation == 0.0) {
        endHsv.hue = startHsv.hue;
    }

    double hueDelta = endHsv.hue - startHsv.hue;
    if (hueDelta > 0.5) {
        hueDelta -= 1.0;
    }
    else if (hueDelta < -0.5) {
        hueDelta += 1.0;
    }

    double hue = std::fmod(startHsv.hue + hueDelta * progress, 1.0);
    if (hue < 0.0) {
        hue += 1.0;
    }
    return hsvToRgb({hue,
                     startHsv.saturation + (endHsv.saturation - startHsv.saturation) * progress,
                     startHsv.value + (endHsv.value - startHsv.value) * progress});
}
