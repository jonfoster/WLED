/*
 * WLED functions that are triggered by IR/Wireless remote controls.
 *
 * WLED supports several different remote controls, and each remote control
 * wants to do more or less the same things when a button is pressed.
 *
 * To avoid code duplication, this file contains functions that are
 * called from those remote control drivers when a button is pressed.
 */

#include "wled.h"
#include "remote_action.h"

class SegmentIterator {
public:
  /**
   * Create an iterator pointing to the first segment to target.
   */
  SegmentIterator(SegmentsFilter filter);

  /**
   * Check if gone past the end of the segments.
   *
   * @return true if there is a current segment to work with,
   *         false if all the segments have been processed.
   */
  operator bool() const
  {
      return m_pointer != nullptr;
  }

  /**
   * Move to the next segment.
   *
   * @return true if there is a next segment to work with,
   *         false if all the segments have been processed.
   */
  bool next();

  /**
   * Get the current segment.
   */
  Segment& operator*() const
  {
    return *m_pointer;
  }

  /**
   * Get the current segment.
   */
  Segment* operator->() const
  {
    return m_pointer;
  }

  /**
   * Get the first segment that was returned by this iterator.
   */
  Segment& first()
  {
    return *m_first;
  }

private:
  SegmentsFilter m_filter;
  uint8_t m_index;
  uint8_t m_count;
  Segment *m_pointer;
  Segment *m_first;
};

#define COLOR_WHITE          0xFFFFFFFF
#define COLOR_RED            0xFF0000
#define COLOR_REDDISH        0xFF7800
#define COLOR_ORANGE         0xFFA000
#define COLOR_YELLOWISH      0xFFC800
#define COLOR_GREEN          0x00FF00
#define COLOR_GREENISH       0x00FF78
#define COLOR_TURQUOISE      0x00FFA0
#define COLOR_CYAN           0x00FFDC
#define COLOR_AQUA           0x00C8FF
#define COLOR_BLUE           0x00A0FF
#define COLOR_DEEPBLUE       0x0000FF
#define COLOR_PURPLE         0xAA00FF
#define COLOR_PINK           0xFF00A0

UiAction RemoteAction(SegmentsFilter_IR_REMOTE_SETTING);

int16_t UiAction::brightnessBeforeNightMode = NIGHT_MODE_DEACTIVATED;

// brightnessSteps: a static array of brightness levels following a geometric
// progression.  Can be generated from the following Python, adjusting the
// arbitrary 4.5 value to taste:
//
// def values(level):
//     while level >= 5:
//         yield int(level)
//         level -= level / 4.5
// result = [v for v in reversed(list(values(255)))]
// print("%d values: %s" % (len(result), result))
//
// It would be hard to maintain repeatable steps if calculating this on the fly.
const uint8_t UiAction::brightnessSteps[] = {
  5, 7, 9, 12, 16, 20, 26, 34, 43, 56, 72, 93, 119, 154, 198, 255
};

uint8_t UiAction::colorCycleIndex = 0;

const uint32_t UiAction::colorCycle[] = {
  COLOR_WHITE,
  COLOR_RED,
  COLOR_REDDISH,
  COLOR_ORANGE,
  COLOR_YELLOWISH,
  COLOR_GREEN,
  COLOR_GREENISH,
  COLOR_TURQUOISE,
  COLOR_CYAN,
  COLOR_BLUE,
  COLOR_DEEPBLUE,
  COLOR_PURPLE,
  COLOR_PINK,
};

SegmentIterator::SegmentIterator(SegmentsFilter filter) :
    m_filter(filter)
{
  if (m_filter == SegmentsFilter_IR_REMOTE_SETTING) {
    m_filter = (irApplyToAllSelected ? SegmentsFilter_SELECTED : SegmentsFilter_MAIN);
  }
  if (m_filter == SegmentsFilter_SELECTED) {
    m_count = strip.getSegmentsNum();
    m_index = strip.getFirstSelectedSegId();
    m_pointer = &strip.getSegment(m_index);
  } else if (m_filter == SegmentsFilter_ACTIVE) {
    m_count = strip.getSegmentsNum();
    m_index = 0;
    while (m_index < m_count && !strip.getSegment(m_index).isActive()) {
      m_index++;
    }
    if (m_index >= m_count) {
      // no active segments, use main segment
      // (Not sure if this can ever happen?)
      m_index = strip.getMainSegmentId();
    }
    m_pointer = &strip.getSegment(m_index);
  } else {
    m_pointer = &strip.getMainSegment();
  }
  m_first = m_pointer;
}

bool SegmentIterator::next()
{
  if (m_pointer == nullptr) {
    return false;
  }
  if (m_filter == SegmentsFilter_SELECTED) {
    do {
      m_index++;
      if (m_index >= m_count)
      {
        m_pointer = nullptr;
        return false;
      }
      m_pointer = &strip.getSegment(m_index);
    } while (!(m_pointer->isActive() && m_pointer->isSelected()));
    return true;
  } else if (m_filter == SegmentsFilter_ACTIVE) {
    do {
      m_index++;
      if (m_index >= m_count)
      {
        m_pointer = nullptr;
        return false;
      }
      m_pointer = &strip.getSegment(m_index);
    } while (!m_pointer->isActive());
    return true;
  } else {
    // only one segment is being used
    m_pointer = nullptr;
    return false;
  }
}

void UiAction::doUpdate()
{
  // for notifier, IR is considered a button input
  stateUpdated(CALL_MODE_BUTTON);
}

void UiAction::turnOnOffToggle()
{
  if (bri == 0) {
    turnOn();
  } else {
    turnOff();
  }
}

bool UiAction::turnOn()
{
  bool changed = false;
  if (brightnessBeforeNightMode != NIGHT_MODE_DEACTIVATED) {
    bri = brightnessBeforeNightMode;
    brightnessBeforeNightMode = NIGHT_MODE_DEACTIVATED;
    changed = true;
  }
  if (bri == 0) {
    bri = briLast;
    strip.restartRuntime();
    stateChanged = true;
    changed = true;
  }
  if (changed) {
    doUpdate();
    return true;
  }
  return false;
}

bool UiAction::turnOff()
{
  bool changed = false;
  if (brightnessBeforeNightMode != NIGHT_MODE_DEACTIVATED) {
    bri = brightnessBeforeNightMode;
    brightnessBeforeNightMode = NIGHT_MODE_DEACTIVATED;
    changed = true;
  }
  if (bri != 0) {
    briLast = bri;
    bri = 0;
    nightlightActive = false;
    brightnessBeforeNightMode = NIGHT_MODE_DEACTIVATED;
    stateChanged = true;
    changed = true;
  }
  if (changed) {
    doUpdate();
    return true;
  }
  return false;
}

bool UiAction::activateNightMode()
{
  if (nightModeActive()) {
    return false;
  }
  brightnessBeforeNightMode = bri;
  bri = NIGHT_MODE_BRIGHTNESS;
  doUpdate();
  return true;
}

bool UiAction::resetNightMode()
{
  if (!nightModeActive()) {
    return false;
  }
  bri = brightnessBeforeNightMode;
  brightnessBeforeNightMode = NIGHT_MODE_DEACTIVATED;
  doUpdate();
  return true;
}

// increment `bri` to the next `brightnessSteps` value
bool UiAction::incBrightness()
{
  if (nightModeActive()) {
    return false;
  }

  // dumb incremental search is efficient enough for so few items
  const size_t numBrightnessSteps = sizeof(brightnessSteps) / sizeof(brightnessSteps[0]);
  for (unsigned index = 0; index < numBrightnessSteps; ++index)
  {
    if (brightnessSteps[index] > bri)
    {
      bri = brightnessSteps[index];
      doUpdate();
      return true;
    }
  }
  return false;
}

bool UiAction::incBrightnessAlternate()
{
  if (nightModeActive()) {
    return false;
  }

  // slower steps when brightness < 16%
  uint8_t delta = (bri < 40) ? 2 : 5;
  return incBrightnessBy(delta);
}

bool UiAction::incBrightnessBy(uint8_t delta)
{
  if (nightModeActive()) {
    return false;
  }

  if (bri == 255) {
    return false;
  }

  if (bri >= 255 - delta) {
    bri = 255;
  } else {
    bri += delta;
  }

  doUpdate();
  return true;
}

// decrement `bri` to the next `brightnessSteps` value
bool UiAction::decBrightness()
{
  if (nightModeActive()) {
    return false;
  }

  // dumb incremental search is efficient enough for so few items
  const size_t numBrightnessSteps = sizeof(brightnessSteps) / sizeof(brightnessSteps[0]);
  for (int index = numBrightnessSteps - 1; index >= 0; --index)
  {
    if (brightnessSteps[index] < bri)
    {
      bri = brightnessSteps[index];
      doUpdate();
      return true;
    }
  }
  return false;
}

bool UiAction::decBrightnessAlternate()
{
  if (nightModeActive()) {
    return false;
  }

  // TODO should this allow going to 0 brightness?  Check previous code.

  // slower steps when brightness < 16%
  uint8_t delta = (bri < 40) ? 2 : 5;
  return decBrightnessBy(delta);
}

bool UiAction::decBrightnessBy(uint8_t delta)
{
  if (nightModeActive()) {
    return false;
  }

  if (bri == 1) {
    return false;
  }
  if (bri == 0) {
    strip.restartRuntime();
    bri = 1;
  } else if (bri <= delta) {
    bri = 1;
  } else {
    bri -= delta;
  }

  doUpdate();
  return true;
}

void UiAction::setBrightness(uint8_t brightness)
{
  if (brightness == 0) {
    turnOff();
  } else {
    brightnessBeforeNightMode = NIGHT_MODE_DEACTIVATED;
    if (bri == 0) {
      strip.restartRuntime();
    }
    bri = brightness;
    doUpdate();
  }
}

void UiAction::nightlightStart()
{
  // TODO probably should call resetNightMode() here.
  nightlightActive = true;
  nightlightStartTime = millis();
  doUpdate();
}

void UiAction::preset(uint8_t presetID)
{
  resetNightMode();
  applyPreset(presetID, CALL_MODE_BUTTON_PRESET);
  // TODO not certain why previous code didn't call stateUpdated() in this case, but it didn't.
  // I *think* that maybe applyPreset() calls stateUpdated(CALL_MODE_BUTTON_PRESET)?
}

void UiAction::presetWithFallback(uint8_t presetID, uint8_t effectID, uint8_t paletteID)
{
  resetNightMode();
  applyPresetWithFallback(presetID, CALL_MODE_BUTTON_PRESET, effectID, paletteID);
  // TODO not certain why previous code didn't call stateUpdated() in this case, but it didn't.
  // I *think* that maybe applyPresetWithFallback() calls stateUpdated(CALL_MODE_BUTTON_PRESET)?
}

static uint8_t relativeChange(uint8_t property, int8_t amount, uint8_t lowerBoundary = 0, uint8_t higherBoundary = 0xFF)
{
  int16_t new_val = (int16_t) property + amount;
  if (lowerBoundary >= higherBoundary) return property;
  if (new_val > (int16_t)higherBoundary) new_val = higherBoundary;
  if (new_val < (int16_t)lowerBoundary)  new_val = lowerBoundary;
  return (uint8_t)new_val;
}

uint32_t UiAction::getNextColorInCycle()
{
  colorCycleIndex++;
  if(colorCycleIndex >= sizeof(colorCycle) / sizeof(colorCycle[0])) {
    colorCycleIndex = 0;
  }

  return colorCycle[colorCycleIndex];
}

void UiAction::nextColorAndPalette()
{
  uint32_t new_color = getNextColorInCycle();

  // TODO wouldn't it be better to wrap pal instead of clamping it?
  uint8_t pal = relativeChange(effectPalette, 1, 0, getPaletteCount() - 1);

  changeColorEffectAndPalette(new_color, -1, -1, pal);
}

void UiAction::setColorRandom()
{
  uint8_t new_color_arr[4];
  setRandomColor(new_color_arr);
  uint32_t new_color = RGBW32(new_color_arr[0], new_color_arr[1], new_color_arr[2], colPri[3]);
  changeColor(new_color);
}

void UiAction::changePalette(uint8_t pal)
{
  SegmentIterator segment_iter(m_filter);
  do {
    segment_iter->setPalette(pal);
  } while (segment_iter.next());
  setValuesFromSegmentRef(segment_iter.first());
  stateChanged = true;
  doUpdate();
}

void UiAction::nextPalette()
{
  uint8_t pal = (effectPalette + 1) % getPaletteCount();
  changePalette(pal);
}

void UiAction::prevPalette()
{
  uint8_t pal = (effectPalette == 0 ? getPaletteCount() - 1 : effectPalette - 1);
  changePalette(pal);
}

void UiAction::changeEffect(uint8_t fx)
{
  SegmentIterator segment_iter(m_filter);
  do {
    segment_iter->setMode(fx);
  } while (segment_iter.next());
  setValuesFromSegmentRef(segment_iter.first());
  stateChanged = true;
  doUpdate();
}

void UiAction::nextEffect()
{
  uint8_t fx = (effectCurrent + 1) % strip.getModeCount();
  changeEffect(fx);
}

void UiAction::prevEffect()
{
  uint8_t fx = (effectCurrent == 0 ? strip.getModeCount() - 1 : effectCurrent - 1);
  changeEffect(fx);
}

void UiAction::changeEffectSpeed(uint8_t speed)
{
  SegmentIterator segment_iter(m_filter);
  do {
    segment_iter->speed = speed;
  } while (segment_iter.next());
  setValuesFromSegmentRef(segment_iter.first());
  stateChanged = true;
  doUpdate();
}

void UiAction::changeEffectSpeedRelative(int8_t delta)
{
  int16_t new_val = (int16_t) effectSpeed + delta;
  uint8_t speed = (uint8_t)constrain(new_val,0,255);

  changeEffectSpeed(speed);
}

void UiAction::incEffectSpeed()
{
  uint8_t speed = effectSpeed;
  if (speed < 240) {
      speed += 12;
  } else if (speed < 255) {
      speed += 1;
  } else {
    return;
  }
  changeEffectSpeed(speed);
}

void UiAction::decEffectSpeed()
{
  uint8_t speed = effectSpeed;
  if (speed > 15) {
      speed -= 12;
  } else if (speed > 0) {
      speed -= 1;
  } else {
    return;
  }
  changeEffectSpeed(speed);
}

void UiAction::changeEffectIntensity(uint8_t intensity)
{
  SegmentIterator segment_iter(m_filter);
  do {
    segment_iter->intensity = intensity;
  } while (segment_iter.next());
  setValuesFromSegmentRef(segment_iter.first());
  stateChanged = true;
  doUpdate();
}

void UiAction::changeEffectIntensityRelative(int8_t delta)
{
  int16_t new_val = (int16_t) effectIntensity + delta;
  uint8_t intensity = (uint8_t)constrain(new_val,0,255);
  
  changeEffectIntensity(intensity);
}

void UiAction::incEffectIntensity()
{
  uint8_t intensity = effectIntensity;
  if (intensity < 240) {
      intensity += 12;
  } else if (intensity < 255) {
      intensity += 1;
  } else {
    return;
  }
  changeEffectIntensity(intensity);
}

void UiAction::decEffectIntensity()
{
  uint8_t intensity = effectIntensity;
  if (intensity > 15) {
      intensity -= 12;
  } else if (intensity > 0) {
      intensity -= 1;
  } else {
    return;
  }
  changeEffectIntensity(intensity);
}

// Changing hue & saturation are almost the same code, so we
// merge them to reduce code duplication.
void UiAction::changeHueSaturationRelative(int8_t hue_delta, int8_t sat_delta)
{
  // Get old color and split into HSV
  SegmentIterator segment_iter(m_filter);
  CRGB fastled_col = CRGB(segment_iter->colors[0]);
  CHSV prim_hsv = rgb2hsv(fastled_col);

  // Change hue
  int16_t new_hue = (int16_t)prim_hsv.h + hue_delta;
  if (new_hue > 255) new_hue -= 255;  // roll-over if  bigger than 255
  if (new_hue < 0) new_hue += 255;    // roll-over if smaller than 0
  prim_hsv.h = (uint8_t)new_hue;

  // Change saturation
  int16_t new_sat = (int16_t)prim_hsv.s + sat_delta;
  prim_hsv.s = (uint8_t)constrain(new_sat,0,255);  // constrain to 0-255

  // Convert color back to RGBW (we don't change the W part)
  hsv2rgb_rainbow(prim_hsv, fastled_col);
  uint32_t new_color = RGBW32(fastled_col.red, fastled_col.green, fastled_col.blue, W(segment_iter->colors[0]));

  // Set the new color.
  do {
    segment_iter->colors[0] = new_color;
  } while (segment_iter.next());
  setValuesFromSegmentRef(segment_iter.first());
  stateChanged = true;
  doUpdate();
}

void UiAction::changeHueRelative(int8_t delta)
{
  changeHueSaturationRelative(delta, 0);
}

void UiAction::changeSaturationRelative(int8_t delta)
{
  changeHueSaturationRelative(0, delta);
}

void UiAction::incEffectSpeedOrHue()
{
  if (effectCurrent != 0) {
    changeEffectSpeedRelative(16);
  } else { // if Effect == "solid Color", change the hue of the primary color
    changeHueRelative(16);
  }
}

void UiAction::decEffectSpeedOrHue()
{
  if (effectCurrent != 0) {
    changeEffectSpeedRelative(-16);
  } else { // if Effect == "solid Color", change the hue of the primary color
    changeHueRelative(-16);
  }
}

void UiAction::incEffectIntensityOrSaturation()
{
  if (effectCurrent != 0) {
    changeEffectIntensityRelative(16);
  } else { // if Effect == "solid Color", change the saturation of the primary color
    changeSaturationRelative(16);
  }
}

void UiAction::decEffectIntensityOrSaturation()
{
  if (effectCurrent != 0) {
    changeEffectIntensityRelative(-16);
  } else { // if Effect == "solid Color", change the saturation of the primary color
    changeSaturationRelative(-16);
  }
}

void UiAction::changeColorEffectAndPalette(uint32_t c, int16_t cct, int16_t effect, int16_t pal)
{
  SegmentIterator segment_iter(m_filter);
  do {
    uint8_t capabilities = segment_iter->getLightCapabilities();
    uint32_t mask = 0;
    bool isRGB   = GET_BIT(capabilities, 0);  // is segment RGB capable
    bool hasW    = GET_BIT(capabilities, 1);  // do we have white/CCT channel
    bool isCCT   = GET_BIT(capabilities, 2);  // is segment CCT capable
    bool wSlider = GET_BIT(capabilities, 3);  // is white auto calculated (white slider NOT shown in UI)
    if (isRGB) mask |= 0x00FFFFFF; // RGB
    if (hasW)  mask |= 0xFF000000; // white
    if (hasW && !wSlider && (c & 0xFF000000)) { // segment has white channel & white channel is auto calculated & white specified
        segment_iter->setColor(0, c | 0xFFFFFF); // for accurate/brighter mode we fake white (since button may not set white color to 0xFFFFFF)
    } else if (c & mask) segment_iter->setColor(0, c & mask); // only apply if not black
    if (isCCT && cct >= 0) segment_iter->setCCT(cct);
    if (effect >= 0) segment_iter->setMode((uint8_t)effect);
    if (pal >= 0) segment_iter->setPalette((uint8_t)pal);
  } while (segment_iter.next());
  setValuesFromSegmentRef(segment_iter.first());
  stateChanged = true;
  doUpdate();
}

void UiAction::changeColor(uint32_t c, int16_t cct)
{
  changeColorEffectAndPalette(c, cct, -1, -1);
}

void UiAction::changeColorStatic(uint32_t c, int16_t cct)
{
  changeColorEffectAndPalette(c, cct, FX_MODE_STATIC, -1);
}

void UiAction::changeWhite(int8_t amount)
{
  SegmentIterator segment_iter(m_filter);
  Segment& seg = *segment_iter;
  uint8_t r = R(seg.colors[0]);
  uint8_t g = G(seg.colors[0]);
  uint8_t b = B(seg.colors[0]);
  uint8_t w = relativeChange(W(seg.colors[0]), amount, 5);
  uint32_t new_color = RGBW32(r, g, b, w);
  changeColor(new_color);
}

void UiAction::whiteOff()
{
  SegmentIterator segment_iter(m_filter);
  Segment& seg = *segment_iter;
  uint8_t r = R(seg.colors[0]);
  uint8_t g = G(seg.colors[0]);
  uint8_t b = B(seg.colors[0]);
  uint8_t w = W(seg.colors[0]);

  if (w) whiteLast = w;

  uint32_t new_color = RGBW32(r, g, b, 0);

  changeColor(new_color);
}

void UiAction::whiteOn()
{
  SegmentIterator segment_iter(m_filter);
  Segment& seg = *segment_iter;
  uint8_t r = R(seg.colors[0]);
  uint8_t g = G(seg.colors[0]);
  uint8_t b = B(seg.colors[0]);
  uint32_t new_color = RGBW32(r, g, b, whiteLast);

  changeColor(new_color);
}

void UiAction::setWhiteAndChangeCctRelative(uint32_t c, int8_t delta)
{
  uint8_t cct = strip.getMainSegment().cct + delta;
  // No need for range check - turns out setCct() will do that for us.

  changeColorStatic(c, cct); 
}

void UiAction::changeCct(uint16_t cct)
{
  SegmentIterator segment_iter(m_filter);
  do {
    segment_iter->setCCT(cct);
  } while (segment_iter.next());
  setValuesFromSegmentRef(segment_iter.first());
  stateChanged = true;
  doUpdate();
}

void UiAction::setToPlainStaticBrightWhite()
{
  if (bri == 0)
  {
    strip.restartRuntime();
  }
  bri = 255;
  stateChanged = true;
  changeColorEffectAndPalette(COLOR_WHITE, -1, FX_MODE_STATIC, 0);
}

void UiAction::changeCustomRelative(uint8_t param_id, int8_t delta)
{
  SegmentIterator segment_iter(m_filter);
  int16_t new_val_uc;
  uint8_t new_val;

  switch (param_id) {
    default:
    case 1:
      new_val_uc = (int16_t)segment_iter->custom1 + delta;
      new_val = (uint8_t)constrain(new_val_uc, 0, 255);
      break;
    case 2:
      new_val_uc = (int16_t)segment_iter->custom2 + delta;
      new_val = (uint8_t)constrain(new_val_uc, 0, 255);
      break;
    case 3:
      new_val_uc = (int16_t)segment_iter->custom3 + delta;
      new_val = (uint8_t)constrain(new_val_uc, 0, 31);  // Only 5 bits.
      break;
  }

  do {
    switch (param_id) {
      default:
      case 1:
        segment_iter->custom1 = new_val;
        break;
      case 2:
        segment_iter->custom2 = new_val;
        break;
      case 3:
        segment_iter->custom3 = new_val;
        break;
    }
  } while (segment_iter.next());
  stateChanged = true;
  doUpdate();
}


#define ACTIONIRJSON_BUSWAIT_TIMEOUT 24 // one frame timeout to wait for bus to finish updating

UiJsonActionResult UiAction::runJson(uint8_t moduleID, const char fileName[], const char objKey[])
{
  if (!requestJSONBufferLock(moduleID)) {
    return UiJsonActionResult_ERR_LOCK;
  }

  // wait for strip to finish updating, accessing FS during sendout causes glitches
  unsigned long start = millis();
  while (strip.isUpdating() && millis()-start < ACTIONIRJSON_BUSWAIT_TIMEOUT) yield(); 

  // attempt to read command from JSON file.
  // this may fail for two reasons: JSON file does not exist or IR code not found
  readObjectFromFile(fileName, objKey, pDoc);
  JsonObject fdo = pDoc->as<JsonObject>();
  if (fdo.isNull()) {
    // the received button does not exist
    if (!WLED_FS.exists(fileName)) {
      releaseJSONBufferLock();
      return UiJsonActionResult_ERR_NO_FILE;
    }
    releaseJSONBufferLock();
    return UiJsonActionResult_ERR_CODE_NOT_IN_FILE;
  }

  String cmdStr = fdo["cmd"].as<String>();
  JsonObject jsonCmdObj = fdo["cmd"]; //object

  if (!jsonCmdObj.isNull())  // we could also use: !fdo["cmd"].is<String>()
  {
    // command is JSON object
    if (jsonCmdObj[F("psave")].isNull()) {
      if (irApplyToAllSelected && jsonCmdObj["seg"].is<JsonArray>()) {
        JsonObject seg = jsonCmdObj["seg"][0];                    // take 1st segment from array and use it to apply to all selected segments
        seg.remove("id");                                         // remove segment ID if it exists
        jsonCmdObj["seg"] = seg;                                  // replace array with object
      }
      deserializeState(jsonCmdObj, CALL_MODE_BUTTON_PRESET);      // **will call stateUpdated() with correct CALL_MODE**
      releaseJSONBufferLock();
      return UiJsonActionResult_OK;
    } else {
      uint8_t psave = jsonCmdObj[F("psave")].as<int>();
      char pname[33];
      sprintf_P(pname, PSTR("IR Preset %d"), psave);
      fdo.clear();  // TODO should probably be AFTER we use fdo in the call to savePreset()
      if (psave > 0 && psave < 251) {
        savePreset(psave, pname, fdo);
        releaseJSONBufferLock();
        stateUpdated(CALL_MODE_BUTTON_PRESET);
        return UiJsonActionResult_OK;
      } else {
        releaseJSONBufferLock();
        return UiJsonActionResult_ERR_CODE_NO_ACTION;
      }
    }
  } else if (cmdStr.startsWith("!")) {
    // call limited set of C functions
    if (cmdStr.startsWith(F("!incBri"))) {
      releaseJSONBufferLock();
      incBrightness();
      return UiJsonActionResult_OK_REPEATABLE;
    } else if (cmdStr.startsWith(F("!decBri"))) {
      releaseJSONBufferLock();
      decBrightness();
      return UiJsonActionResult_OK_REPEATABLE;
    } else if (cmdStr.startsWith(F("!presetF"))) { //!presetFallback
      uint8_t presetID = fdo["PL"] | 1;
      uint8_t effectID = fdo["FX"] | hw_random8(strip.getModeCount() -1);
      uint8_t paletteID = fdo["FP"] | 0;
      releaseJSONBufferLock();
      presetWithFallback(presetID, effectID, paletteID);
      return UiJsonActionResult_OK;
    } else {
      releaseJSONBufferLock();
      return UiJsonActionResult_ERR_CODE_NO_ACTION;
    }
  } else {
    // HTTP API command
    String apireq = "win"; apireq += '&';                        // reduce flash string usage
    bool repeatable = (cmdStr.indexOf("~") > 0 || fdo["rpt"]);   // repeatable action?
    if (!cmdStr.startsWith(apireq)) cmdStr = apireq + cmdStr;    // if no "win&" prefix
    if (!irApplyToAllSelected && cmdStr.indexOf(F("SS="))<0) {
      char tmp[10];
      sprintf_P(tmp, PSTR("&SS=%d"), strip.getMainSegmentId());
      cmdStr += tmp;
    }
    fdo.clear();                                                 // clear JSON buffer (it is no longer needed)
    handleSet(nullptr, cmdStr, false);                           // no stateUpdated() call here
    releaseJSONBufferLock();
    stateUpdated(CALL_MODE_BUTTON_PRESET);
    return repeatable ? UiJsonActionResult_OK_REPEATABLE : UiJsonActionResult_OK;
  }
}
