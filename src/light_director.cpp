#include "light_director.h"
#include <QHostInfo>
#include <QTimer>
#include <huestream/HueStream.h>
#include <huestream/effect/effects/AreaEffect.h>
#include <huestream/common/data/Area.h>
#include <huestream/common/data/Color.h>
#include <logger.h>

LightDirector::LightDirector(QObject *parent)
    : QObject(parent), m_bridgeStatus("No bridge connected"), m_bridgeFound(false),
      m_isStreaming(false), m_shouldAutoReconnect(false), m_currentLightingFX(LightingFX::IDLE), m_currentPostFX(PostFX::DEFAULT) {
    // huestream stores the config by itself, don't need to have any QSettings stuff for it anymore
    m_config = std::make_shared<huestream::Config>("VenHue", QHostInfo::localHostName().toStdString(), huestream::PersistenceEncryptionKey("VenHuePersistenceKey"));
    m_hueStream = std::make_shared<huestream::HueStream>(m_config);

    // Register connection flow feedback callback
    m_hueStream->RegisterFeedbackCallback([this](const huestream::FeedbackMessage &message) {
        handleHueStreamFeedback(message);
    });
}

void LightDirector::handleHueStreamFeedback(const huestream::FeedbackMessage &message) {
    Logger::hue(message.GetDebugMessage());

    using namespace huestream;
    switch (message.GetId()) {
        case FeedbackMessage::ID_FINISH_SEARCH_BRIDGES_FOUND:
            m_bridgeFound = true;
            emit bridgeFoundChanged(true);
            updateBridgeStatus("Bridge found and ready to connect");
            break;

        case FeedbackMessage::ID_PRESS_PUSH_LINK:
            updateBridgeStatus("Press the link button on your Hue bridge...");
            break;

        case FeedbackMessage::ID_FINISH_AUTHORIZING_AUTHORIZED:
            updateBridgeStatus("Bridge paired sucessfully!");
            emit bridgePaired();
            break;

        case FeedbackMessage::ID_NO_BRIDGE_FOUND:
            updateBridgeStatus("No bridge found.");
            break;

        case FeedbackMessage::ID_FINISH_LOADING_BRIDGE_CONFIGURED:
            m_bridgeFound = true;
            emit bridgeFoundChanged(true);
            
            // Only auto-reconnect if we're loading at startup, not during manual search
            // Otherwise hit a breakpoint in the EDK cause of deadlock
            if (m_shouldAutoReconnect) {
                updateBridgeStatus("Previous bridge found, reconnecting...");
                m_hueStream->ConnectBridgeAsync();
            } else {
                updateBridgeStatus("Bridge found and ready to connect");
            }
            
            m_shouldAutoReconnect = false;
            break;

        case FeedbackMessage::ID_FINISH_LOADING_NO_BRIDGE_CONFIGURED:
            updateBridgeStatus("No bridge paired. Search for a bridge to begin setup.");
            break;

        case FeedbackMessage::ID_FINISH_RETRIEVING_READY_TO_START:
        case FeedbackMessage::ID_FINISH_RETRIEVING_ACTION_REQUIRED: {
            // Get a list of all available entertainment areas
            m_availableAreas.clear();
            auto groups = m_hueStream->GetLoadedBridgeGroups();
            if (groups) {
                for (const auto &group : *groups) {
                    m_availableAreas.append(QString::fromStdString(group->GetName()));
                }
            }

            updateBridgeStatus("Bridge connected! Select an entertainment area.");
            emit areasAvailableChanged(true);
            break;
        }

        case FeedbackMessage::ID_SELECT_GROUP:
            Logger::info("Select an entertainment area");
            updateBridgeStatus("Select an entertainment area.");
            
            // Populate areas if not already done
            if (m_availableAreas.isEmpty()) {
                auto groups = m_hueStream->GetLoadedBridgeGroups();
                if (groups) {
                    for (const auto &group : *groups) {
                        m_availableAreas.append(QString::fromStdString(group->GetName()));
                    }
                }
            }
            emit areasAvailableChanged(!m_availableAreas.isEmpty());
            break;

        case FeedbackMessage::ID_STREAMING_CONNECTED:
            m_isStreaming = true;
            updateBridgeStatus("Streaming active");
            Logger::info("Entertainment streaming is now active");
            applyFullEffect(); // Apply idle effect immediatly
            break;

        case FeedbackMessage::ID_STREAMING_DISCONNECTED:
            m_isStreaming = false;
            updateBridgeStatus("Streaming disconnected");
            Logger::info("Entertainment streaming disconnected");
            break;

        default:
            Logger::warning("Unhandled HueStream message ID: " + std::to_string(message.GetId()));
            break;
    }
}

bool LightDirector::isBridgePaired() const {
    QSettings settings;
    return settings.contains("bridge/ip") && settings.contains("bridge/username");
}

void LightDirector::initializeBridge() {
    // Enable auto-reconnect for initialization (startup)
    m_shouldAutoReconnect = true;
    
    m_hueStream->LoadBridgeInfoAsync();

    if (isBridgePaired()) {
        updateBridgeStatus("Loading bridge configuration...");
    }
    else {
        updateBridgeStatus("No bridge paired. Search for a bridge to begin setup.");
    }
}

void LightDirector::reconnectToBridge() {
    updateBridgeStatus("Reconnecting with saved bridge...");
    m_hueStream->ConnectBridgeAsync();
}

void LightDirector::searchForBridge() {
    m_shouldAutoReconnect = false;
    
    updateBridgeStatus("Searching for bridges...");
    m_hueStream->ConnectBridgeBackgroundAsync();
}

void LightDirector::connectToBridge() {
    if (!m_bridgeFound)
        return;

    updateBridgeStatus("Press the link button on your Hue bridge...");

    m_hueStream->ConnectBridgeAsync();
}

void LightDirector::resetBridgePairing() {
    QSettings settings;
    settings.remove("bridge/ip");
    settings.remove("bridge/username");
    m_hueStream->ResetBridgeInfoAsync();

    m_bridgeFound = false;
    emit bridgeFoundChanged(false);
    updateBridgeStatus("Bridge pairing reset");
}

void LightDirector::updateBridgeStatus(const QString &status) {
    Logger::info(status.toStdString());
    m_bridgeStatus = status;
    emit bridgeStatusChanged(status);
}

QStringList LightDirector::getAvailableAreas() const {
    return m_availableAreas;
}

void LightDirector::selectArea(const QString &areaName) {
    auto groups = m_hueStream->GetLoadedBridgeGroups();
    if (groups) {
        for (const auto &group : *groups) {
            if (QString::fromStdString(group->GetName()) == areaName) {
                m_hueStream->SelectGroupAsync(group);
                m_currentArea = areaName;
                emit areaSelected(areaName);
                updateBridgeStatus("Selected entertainment area: " + areaName);
                break;
            }
        }
    }
}

QString LightDirector::getCurrentArea() const {
    return m_currentArea;
}

bool LightDirector::hasAvailableAreas() const {
    return !m_availableAreas.isEmpty();
}

LightDirector::LightingFX LightDirector::stringToLightingFX(const std::string &effectName) {
    if (effectName == "idle") return LightingFX::IDLE;
    if (effectName == "verse") return LightingFX::VERSE;
    if (effectName == "chorus") return LightingFX::CHORUS;
    if (effectName == "manual_cool") return LightingFX::MANUAL_COOL;
    if (effectName == "manual_warm") return LightingFX::MANUAL_WARM;
    if (effectName == "dischord") return LightingFX::DISCHORD;
    if (effectName == "stomp") return LightingFX::STOMP;
    if (effectName == "loop_cool") return LightingFX::LOOP_COOL;
    if (effectName == "loop_warm") return LightingFX::LOOP_WARM;
    if (effectName == "harmony") return LightingFX::HARMONY;
    if (effectName == "frenzy") return LightingFX::FRENZY;
    if (effectName == "silhouettes") return LightingFX::SILHOUETTES;
    if (effectName == "silhouettes_spot") return LightingFX::SILHOUETTES_SPOT;
    if (effectName == "searchlights") return LightingFX::SEARCHLIGHTS;
    if (effectName == "sweep") return LightingFX::SWEEP;
    if (effectName == "strobe_slow") return LightingFX::STROBE_SLOW;
    if (effectName == "strobe_fast") return LightingFX::STROBE_FAST;
    if (effectName == "blackout_slow") return LightingFX::BLACKOUT_SLOW;
    if (effectName == "blackout_fast") return LightingFX::BLACKOUT_FAST;
    if (effectName == "blackout_spot") return LightingFX::BLACKOUT_SPOT;
    if (effectName == "flare_slow") return LightingFX::FLARE_SLOW;
    if (effectName == "flare_fast") return LightingFX::FLARE_FAST;
    if (effectName == "bre") return LightingFX::BRE;
    
    Logger::warning("Unknown LightingFX string: " + effectName);
    return LightingFX::IDLE;
}

LightDirector::PostFX LightDirector::stringToPostFX(const std::string &effectName) {
    if (effectName == "default") return PostFX::DEFAULT;
    if (effectName == "color_muted") return PostFX::COLOR_MUTED;
    if (effectName == "video_grainy") return PostFX::VIDEO_GRAINY;
    if (effectName == "16mm_film") return PostFX::SIXTEENMM_FILM;
    if (effectName == "shitty_tv") return PostFX::SHITTY_TV;
    if (effectName == "bloom") return PostFX::BLOOM;
    if (effectName == "sepia_ink") return PostFX::SEPIA_INK;
    if (effectName == "silvertone") return PostFX::SILVERTONE;
    if (effectName == "film_bw") return PostFX::FILM_BW;
    if (effectName == "video_bw") return PostFX::VIDEO_BW;
    if (effectName == "contrast_bw") return PostFX::CONTRAST_BW;
    if (effectName == "photocopy") return PostFX::PHOTOCOPY;
    if (effectName == "blue_filter") return PostFX::BLUE_FILTER;
    if (effectName == "desat_blue") return PostFX::DESAT_BLUE;
    if (effectName == "video_security") return PostFX::VIDEO_SECURITY;
    if (effectName == "bright") return PostFX::BRIGHT;
    if (effectName == "posterize") return PostFX::POSTERIZE;
    if (effectName == "clean_trails") return PostFX::CLEAN_TRAILS;
    if (effectName == "video_trails") return PostFX::VIDEO_TRAILS;
    if (effectName == "flicker_trails") return PostFX::FLICKER_TRAILS;
    if (effectName == "desat_posterize") return PostFX::DESAT_POSTERIZE;
    if (effectName == "film_contrast") return PostFX::FILM_CONTRAST;
    if (effectName == "film_contrast_blue") return PostFX::FILM_CONTRAST_BLUE;
    if (effectName == "film_contrast_green") return PostFX::FILM_CONTRAST_GREEN;
    if (effectName == "film_contrast_red") return PostFX::FILM_CONSTRAST_RED;
    if (effectName == "horror_movie") return PostFX::HORROR_MOVIE;
    if (effectName == "photo_negative") return PostFX::PHOTO_NEGATIVE;
    if (effectName == "mirror") return PostFX::MIRROR;
    if (effectName == "psych_blue_red") return PostFX::PSYCH_BLUE_RED;
    if (effectName == "space_woosh") return PostFX::SPACE_WOOSH;
    
    Logger::warning("Unknown PostFX string: " + effectName);
    return PostFX::DEFAULT;
}

// Primitive light effects. These are all placeholders as lights will need to "animate" (like fading between colors)

LightDirector::LightingData LightDirector::generateBaseLighting(LightingFX fx) {

    LightingData lighting;
    
    switch (fx) {
        case LightingFX::IDLE:
            // Blue, ambient "night time" lighting
            lighting.red = 0.0;
            lighting.green = 0.2;
            lighting.blue = 0.6;
            lighting.brightness = 0.3;
            lighting.saturation = 1.0;
            break;
            
        case LightingFX::VERSE:
            lighting.red = 1.0;
            lighting.green = 0.65;
            lighting.blue = 0.0;
            lighting.brightness = 0.7;
            lighting.saturation = 0.8;
            break;
            
        case LightingFX::CHORUS:
            lighting.red = 1.0;
            lighting.green = 0.0;
            lighting.blue = 1.0;
            lighting.brightness = 1.0;
            lighting.saturation = 1.0;
            break;
            
        case LightingFX::MANUAL_COOL:
            lighting.red = 0.4;
            lighting.green = 0.6;
            lighting.blue = 1.0;
            lighting.brightness = 0.8;
            lighting.saturation = 0.7;
            break;
            
        case LightingFX::MANUAL_WARM:
            lighting.red = 1.0;
            lighting.green = 0.7;
            lighting.blue = 0.3;
            lighting.brightness = 0.8;
            lighting.saturation = 0.7;
            break;
            
        case LightingFX::DISCHORD:
            lighting.red = 1.0;
            lighting.green = 1.0;
            lighting.blue = 0.0;
            lighting.brightness = 1.0;
            lighting.saturation = 1.0;
            break;
            
        case LightingFX::STOMP:
            lighting.red = 1.0;
            lighting.green = 1.0;
            lighting.blue = 1.0;
            lighting.brightness = 1.0;
            lighting.saturation = 0.0;
            break;
            
        case LightingFX::LOOP_COOL:
            lighting.red = 0.0;
            lighting.green = 0.8;
            lighting.blue = 1.0;
            lighting.brightness = 0.8;
            lighting.saturation = 1.0;
            break;
            
        case LightingFX::LOOP_WARM:
            lighting.red = 1.0;
            lighting.green = 0.4;
            lighting.blue = 0.0;
            lighting.brightness = 0.8;
            lighting.saturation = 1.0;
            break;
            
        case LightingFX::HARMONY:
            lighting.red = 1.0;
            lighting.green = 0.65;
            lighting.blue = 0.0; 
            lighting.brightness = 0.7;
            lighting.saturation = 0.85;
            break;
            
        case LightingFX::FRENZY:
            lighting.red = 0.0;
            lighting.green = 1.0;
            lighting.blue = 0.0;
            lighting.brightness = 1.0;
            lighting.saturation = 1.0;
            break;
            
        case LightingFX::SILHOUETTES:
            lighting.red = 0.0;
            lighting.green = 0.12;
            lighting.blue = 0.3;
            lighting.brightness = 0.25;
            lighting.saturation = 0.8;
            break;
            
        case LightingFX::SILHOUETTES_SPOT:
            lighting.red = 0.0;
            lighting.green = 0.2;
            lighting.blue = 0.47;
            lighting.brightness = 0.4;
            lighting.saturation = 0.8;
            break;
            
        case LightingFX::SEARCHLIGHTS:
            lighting.red = 1.0;
            lighting.green = 1.0;
            lighting.blue = 1.0;
            lighting.brightness = 1.0;
            lighting.saturation = 0.0;
            break;
            
        case LightingFX::SWEEP:
            lighting.red = 1.0;
            lighting.green = 1.0;
            lighting.blue = 1.0;
            lighting.brightness = 1.0;
            lighting.saturation = 0.4;
            break;
            
        case LightingFX::STROBE_SLOW:
            lighting.red = 1.0;
            lighting.green = 1.0;
            lighting.blue = 1.0;
            lighting.brightness = 1.0;
            lighting.saturation = 0.0;
            lighting.strobe = true;
            lighting.strobeRate = 2.0;
            break;
            
        case LightingFX::STROBE_FAST:
            lighting.red = 1.0;
            lighting.green = 1.0;
            lighting.blue = 1.0;
            lighting.brightness = 1.0;
            lighting.saturation = 0.0;
            lighting.strobe = true;
            lighting.strobeRate = 4.0;
            break;
            
        case LightingFX::BLACKOUT_SLOW:
        case LightingFX::BLACKOUT_FAST:
        case LightingFX::BLACKOUT_SPOT:
            lighting.red = 0.0;
            lighting.green = 0.0;
            lighting.blue = 0.0;
            lighting.brightness = 0.0;
            lighting.saturation = 0.0;
            break;
            
        case LightingFX::FLARE_SLOW:
        case LightingFX::FLARE_FAST:
            lighting.red = 1.0;
            lighting.green = 1.0;
            lighting.blue = 1.0;
            lighting.brightness = 1.0;
            lighting.saturation = 0.0;
            break;
            
        case LightingFX::BRE:
            lighting.red = 1.0;
            lighting.green = 0.0;
            lighting.blue = 1.0; 
            lighting.brightness = 1.0;
            lighting.saturation = 1.0;
            break;
            
        default:
            Logger::warning("Unknown LightingFX enum value");
            lighting.red = 0.0;
            lighting.green = 0.2;
            lighting.blue = 0.6;
            lighting.brightness = 0.3;
            lighting.saturation = 1.0;
            break;
    }
    
    return lighting;
}

LightDirector::LightingData LightDirector::applyPostProcessing(const LightingData &baseLighting, PostFX postFx) {
    LightingData processed = baseLighting;
    
    switch (postFx) {
        case PostFX::DEFAULT:
            break;
            
        case PostFX::COLOR_MUTED:
            processed.saturation = processed.saturation * 0.7;
            break;
            
        case PostFX::VIDEO_GRAINY:
        case PostFX::SIXTEENMM_FILM:
        case PostFX::CLEAN_TRAILS:
        case PostFX::VIDEO_TRAILS:
            break;
            
        case PostFX::SHITTY_TV:
            processed.brightness = std::min(1.0, processed.brightness * 1.5);
            processed.red = std::min(1.0, processed.red * 1.3);
            processed.green = std::min(1.0, processed.green * 1.3);
            processed.blue = std::min(1.0, processed.blue * 1.3);
            break;
            
        case PostFX::BLOOM:
            processed.brightness = std::min(1.0, processed.brightness * 1.2);
            break;
            
        case PostFX::SEPIA_INK:
            {
                double gray = 0.299 * processed.red + 0.587 * processed.green + 0.114 * processed.blue;
                processed.red = std::min(1.0, gray * 1.2);
                processed.green = std::min(1.0, gray * 1.0);
                processed.blue = gray * 0.6;
            }
            break;
            
        case PostFX::SILVERTONE:
            {
                double gray = 0.299 * processed.red + 0.587 * processed.green + 0.114 * processed.blue;
                processed.red = gray;
                processed.green = gray;
                processed.blue = gray;
                processed.saturation = 0.0;
            }
            break;
            
        case PostFX::FILM_BW:
            {
                double gray = 0.299 * processed.red + 0.587 * processed.green + 0.114 * processed.blue;
                gray = 0.25 + (gray * 0.75);
                processed.red = gray;
                processed.green = gray;
                processed.blue = gray;
                processed.saturation = 0.0;
            }
            break;
            
        case PostFX::VIDEO_BW:
        case PostFX::PHOTOCOPY:
            {
                double gray = 0.299 * processed.red + 0.587 * processed.green + 0.114 * processed.blue;
                gray = 0.375 + (gray * 0.5);
                processed.red = gray;
                processed.green = gray;
                processed.blue = gray;
                processed.saturation = 0.0;
            }
            break;
            
        case PostFX::CONTRAST_BW:
            {
                double gray = 0.299 * processed.red + 0.587 * processed.green + 0.114 * processed.blue;
                gray = (gray > 0.5) ? 1.0 : 0.0;
                processed.red = gray;
                processed.green = gray;
                processed.blue = gray;
                processed.saturation = 0.0;
            }
            break;
            
        case PostFX::BLUE_FILTER:
            processed.red = processed.red * 0.2;
            processed.green = processed.green * 0.4;
            processed.blue = std::min(1.0, processed.blue * 1.2);
            break;
            
        case PostFX::DESAT_BLUE:
            processed.saturation = processed.saturation * 0.8;
            processed.blue = std::min(1.0, processed.blue * 1.1);
            break;
            
        case PostFX::VIDEO_SECURITY:
            processed.red = processed.red * 0.2;
            processed.blue = processed.blue * 0.2;
            processed.green = std::min(1.0, processed.green * 1.3);
            break;
            
        case PostFX::BRIGHT:
            processed.brightness = std::min(1.0, processed.brightness * 1.4);
            processed.red = std::min(1.0, processed.red + 0.15);
            processed.green = std::min(1.0, processed.green + 0.15);
            processed.blue = std::min(1.0, processed.blue + 0.15);
            break;
            
        case PostFX::POSTERIZE:
        case PostFX::DESAT_POSTERIZE:
            processed.red = std::round(processed.red * 3.0) / 3.0;
            processed.green = std::round(processed.green * 3.0) / 3.0;
            processed.blue = std::round(processed.blue * 3.0) / 3.0;
            if (postFx == PostFX::DESAT_POSTERIZE) {
                processed.saturation = processed.saturation * 0.6;
            }
            break;
            
        case PostFX::FLICKER_TRAILS:
            processed.brightness = processed.brightness * 0.8;
            processed.saturation = processed.saturation * 0.7;
            break;
            
        case PostFX::FILM_CONTRAST:
            processed.red = (processed.red < 0.5) ? processed.red * 0.7 : std::min(1.0, processed.red * 1.3);
            processed.green = (processed.green < 0.5) ? processed.green * 0.7 : std::min(1.0, processed.green * 1.3);
            processed.blue = (processed.blue < 0.5) ? processed.blue * 0.7 : std::min(1.0, processed.blue * 1.3);
            break;
            
        case PostFX::FILM_CONTRAST_BLUE:
            processed.red = (processed.red < 0.5) ? processed.red * 0.7 : std::min(1.0, processed.red * 1.3);
            processed.green = (processed.green < 0.5) ? processed.green * 0.7 : std::min(1.0, processed.green * 1.3);
            processed.blue = (processed.blue < 0.5) ? processed.blue * 0.7 : std::min(1.0, processed.blue * 1.3);
            processed.blue = std::min(1.0, processed.blue * 1.1);
            break;
            
        case PostFX::FILM_CONTRAST_GREEN:
            processed.red = (processed.red < 0.5) ? processed.red * 0.7 : std::min(1.0, processed.red * 1.3);
            processed.green = (processed.green < 0.5) ? processed.green * 0.7 : std::min(1.0, processed.green * 1.3);
            processed.blue = (processed.blue < 0.5) ? processed.blue * 0.7 : std::min(1.0, processed.blue * 1.3);
            processed.green = std::min(1.0, processed.green * 1.1);
            break;
            
        case PostFX::FILM_CONSTRAST_RED:
            processed.red = (processed.red < 0.5) ? processed.red * 0.7 : std::min(1.0, processed.red * 1.3);
            processed.green = (processed.green < 0.5) ? processed.green * 0.7 : std::min(1.0, processed.green * 1.3);
            processed.blue = (processed.blue < 0.5) ? processed.blue * 0.7 : std::min(1.0, processed.blue * 1.3);
            processed.red = std::min(1.0, processed.red * 1.1);
            break;
            
        case PostFX::HORROR_MOVIE:
            processed.green = processed.green * 0.2;
            processed.blue = processed.blue * 0.2;
            processed.red = std::min(1.0, processed.red * 1.5);
            break;
            
        case PostFX::PHOTO_NEGATIVE:
            processed.red = 1.0 - processed.red;
            processed.green = 1.0 - processed.green;
            processed.blue = 1.0 - processed.blue;
            break;
            
        case PostFX::MIRROR:
            {
                double avg = (processed.red + processed.green + processed.blue) / 3.0;
                processed.red = std::min(1.0, avg * 1.2);
                processed.green = std::min(1.0, avg * 1.1);
                processed.blue = avg * 0.6;
            }
            break;
            
        case PostFX::PSYCH_BLUE_RED:
            {
                double avg = (processed.red + processed.green + processed.blue) / 3.0;
                if (avg > 0.5) {
                    processed.red = 1.0;
                    processed.green = 0.0;
                    processed.blue = 0.0;
                } else {
                    processed.red = 0.0;
                    processed.green = 0.0;
                    processed.blue = 1.0;
                }
            }
            break;
            
        case PostFX::SPACE_WOOSH:
            processed.brightness = 1.0;
            processed.red = std::min(1.0, processed.red * 1.8);
            processed.green = std::min(1.0, processed.green * 1.8);
            processed.blue = std::min(1.0, processed.blue * 1.8);
            break;
            
        default:
            Logger::warning("Unknown PostFX enum value");
            break;
    }
    
    return processed;
}

// Super basic atm, all lights will share the same color
void LightDirector::applyToHueLights(const LightingData &finalLighting) {
    if (!m_isStreaming || !m_hueStream) {
        return;
    }
    
    auto effect = std::make_shared<huestream::AreaEffect>("VenHue", 2);
    effect->AddArea(huestream::Area::All);
    auto color = huestream::Color(finalLighting.red, finalLighting.green, finalLighting.blue);
    effect->SetFixedColor(color);
    effect->SetFixedOpacity(finalLighting.brightness);
    effect->Enable();
    
    m_hueStream->LockMixer();
    m_hueStream->AddEffect(effect);
    m_hueStream->UnlockMixer();
    
    Logger::info("Applied lighting: R=" + std::to_string(finalLighting.red) + 
                 " G=" + std::to_string(finalLighting.green) + 
                 " B=" + std::to_string(finalLighting.blue) + 
                 " Brightness=" + std::to_string(finalLighting.brightness));
}

// Generate base lighting from current LightingFX, then apply post-processing from current PostFX, before sending to Hue
void LightDirector::applyFullEffect() {
    LightingData baseLighting = generateBaseLighting(m_currentLightingFX);
    LightingData finalLighting = applyPostProcessing(baseLighting, m_currentPostFX);
    applyToHueLights(finalLighting);
}

void LightDirector::updateFromEffectString(const std::string &effectName) {
    // TODO: Parse the effect string to separate LightingFX and PostFX
    
    Logger::info("Updating lights from effect string: " + effectName);
    
    LightingFX lightingFx = stringToLightingFX(effectName);
    m_currentLightingFX = lightingFx;
    
    // PostFX postFx = stringToPostFX(postFxName);
    // m_currentPostFX = postFx;
    applyFullEffect();
}

// Receives effect changes from VenueMonitor
void LightDirector::onEffectChanged(const std::string &effectName, const VenueData &data, double currentTime) {
    Logger::info("Effect changed to: " + effectName + " at time: " + std::to_string(currentTime));
    updateFromEffectString(effectName);
}

void LightDirector::shutdown() {
    if (m_hueStream && m_isStreaming) {
        m_hueStream->ShutDown();
        m_isStreaming = false;
        Logger::info("LightDirector: Streaming connection closed");
    }
}
