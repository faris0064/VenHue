#pragma once
#include "hue_connection_state.h"
#include "parser.h"
#include <QObject>
#include <QSettings>
#include <QTimer>
#include <memory>

#include <huestream/HueStream.h>
#include <huestream/config/Config.h>
#include <huestream/connect/IFeedbackMessageHandler.h>

// Handles communication between VenHue and Phillips Hue.
class LightDirector : public QObject {
    Q_OBJECT

public:

    // Base lighting
    enum class LightingFX {
        IDLE, // Blue, ambient "night time" lighting

        VERSE, // Tends towarrds soft yet full blends, such as orange and green.
        CHORUS, // Tends towards stark, dramatic colors, such as saturated blue and red. Invokes a peak state.
        MANUAL_COOL, // Cool temperature lighting
        MANUAL_WARM, // Warm temperature lighting
        DISCHORD, // Harsh lighting with a blend of dissonant colors (Yellow, blue, red?)
        STOMP, // All lights are either on or off

        LOOP_COOL, // A blend of cool temperature colors
        LOOP_WARM, // A blend of warm temperature colors
        HARMONY, // A blend of lights with a harmonious color palette (Orange, purple, blue, etc.)
        FRENZY, // Frentic dissonant lighting that alternates quickly (Blue, green, purple, yellow? maybe more)
        SILHOUETTES, // Dark, atmospheric lighting. Blueish colors
        SILHOUETTES_SPOT, // Dark atmospheric lighting, slightly brighter. Blueish colors
        SEARCHLIGHTS, // Searchlights that sweep individually. Maybe sweep between lights at max brightness
        SWEEP, // Lights that sweep together in banks
        STROBE_SLOW, // Strobe lights that blink every 8th note
        STROBE_FAST, // Strobe lights that blink every 16th note
        BLACKOUT_SLOW, // Turn off all lights
        BLACKOUT_FAST, // Turn off all lights
        BLACKOUT_SPOT, // Turn off all lights
        FLARE_SLOW, // Bright white flare
        FLARE_FAST, // Bright white flare
        BRE // Frentic lighting for a Big Rock Ending. Looks like frenzy, only crazier.
    };

    // Effects that add modifiers to lighting
    enum class PostFX {
        DEFAULT, // No changes
        COLOR_MUTED, // Slightly mutes all colors
        VIDEO_GRAINY, // No effects to lighting
        SIXTEENMM_FILM, // No effects to lighting
        SHITTY_TV, // Dramatically lightens colors
        BLOOM, // Brightens slightly
        SEPIA_INK, // Reduces colors to a wide range of yellowish-grey shades
        SILVERTONE, // Reduces colors to a wide range of grey shades
        FILM_BW, // Reduces colors to a range of grey tones, the range being smaller than Silvertone
        VIDEO_BW, // Reduces colors to grey shades. A smaller range of grey than Silvertone.
        CONTRAST_BW, // Somewhat polarized black/white
        PHOTOCOPY, // Same as VIDEO_BW
        BLUE_FILTER, // Reduces colors to a wide range of blue shades.
        DESAT_BLUE, // Slightly gainy colors, with a blue tinge.
        VIDEO_SECURITY, // Colors are reduced to a wide range of green shades.

        BRIGHT, // Brightens to bloom-esque effect. Lightens dark colors
        POSTERIZE, // Flattens colors
        CLEAN_TRAILS, // No effects to lighting
        VIDEO_TRAILS, // No effects to lighting
        FLICKER_TRAILS, // Slightly darkens lighting and mutes colors
        DESAT_POSTERIZE, // Flattens colors
        FILM_CONTRAST, // Makes dark colors darker, and light colors lighter.
        FILM_CONTRAST_BLUE, // Makes dark colors darker, and light colors lighter. Adds a slight blue hue.
        FILM_CONTRAST_GREEN, // Makes dark colors darker, and light colors lighter. Adds a slight green hue.
        FILM_CONSTRAST_RED, // Makes dark colors darker, and light colors lighter. Adds a slight red hue. 
        HORROR_MOVIE, // Make everything red.
        PHOTO_NEGATIVE, // Inverts colors
        MIRROR, // Changes colors to a variety of oranges, greens, and yellows
        PSYCH_BLUE_RED, // Polarizes colors to either red or blue
        SPACE_WOOSH // Lightens colors dramatically.
    };

    explicit LightDirector(QObject *parent = nullptr);

    HueConnectionState *connectionState() const { return m_connectionState; }
    void initializeHue();
    void connectHue();
    void cancelHueConnection();
    void retryHueConnection();
    void selectEntertainmentArea(const std::string &id);
    void resetAllHueData();
    void shutdown();

private:
    void handleHueStreamFeedback(int messageId, int requestType);
    void refreshEntertainmentAreas();
    void connectAfterCallback();
    void confirmStreaming();
    void failForMessage(int messageId, int requestType);
    std::string selectedAreaId() const;
    std::string selectedAreaName() const;

    std::shared_ptr<huestream::Config> m_config;
    std::shared_ptr<huestream::HueStream> m_hueStream;
    HueConnectionState *m_connectionState;
    bool m_shuttingDown = false;

    LightingFX m_currentLightingFX;
    PostFX m_currentPostFX;
    
    LightingFX stringToLightingFX(const std::string &effectName);
    PostFX stringToPostFX(const std::string &effectName);
    
    // struct to be parsed by applyToHueLights() to determine color, brightness, and saturation.
    struct LightingData {
        double red = 0.0;            
        double green = 0.0;          
        double blue = 0.0;           
        double brightness = 1.0;     
        double saturation = 1.0;     
        bool strobe = false;
        double strobeRate = 0.0; // in hz right now, should be dependant on bpm
    };
    
    // Generate the base light data with no modifiers applied
    LightingData generateBaseLighting(LightingFX fx);

    // Generate lighting data for modifiers
    LightingData applyPostProcessing(const LightingData &baseLighting, PostFX postFx);

    // Apply lighting data to Hue lights
    void applyToHueLights(const LightingData &finalLighting);
    
    // Combine base lighting with any applicable modifiers
    void applyFullEffect();
    
    // Convert string to effect enum
    void updateFromEffectString(const std::string &effectName);

public slots:
    void onEffectChanged(const std::string &effectName, const VenueData &data, double currentTime);
};
