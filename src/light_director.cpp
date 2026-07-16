#include "light_director.h"
#include "animated_light_effect.h"
#include "light_effect_loader.h"
#include <QHostInfo>
#include <QMetaObject>
#include <QTimer>
#include <algorithm>
#include <array>
#include <string_view>
#include <utility>
#include <vector>
#include <huestream/HueStream.h>
#include <huestream/effect/effects/AreaEffect.h>
#include <huestream/common/data/Area.h>
#include <huestream/common/data/Color.h>
#include <logger.h>

namespace {

constexpr std::array ANIMATED_EFFECTS{
    std::pair{LightDirector::LightingFX::IDLE, std::string_view{"idle"}},
    std::pair{LightDirector::LightingFX::LOOP_COOL, std::string_view{"loop_cool"}},
    std::pair{LightDirector::LightingFX::LOOP_WARM, std::string_view{"loop_warm"}},
    std::pair{LightDirector::LightingFX::HARMONY, std::string_view{"harmony"}},
    std::pair{LightDirector::LightingFX::FRENZY, std::string_view{"frenzy"}},
    std::pair{LightDirector::LightingFX::SILHOUETTES, std::string_view{"silhouettes"}},
    std::pair{LightDirector::LightingFX::SILHOUETTES_SPOT, std::string_view{"silhouettes_spot"}},
    std::pair{LightDirector::LightingFX::BRE, std::string_view{"bre"}}};

std::map<LightDirector::LightingFX, AnimatedEffectDefinition> loadAnimatedEffects() {
    std::vector<std::string_view> names;
    names.reserve(ANIMATED_EFFECTS.size());
    for (const auto &[fx, name] : ANIMATED_EFFECTS) {
        names.push_back(name);
    }

    AnimatedEffectDefinitions byName = loadAnimatedEffectDefinitions(names);
    std::map<LightDirector::LightingFX, AnimatedEffectDefinition> byFx;
    for (const auto &[fx, name] : ANIMATED_EFFECTS) {
        const auto definition = byName.find(name);
        if (definition != byName.end()) {
            byFx.emplace(fx, std::move(definition->second));
        }
    }
    return byFx;
}

} // namespace

LightDirector::LightDirector(QObject *parent)
    : QObject(parent), m_animatedEffectDefinitions(loadAnimatedEffects()), m_connectionState(new HueConnectionState(this)), m_currentLightingFX(LightingFX::IDLE), m_currentPostFX(PostFX::DEFAULT) {
    // huestream stores the config by itself, don't need to have any QSettings stuff for it anymore
    m_config = std::make_shared<huestream::Config>("VenHue", QHostInfo::localHostName().toStdString(), huestream::PersistenceEncryptionKey("VenHuePersistenceKey"));
    m_hueStream = std::make_shared<huestream::HueStream>(m_config);

    // Register connection flow feedback callback
    m_hueStream->RegisterFeedbackCallback([this](const huestream::FeedbackMessage &message) {
        const int messageId = static_cast<int>(message.GetId());
        const int requestType = static_cast<int>(message.GetRequestType());
        QMetaObject::invokeMethod(this, [this, messageId, requestType]() {
            if (!m_shuttingDown) {
                handleHueStreamFeedback(messageId, requestType);
            } }, Qt::QueuedConnection);
    });
}

void LightDirector::handleHueStreamFeedback(int rawMessageId, int rawRequestType) {
    using namespace huestream;
    const auto messageId = static_cast<FeedbackMessage::Id>(rawMessageId);
    const auto requestType = static_cast<FeedbackMessage::RequestType>(rawRequestType);
    Logger::hue("Hue feedback message=" + std::to_string(rawMessageId) + " request=" + std::to_string(rawRequestType));

    switch (messageId) {
        case FeedbackMessage::ID_START_SEARCHING:
            m_connectionState->searching();
            break;
        case FeedbackMessage::ID_PRESS_PUSH_LINK:
            m_connectionState->awaitingLink();
            break;
        case FeedbackMessage::ID_FINISH_AUTHORIZING_AUTHORIZED:
        case FeedbackMessage::ID_START_RETRIEVING:
        case FeedbackMessage::ID_START_RETRIEVING_SMALL:
        case FeedbackMessage::ID_FINISH_RETRIEVING_SMALL:
            m_connectionState->loadingBridge();
            break;
        case FeedbackMessage::ID_START_SAVING:
        case FeedbackMessage::ID_FINISH_SAVING_SAVED:
            break;
        case FeedbackMessage::ID_BRIDGE_CONNECTED:
            refreshEntertainmentAreas();
            break;
        case FeedbackMessage::ID_FINISH_LOADING_BRIDGE_CONFIGURED:
            m_connectionState->bridgeLoaded(true);
            refreshEntertainmentAreas();
            m_connectionState->loadConfirmedArea(selectedAreaId(), selectedAreaName());
            if (requestType == FeedbackMessage::REQUEST_TYPE_LOAD_BRIDGE) {
                connectAfterCallback();
            }
            break;
        case FeedbackMessage::ID_FINISH_LOADING_NO_BRIDGE_CONFIGURED:
            m_connectionState->bridgeLoaded(false);
            break;
        case FeedbackMessage::ID_FINISH_RETRIEVING_READY_TO_START:
            refreshEntertainmentAreas();
            m_connectionState->configurationRetrieved(m_connectionState->entertainmentAreas(), false);
            break;
        case FeedbackMessage::ID_FINISH_RETRIEVING_ACTION_REQUIRED:
            refreshEntertainmentAreas();
            m_connectionState->configurationRetrieved(m_connectionState->entertainmentAreas(), true);
            break;
        case FeedbackMessage::ID_SELECT_GROUP:
            refreshEntertainmentAreas();
            m_connectionState->requireAreaSelection();
            break;
        case FeedbackMessage::ID_BRIDGE_CHANGED:
            refreshEntertainmentAreas();
            break;
        case FeedbackMessage::ID_GROUPLIST_UPDATED: {
            const bool wasWaitingForAreas = m_connectionState->hueState() == HueConnectionState::NoAreas;
            refreshEntertainmentAreas();
            const auto areas = m_connectionState->entertainmentAreas();
            if (wasWaitingForAreas && areas.size() == 1) {
                selectEntertainmentArea(areas.front().id);
            }
            break;
        }
        case FeedbackMessage::ID_START_ACTIVATING:
            m_connectionState->activatingArea();
            break;
        case FeedbackMessage::ID_FINISH_ACTIVATING_ACTIVE:
        case FeedbackMessage::ID_STREAMING_CONNECTED:
            confirmStreaming();
            break;
        case FeedbackMessage::ID_STREAMING_DISCONNECTED:
        case FeedbackMessage::ID_BRIDGE_DISCONNECTED:
            m_connectionState->disconnected();
            Logger::info("Entertainment streaming disconnected");
            break;
        case FeedbackMessage::ID_DONE_ABORTED:
            m_connectionState->cancel();
            break;
        case FeedbackMessage::ID_DONE_RESET:
            break;
        case FeedbackMessage::ID_USERPROCEDURE_FINISHED:
            if (requestType == FeedbackMessage::REQUEST_TYPE_RESET_ALL
                && m_connectionState->hueState() == HueConnectionState::Resetting) {
                m_connectionState->finishReset();
            }
            break;
        case FeedbackMessage::ID_FINISH_SEARCHING_NO_BRIDGES_FOUND:
        case FeedbackMessage::ID_DONE_NO_BRIDGE_FOUND:
        case FeedbackMessage::ID_NO_BRIDGE_FOUND:
        case FeedbackMessage::ID_BRIDGE_NOT_FOUND:
        case FeedbackMessage::ID_FINISH_SEARCHING_INVALID_BRIDGES_FOUND:
        case FeedbackMessage::ID_FINISH_AUTHORIZING_FAILED:
        case FeedbackMessage::ID_FINISH_RETRIEVING_FAILED:
        case FeedbackMessage::ID_FINISH_SAVING_FAILED:
        case FeedbackMessage::ID_FINISH_ACTIVATING_FAILED:
        case FeedbackMessage::ID_INVALID_MODEL:
        case FeedbackMessage::ID_INVALID_VERSION:
        case FeedbackMessage::ID_NO_GROUP_AVAILABLE:
        case FeedbackMessage::ID_BUSY_STREAMING:
        case FeedbackMessage::ID_CANNOT_RECONNECT_TOO_MANY_REQUEST:
        case FeedbackMessage::ID_INTERNAL_ERROR:
            failForMessage(rawMessageId, rawRequestType);
            break;

        default:
            break;
    }
}

void LightDirector::initializeHue() {
    m_connectionState->beginInitialization();
    m_hueStream->LoadBridgeInfoAsync();
}

void LightDirector::connectAfterCallback() {
    // wait to connect untill EDK callback returns
    QTimer::singleShot(0, this, [this]() {
        if (!m_shuttingDown) {
            m_hueStream->ConnectBridgeAsync();
        }
    });
}

void LightDirector::connectHue() {
    m_connectionState->beginConnect(false);
    m_hueStream->ConnectBridgeAsync();
}

void LightDirector::cancelHueConnection() {
    m_hueStream->AbortConnectingAsync();
    m_connectionState->cancel();
}

void LightDirector::retryHueConnection() {
    switch (m_connectionState->retryAction()) {
        case HueConnectionState::RetrySelection:
            if (!m_connectionState->pendingAreaId().empty()) {
                selectEntertainmentArea(m_connectionState->pendingAreaId());
                return;
            }
            refreshEntertainmentAreas();
            m_connectionState->requireAreaSelection();
            return;
        case HueConnectionState::RetryReset:
            resetAllHueData();
            return;
        case HueConnectionState::RetryReconnect:
            m_connectionState->beginConnect(true);
            m_hueStream->ConnectBridgeAsync();
            return;
        case HueConnectionState::RetryConnect:
        case HueConnectionState::RetryNone:
            connectHue();
            return;
    }
}

void LightDirector::selectEntertainmentArea(const std::string &id) {
    if (m_connectionState->hueState() == HueConnectionState::Streaming
        && m_connectionState->currentAreaId() == id) {
        return;
    }

    auto groups = m_hueStream->GetLoadedBridgeGroups();
    if (!groups) {
        m_connectionState->fail("Entertainment Areas could not be loaded.",
                                HueConnectionState::RetryReconnect);
        return;
    }
    for (const auto &group : *groups) {
        if (group->GetId() == id) {
            m_connectionState->beginAreaSelection(id);
            m_hueStream->SelectGroupAsync(group);
            return;
        }
    }
    m_connectionState->fail("Entertainment Area no longer available.",
                            HueConnectionState::RetryReconnect);
}

void LightDirector::resetAllHueData() {
    m_connectionState->beginReset();
    m_hueStream->ResetAllPersistentDataAsync();
}

void LightDirector::refreshEntertainmentAreas() {
    EntertainmentAreas areas;
    auto groups = m_hueStream->GetLoadedBridgeGroups();
    if (groups) {
        for (const auto &group : *groups) {
            areas.push_back({group->GetId(), group->GetName()});
        }
    }
    m_connectionState->updateAreas(areas);
}

void LightDirector::confirmStreaming() {
    refreshEntertainmentAreas();
    const std::string areaId = selectedAreaId();
    const bool alreadyStreaming = m_connectionState->hueState() == HueConnectionState::Streaming
        && m_connectionState->currentAreaId() == areaId;
    m_connectionState->confirmStreaming(areaId, selectedAreaName());
    if (alreadyStreaming) {
        return;
    }
    Logger::info("Streaming now active");
    applyFullEffect();
}

std::string LightDirector::selectedAreaId() const {
    auto bridge = m_hueStream->GetLoadedBridge();
    auto group = bridge ? bridge->GetGroup() : nullptr;
    return group ? group->GetId() : std::string();
}

std::string LightDirector::selectedAreaName() const {
    auto bridge = m_hueStream->GetLoadedBridge();
    auto group = bridge ? bridge->GetGroup() : nullptr;
    return group ? group->GetName() : "Hue Entertainment Area";
}

void LightDirector::failForMessage(int rawMessageId, int rawRequestType) {
    using Message = huestream::FeedbackMessage;
    const auto id = static_cast<Message::Id>(rawMessageId);
    const auto requestType = static_cast<Message::RequestType>(rawRequestType);
    if (requestType == Message::REQUEST_TYPE_RESET_ALL) {
        m_connectionState->fail("Unpairing failed.",
                                HueConnectionState::RetryReset);
        return;
    }
    std::string text;
    auto retry = HueConnectionState::RetryConnect;
    switch (id) {
        case Message::ID_FINISH_SEARCHING_NO_BRIDGES_FOUND:
        case Message::ID_DONE_NO_BRIDGE_FOUND:
        case Message::ID_NO_BRIDGE_FOUND:
            text = "No Hue bridge found.";
            break;
        case Message::ID_BRIDGE_NOT_FOUND:
        case Message::ID_CANNOT_RECONNECT_TOO_MANY_REQUEST:
            text = "Cannot connect to previous bridge.";
            retry = HueConnectionState::RetryReconnect;
            break;
        case Message::ID_FINISH_AUTHORIZING_FAILED:
            text = "Bridge authorization failed.";
            break;
        case Message::ID_INVALID_MODEL:
            text = "Unsupported Hue bridge.";
            break;
        case Message::ID_INVALID_VERSION:
            text = "Outdated Hue bridge.";
            break;
        case Message::ID_NO_GROUP_AVAILABLE:
            m_connectionState->updateAreas({});
            m_connectionState->requireAreaSelection();
            return;
        case Message::ID_BUSY_STREAMING:
            text = "Entertainment Area already in use by another application. Close it and try again.";
            retry = HueConnectionState::RetryReconnect;
            break;
        case Message::ID_FINISH_RETRIEVING_FAILED:
            text = "Bridge config retrieval failed.";
            retry = HueConnectionState::RetryReconnect;
            break;
        case Message::ID_FINISH_SAVING_FAILED:
            text = "Unable to save Hue bridge setup.";
            retry = HueConnectionState::RetryReconnect;
            break;
        case Message::ID_FINISH_ACTIVATING_FAILED:
            text = "Entertainment Area could not be activated.";
            retry = HueConnectionState::RetryReconnect;
            break;
        default:
            text = "Internal Hue error.";
            retry = HueConnectionState::RetryReconnect;
            break;
    }
    if (requestType == Message::REQUEST_TYPE_SELECT && (id == Message::ID_FINISH_RETRIEVING_FAILED || id == Message::ID_FINISH_SAVING_FAILED || id == Message::ID_FINISH_ACTIVATING_FAILED || id == Message::ID_BUSY_STREAMING)) {
        retry = HueConnectionState::RetrySelection;
    }
    m_connectionState->fail(text, retry);
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
    if (!m_hueStream || m_connectionState->hueState() != HueConnectionState::Streaming) {
        return;
    }
    
    auto effect = std::make_shared<huestream::AreaEffect>("VenHue", 2);
    effect->AddArea(huestream::Area::All);
    auto color = huestream::Color(finalLighting.red, finalLighting.green, finalLighting.blue);
    effect->SetFixedColor(color);
    effect->SetFixedOpacity(finalLighting.brightness);
    replaceActiveEffect(effect);
    
    Logger::info("Applied lighting: R=" + std::to_string(finalLighting.red) + 
                 " G=" + std::to_string(finalLighting.green) + 
                 " B=" + std::to_string(finalLighting.blue) + 
                 " Brightness=" + std::to_string(finalLighting.brightness));
}

void LightDirector::replaceActiveEffect(const huestream::EffectPtr &effect) {
    m_hueStream->LockMixer();
    if (m_activeEffect) {
        m_activeEffect->Finish();
    }
    m_hueStream->AddEffect(effect);
    effect->Enable();
    m_activeEffect = effect;
    m_hueStream->UnlockMixer();
}

void LightDirector::applyAnimatedEffect(const AnimatedEffectDefinition &definition) {
    if (!m_hueStream || m_connectionState->hueState() != HueConnectionState::Streaming) {
        return;
    }

    auto effect = std::make_shared<AnimatedLightEffect>("VenHue", 2, definition);
    replaceActiveEffect(effect);
}

// Generate base lighting from current LightingFX, then apply post-processing from current PostFX, before sending to Hue
void LightDirector::applyFullEffect() {
    const auto definition = m_animatedEffectDefinitions.find(m_currentLightingFX);
    if (definition != m_animatedEffectDefinitions.end()) {
        applyAnimatedEffect(definition->second);
        return;
    }
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

void LightDirector::onSongStateChanged(bool isPlaying) {
    if (!isPlaying) {
        m_currentLightingFX = LightingFX::IDLE;
        applyFullEffect();
    }
}

void LightDirector::shutdown() {
    if (m_hueStream && !m_shuttingDown) {
        m_shuttingDown = true;
        m_hueStream->RegisterFeedbackCallback({});
        m_hueStream->ShutDown();
        Logger::info("LightDirector: Hue connection closed.");
    }
}
