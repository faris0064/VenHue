#include "strobe_light_effect.h"
#include "logger.h"

#include <algorithm>
#include <cmath>
#include <utility>

StrobeLightEffect::StrobeLightEffect(std::string name, unsigned int layer,
                                     const StrobeEffectDefinition &definition,
                                     const std::optional<StrobeTiming> &timing)
    : huestream::Effect(std::move(name), layer),
      m_definition(definition),
      m_timingReceivedAt(std::chrono::steady_clock::now()),
      m_color(1.0, 1.0, 1.0) {
    if (timing) {
        updateTiming(*timing);
    }
}

void StrobeLightEffect::UpdateGroup(huestream::GroupPtr group) {
    (void)group;
}

void StrobeLightEffect::Render() {
    const auto now = std::chrono::steady_clock::now();
    if (!hasUsableTiming(now)) {
        m_color = huestream::Color(1.0, 1.0, 1.0);
        if (!m_warnedAboutTiming) {
            Logger::warning("Strobe timing is invalid, falling back to static colors");
            m_warnedAboutTiming = true;
        }
        return;
    }

    const double elapsedSeconds = std::chrono::duration<double>(now - m_timingReceivedAt).count();
    const double currentBeat = *m_beat + elapsedSeconds * *m_bpm / 60.0;
    const double subdivisionPosition = currentBeat * m_definition.subdivisionsPerBeat;
    const double subdivision = std::floor(subdivisionPosition);
    const bool white = subdivisionPosition - subdivision < m_definition.dutyCycle;
    m_color = white ? huestream::Color(1.0, 1.0, 1.0) : huestream::Color(0.0, 0.0, 0.0);
}

huestream::Color StrobeLightEffect::GetColor(huestream::LightPtr light) {
    (void)light;
    return m_color;
}

std::string StrobeLightEffect::GetTypeName() const {
    return "StrobeLightEffect";
}

void StrobeLightEffect::updateTiming(const StrobeTiming &timing) {
    m_bpm = timing.bpm;
    m_beat = timing.beat;
    m_timingReceivedAt = timing.receivedAt;
}

bool StrobeLightEffect::hasUsableTiming(std::chrono::steady_clock::time_point now) const {
    if (!m_bpm || !m_beat || !std::isfinite(*m_bpm) || *m_bpm <= 0.0 || !std::isfinite(*m_beat)) {
        return false;
    }

    const double staleAfterSeconds = std::max(1.0, 60.0 / *m_bpm);
    return std::chrono::duration<double>(now - m_timingReceivedAt).count() <= staleAfterSeconds;
}
