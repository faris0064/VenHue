#pragma once

#include <chrono>
#include <optional>
#include <string>

#include <huestream/effect/effects/base/Effect.h>

struct StrobeEffectDefinition {
    double subdivisionsPerBeat;
    double dutyCycle;
};

struct StrobeTiming {
    std::optional<double> bpm;
    std::optional<double> beat;
    std::chrono::steady_clock::time_point receivedAt;
};

class StrobeLightEffect : public huestream::Effect {
public:
    StrobeLightEffect(std::string name, unsigned int layer, const StrobeEffectDefinition &definition,
                      const std::optional<StrobeTiming> &timing);

    void UpdateGroup(huestream::GroupPtr group) override;
    void Render() override;
    huestream::Color GetColor(huestream::LightPtr light) override;
    std::string GetTypeName() const override;

    void updateTiming(const StrobeTiming &timing);

private:
    bool hasUsableTiming(std::chrono::steady_clock::time_point now) const;

    StrobeEffectDefinition m_definition;
    std::optional<double> m_bpm;
    std::optional<double> m_beat;
    std::chrono::steady_clock::time_point m_timingReceivedAt;
    huestream::Color m_color;
    bool m_warnedAboutTiming = false;
};
