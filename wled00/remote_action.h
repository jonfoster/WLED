/*
 * WLED functions that are triggered by IR/Wireless remote controls.
 *
 * WLED supports several different remote controls, and each remote control
 * wants to do more or less the same things when a button is pressed.
 *
 * To avoid code duplication, this file contains functions that are
 * called from those remote control drivers when a button is pressed.
 */

#ifndef WLED_REMOTE_ACTION_H
#define WLED_REMOTE_ACTION_H

#include <stdint.h>

enum SegmentsFilter {
    /** Follow irApplyToAllSelected global variable */
    SegmentsFilter_IR_REMOTE_SETTING,
    
    /** Target main segment only. */
    SegmentsFilter_MAIN,

    /** Target all segments that are both selected and active.
     * If none, falls back to main segment.
     */
    SegmentsFilter_SELECTED,

    /** Target all active segments.
     * If none, falls back to main segment.
     */
    SegmentsFilter_ACTIVE
};

typedef enum UiJsonActionResult {
  UiJsonActionResult_OK = 0,
  UiJsonActionResult_OK_REPEATABLE,
  UiJsonActionResult_ERR_LOCK,
  UiJsonActionResult_ERR_NO_FILE,
  UiJsonActionResult_ERR_CODE_NOT_IN_FILE,
  UiJsonActionResult_ERR_CODE_NO_ACTION
} UiJsonActionResult;

class UiAction {
public:

    explicit UiAction(SegmentsFilter filter) : m_filter(filter) {}

    UiAction() = delete;

    UiAction(const UiAction&) = default;
    UiAction(UiAction&&) = default;
    UiAction& operator=(const UiAction&) = default;
    UiAction& operator=(UiAction&&) = default;

    /**
     * Turn on if off, or turn off if on.
     */
    void turnOnOffToggle();

    /**
     * Turn on.  If already on, do nothing.
     */
    bool turnOn();

    /**
     * Turn off.  If already off, do nothing.
     */
    bool turnOff();

    static inline bool nightModeActive() {
        return brightnessBeforeNightMode != NIGHT_MODE_DEACTIVATED;
    }

    // Warning: "Night mode" and "Nightlight mode" are completely different things.

    bool activateNightMode();
    bool resetNightMode();

    /**
     * Increment brightness to the next step.
     * This uses a logarithmic scale of brightness levels.
     * If already at maximum brightness, do nothing.
     */
    bool incBrightness();
    
    bool incBrightnessAlternate();

    /**
     * Decrement brightness to the next nonzero step.
     * This uses a logarithmic scale of brightness levels.
     * Will not actually turn off the light.
     */
    bool decBrightness();

    bool decBrightnessAlternate();

    /**
     * Increment brightness by the specified delta.
     * If that would exceed the maximum brightness, set to maximum.
     * If already at maximum brightness, do nothing.
     */
    bool incBrightnessBy(uint8_t delta);

    /**
     * Decrement brightness by the specified delta.
     * If that would go below 1 brightness, set to 1.
     * (That includes the case where the light is off, this function
     * will turn the light on to 1 brightness.)
     * If already at minimum brightness, do nothing.
     */
    bool decBrightnessBy(uint8_t delta);

    void setBrightness(uint8_t brightness);

    void nightlightStart();

    void preset(uint8_t presetID);
    void presetWithFallback(uint8_t presetID, uint8_t effectID, uint8_t paletteID);

    void changeColor(uint32_t c, int16_t cct = -1);
    void changeColorStatic(uint32_t c, int16_t cct = -1);
    void setColorRandom();
    void changeHueRelative(int8_t delta);
    void changeSaturationRelative(int8_t delta);
    
    void changeWhite(int8_t amount);
    void whiteOff();
    void whiteOn();
    void setWhiteAndChangeCctRelative(uint32_t c, int8_t delta);
    void changeCct(uint16_t cct);

    void setToPlainStaticBrightWhite();

    uint32_t getNextColorInCycle();

    void nextColorAndPalette();

    void changePalette(uint8_t pal);
    void nextPalette();
    void prevPalette();

    void changeEffect(uint8_t fx);
    void nextEffect();
    void prevEffect();

    void changeEffectSpeed(uint8_t speed);
    void changeEffectSpeedRelative(int8_t delta);
    void incEffectSpeed();
    void decEffectSpeed();

    void changeEffectIntensity(uint8_t intensity);
    void changeEffectIntensityRelative(int8_t delta);
    void incEffectIntensity();
    void decEffectIntensity();

    void incEffectSpeedOrHue();
    void decEffectSpeedOrHue();
    void incEffectIntensityOrSaturation();
    void decEffectIntensityOrSaturation();

    void changeCustomRelative(uint8_t which, int8_t delta);

    UiJsonActionResult runJson(uint8_t moduleID, const char fileName[], const char objKey[]);

    SegmentsFilter m_filter;

protected:
    void doUpdate();
    
    static int16_t brightnessBeforeNightMode;
    
    static constexpr int16_t NIGHT_MODE_DEACTIVATED = -1;
    static constexpr uint8_t NIGHT_MODE_BRIGHTNESS = 5;

private:
    void changeColorEffectAndPalette(uint32_t c, int16_t cct, int16_t effect, int16_t pal);

    void changeHueSaturationRelative(int8_t hue_delta, int8_t sat_delta);

    static uint8_t colorCycleIndex;

    static const uint8_t brightnessSteps[];
    static const uint32_t colorCycle[];
};

extern UiAction RemoteAction;

#endif