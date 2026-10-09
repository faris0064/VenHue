#pragma once

#include "manual_effect_definition.h"
#include "parser.h"
#include "rgb_color.h"

#include <chrono>
#include <cstddef>
#include <huestream/common/data/Group.h>
#include <huestream/common/data/Light.h>
#include <map>
#include <random>
#include <string>

#include <huestream/effect/effects/base/Effect.h>

class ManualLightEffect : public huestream::Effect {
public:
    ManualLightEffect(std::string name, unsigned int layer, const ManualEffectDefinition &definition);
    
    void UpdateGroup(huestream::GroupPtr group) override;
    void Render() override;
    huestream::Color GetColor(huestream::LightPtr light) override;
    std::string GetTypeName() const override;

    void handleCommand(KeyframeCommand command);

private:
    struct ChannelState {
        RgbColor startColor;
        RgbColor renderedColor;
        std::size_t startIndex;
        std::size_t targetIndex;
        std::chrono::steady_clock::time_point transitionStart;
        std::chrono::milliseconds transitionDuration;
    };

    RgbColor paletteColor(std::size_t index) const;
    double transitionProgress(const ChannelState &state, std::chrono::steady_clock::time_point now) const;
    RgbColor interpolate(const RgbColor &start, const RgbColor &end, double progress) const;

    ManualEffectDefinition m_definition;
    std::map<std::string, ChannelState> m_channels;
    std::mt19937 m_random;

};