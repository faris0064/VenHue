#include "light_effect_loader.h"
#include "animated_effect_definition.h"
#include "logger.h"
#include "manual_effect_definition.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#include <array>
#include <cmath>
#include <optional>
#include <qcoreapplication.h>
#include <qfiledevice.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qjsonparseerror.h>
#include <qjsonvalue.h>
#include <qobject.h>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

    const std::set<QString> DEFINITION_FIELDS{
        QStringLiteral("palette"),
        QStringLiteral("brightness"),
        QStringLiteral("duration_ms"),
        QStringLiteral("entry_duration_ms"),
        QStringLiteral("duration_jitter"),
        QStringLiteral("transition_mode"),
        QStringLiteral("transition_curve")};

    const std::set<QString> MANUAL_DEF_FIELDS {
        QStringLiteral("palette"),
        QStringLiteral("brightness"),
        QStringLiteral("entry_duration_ms"),
        QStringLiteral("transition_duration_ms"),
        QStringLiteral("shuffle")
    };

    bool readNumber(const QJsonObject &object, const QString &field, double minimum, double maximum,
                    bool maximumInclusive, double &result, std::string &error) {
        const QJsonValue value = object.value(field);
        if (!value.isDouble()) {
            error = field.toStdString() + " must be numeric";
            return false;
        }

        result = value.toDouble();
        if (!std::isfinite(result) || result < minimum || (maximumInclusive ? result > maximum : result >= maximum)) {
            error = field.toStdString() + " is outside its allowed range";
            return false;
        }
        return true;
    }

    bool readMilliseconds(const QJsonObject &object, const QString &field, bool allowZero,
                          std::chrono::milliseconds &result, std::string &error) {
        constexpr double MAX_SAFE_INTEGER = 9007199254740991.0;
        double value = 0.0;
        if (!readNumber(object, field, allowZero ? 0.0 : 1.0, MAX_SAFE_INTEGER, true, value, error)) {
            return false;
        }
        if (std::floor(value) != value) {
            error = field.toStdString() + " must be an integer";
            return false;
        }
        result = std::chrono::milliseconds(static_cast<std::chrono::milliseconds::rep>(value));
        return true;
    }

    bool readBool(const QJsonObject &object, const QString &field, bool &result, std::string &error) {
        const QJsonValue value = object.value(field);
        if(!value.isBool()) {
            error = field.toStdString() + " must be a boolean";
            return false;
        }

        result = value.toBool();
        return true;
    }

    bool readPalette(const QJsonObject &object, std::vector<RgbColor> &palette, std::string &error) {
        const QJsonValue value = object.value(QStringLiteral("palette"));
        if (!value.isArray()) {
            error = "palette must be an array";
            return false;
        }

        const QJsonArray colors = value.toArray();
        if (colors.size() < 2) {
            error = "palette must contain at least two colors";
            return false;
        }

        palette.reserve(colors.size());
        for (qsizetype colorIndex = 0; colorIndex < colors.size(); ++colorIndex) {
            if (!colors.at(colorIndex).isArray()) {
                error = "palette color " + std::to_string(colorIndex) + " must be an array";
                return false;
            }
            const QJsonArray channels = colors.at(colorIndex).toArray();
            if (channels.size() != 3) {
                error = "palette color " + std::to_string(colorIndex) + " must contain exactly three values";
                return false;
            }

            std::array<double, 3> channelValues{};
            for (qsizetype channelIndex = 0; channelIndex < channels.size(); ++channelIndex) {
                const QJsonValue channel = channels.at(channelIndex);
                const double channelValue = channel.toDouble();
                if (!channel.isDouble() || !std::isfinite(channelValue) || channelValue < 0.0 || channelValue > 1.0) {
                    error = "palette color " + std::to_string(colorIndex) + " contains an invalid RGB value";
                    return false;
                }
                channelValues[static_cast<std::size_t>(channelIndex)] = channelValue;
            }
            palette.push_back({channelValues[0], channelValues[1], channelValues[2]});
        }
        return true;
    }

    bool parseAnimatedDefinition(const QJsonObject &object, AnimatedEffectDefinition &definition, std::string &error) {
        for (auto field = object.begin(); field != object.end(); ++field) {
            if (DEFINITION_FIELDS.count(field.key()) == 0) {
                error = "unknown field: " + field.key().toStdString();
                return false;
            }
        }
        for (const QString &field : DEFINITION_FIELDS) {
            if (!object.contains(field)) {
                error = "missing required field: " + field.toStdString();
                return false;
            }
        }

        if (!readPalette(object, definition.palette, error) || !readNumber(object, QStringLiteral("brightness"), 0.0, 1.0, true, definition.brightness, error) || !readMilliseconds(object, QStringLiteral("duration_ms"), false, definition.duration, error) || !readMilliseconds(object, QStringLiteral("entry_duration_ms"), true, definition.entryDuration, error) || !readNumber(object, QStringLiteral("duration_jitter"), 0.0, 1.0, false, definition.durationJitter, error)) {
            return false;
        }

        const QJsonValue mode = object.value(QStringLiteral("transition_mode"));
        const QString modeName = mode.toString();
        if (!mode.isString() || (modeName != QStringLiteral("fade") && modeName != QStringLiteral("snap"))) {
            error = "transition_mode must be fade or snap";
            return false;
        }
        definition.mode = modeName == QStringLiteral("fade") ? TransitionMode::Fade : TransitionMode::Snap;

        const QJsonValue curve = object.value(QStringLiteral("transition_curve"));
        const QString curveName = curve.toString();
        if (!curve.isString() || (curveName != QStringLiteral("linear") && curveName != QStringLiteral("ease_in_out_sine"))) {
            error = "transition_curve must be linear or ease_in_out_sine";
            return false;
        }
        definition.curve = curveName == QStringLiteral("linear") ? TransitionCurve::Linear : TransitionCurve::EaseInOutSine;
        return true;
    }

    bool parseManualDefinition(const QJsonObject &object, ManualEffectDefinition &definition, std::string &error) {

        for (auto field = object.begin(); field != object.end(); ++field) {
            if (MANUAL_DEF_FIELDS.count(field.key()) == 0) {
                error = "unknown field: " + field.key().toStdString();
                return false;
            }
        }
        
        for (const QString &field : MANUAL_DEF_FIELDS) {
            if (!object.contains(field)) {
                error = "missing required field: " + field.toStdString();
                return false;
            }
        }

        if (!readPalette(object, definition.palette, error) || !readNumber(object, QStringLiteral("brightness"), 0.0, 1.0, true, definition.brightness, error)
                                                            || !readMilliseconds(object, QStringLiteral("entry_duration_ms"), true, definition.entryDuration, error)
                                                            || !readMilliseconds(object, QStringLiteral("transition_duration_ms"), true, definition.transitionDuration, error)
                                                            || !readBool(object, QStringLiteral("shuffle"), definition.shuffle, error)) {
            return false;                                                
        }

        return true;
    }

    // Replace return type with std::expected once i figure out why c++23 wasn't working
    std::optional<QJsonObject> parseEffectsObject(const QString &path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            Logger::error("Unable to load light effects from " + path.toStdString() + ": " + file.errorString().toStdString());
            return std::nullopt;
        }
        
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            Logger::error("Unable to parse light effects from " + path.toStdString() + ": " + parseError.errorString().toStdString());
            return std::nullopt;
        }

        if (!document.isObject() || !document.object().value(QStringLiteral("effects")).isObject()) {
            Logger::warning("Light effects file must contain a top-level effects object.");
            return std::nullopt;
        }

        return document.object().value(QStringLiteral("effects")).toObject();

    }

} // namespace

AnimatedEffectDefinitions loadAnimatedEffectDefinitions(const std::vector<std::string_view> &effectNames) {
    AnimatedEffectDefinitions definitions;
    const std::set<std::string_view, std::less<>> validEffectNames(effectNames.begin(), effectNames.end());
    const QString path = QCoreApplication::applicationDirPath() + QStringLiteral("/light_effects.json");
    const std::optional<QJsonObject> effects = parseEffectsObject(path);

    if (effects == std::nullopt) {
        return definitions;
    }

    std::set<std::string, std::less<>> invalidDefinitions;
    for (auto effect = effects->begin(); effect != effects->end(); ++effect) {
        const std::string name = effect.key().toStdString();
        if (validEffectNames.count(name) == 0) {
            continue;
        }
        if (!effect.value().isObject()) {
            Logger::warning("Invalid animated light effect definition for " + name + ": definition must be an object. This effect will fall back to static lighting.");
            invalidDefinitions.insert(name);
            continue;
        }

        AnimatedEffectDefinition definition;
        std::string error;
        if (!parseAnimatedDefinition(effect.value().toObject(), definition, error)) {
            Logger::warning("Invalid animated light effect definition for " + name + ": " + error + ". This effect will fall back to static lighting.");
            invalidDefinitions.insert(name);
            continue;
        }
        definitions.emplace(name, std::move(definition));
    }

    for (const std::string_view name : effectNames) {
        if (definitions.count(name) == 0 && invalidDefinitions.count(name) == 0) {
            Logger::warning("Animated light effect definition missing for " + std::string(name) + ". This effect will fall back to static lighting.");
        }
    }
    Logger::info("Loaded " + std::to_string(definitions.size()) + " animated light effect definitions from " + path.toStdString());
    return definitions;
}

ManualEffectDefinitions loadManualEffectDefinitions(const std::vector<std::string_view> &effectNames) {
    ManualEffectDefinitions definitions;

    const std::set<std::string_view, std::less<>> validEffectNames(effectNames.begin(), effectNames.end());
    const QString path = QCoreApplication::applicationDirPath() + QStringLiteral("/light_effects.json");
    const std::optional<QJsonObject> effects = parseEffectsObject(path);

    if (effects == std::nullopt) {
        return definitions;
    }

    std::set<std::string, std::less<>> invalidDefinitions;
    for (auto effect = effects->begin(); effect != effects->end(); ++effect) {
        const std::string name = effect.key().toStdString();
        if (validEffectNames.count(name) == 0) {
            continue;
        }
        if (!effect.value().isObject()) {
            Logger::warning("Invalid manual light effect definition for " + name + ": definition must be an object. This effect will fall back to static lighting.");
            invalidDefinitions.insert(name);
            continue;
        }

        ManualEffectDefinition definition;
        std::string error;
        if (!parseManualDefinition(effect.value().toObject(), definition, error)) {
            Logger::warning("Invalid manual light effect definition for " + name + ": " + error + ". This effect will fall back to static lighting.");
            invalidDefinitions.insert(name);
            continue;
        }
        definitions.emplace(name, std::move(definition));
    }

    for (const std::string_view name : effectNames) {
        if (definitions.count(name) == 0 && invalidDefinitions.count(name) == 0) {
            Logger::warning("Manual light effect definition missing for " + std::string(name) + ". This effect will fall back to static lighting.");
        }
    }
    Logger::info("Loaded " + std::to_string(definitions.size()) + " Manual light effect definitions from " + path.toStdString());
    return definitions;
}
