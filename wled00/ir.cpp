#include "wled.h"

#ifndef WLED_DISABLE_INFRARED
#include "ir_codes.h"
#include "remote_action.h"

/*
 * Infrared sensor support for several generic RGB remotes and custom JSON remote
 */

static IRrecv* irrecv;
static decode_results results;
static unsigned long irCheckedTime = 0;
static uint32_t lastValidCode = 0;
static byte lastRepeatableAction = ACTION_NONE;
static uint16_t irTimesRepeated = 0;


// increment `bri` to the next `brightnessSteps` value
static void incBrightness()
{
  if (RemoteAction.incBrightness()) {
    lastRepeatableAction = ACTION_BRIGHT_UP;
  }
}

// decrement `bri` to the next `brightnessSteps` value
static void decBrightness()
{
  if (RemoteAction.decBrightness()) {
    lastRepeatableAction = ACTION_BRIGHT_DOWN;
  }
}

static void presetFallback(uint8_t presetID, uint8_t effectID, uint8_t paletteID)
{
  RemoteAction.presetWithFallback(presetID, effectID, paletteID);
  stateUpdated(CALL_MODE_BUTTON);
}

static void incEffectSpeedOrHue()
{
  RemoteAction.incEffectSpeedOrHue();
  lastRepeatableAction = ACTION_SPEED_UP;
}

static void decEffectSpeedOrHue()
{
  RemoteAction.decEffectSpeedOrHue();
  lastRepeatableAction = ACTION_SPEED_DOWN;
}

static void incEffectIntensityOrSaturation()
{
  RemoteAction.incEffectIntensityOrSaturation();
  lastRepeatableAction = ACTION_INTENSITY_UP;
}

static void decEffectIntensityOrSaturation()
{
  RemoteAction.decEffectIntensityOrSaturation();
  lastRepeatableAction = ACTION_INTENSITY_DOWN;
}

static void decodeIR24(uint32_t code)
{
  switch (code) {
    case IR24_BRIGHTER  : incBrightness();                                         break;
    case IR24_DARKER    : decBrightness();                                         break;
    case IR24_OFF       : RemoteAction.turnOff();                                  break;
    case IR24_ON        : RemoteAction.turnOn();                                   break;
    case IR24_RED       : RemoteAction.changeColor(COLOR_RED);                     break;
    case IR24_REDDISH   : RemoteAction.changeColor(COLOR_REDDISH);                 break;
    case IR24_ORANGE    : RemoteAction.changeColor(COLOR_ORANGE);                  break;
    case IR24_YELLOWISH : RemoteAction.changeColor(COLOR_YELLOWISH);               break;
    case IR24_YELLOW    : RemoteAction.changeColor(COLOR_YELLOW);                  break;
    case IR24_GREEN     : RemoteAction.changeColor(COLOR_GREEN);                   break;
    case IR24_GREENISH  : RemoteAction.changeColor(COLOR_GREENISH);                break;
    case IR24_TURQUOISE : RemoteAction.changeColor(COLOR_TURQUOISE);               break;
    case IR24_CYAN      : RemoteAction.changeColor(COLOR_CYAN);                    break;
    case IR24_AQUA      : RemoteAction.changeColor(COLOR_AQUA);                    break;
    case IR24_BLUE      : RemoteAction.changeColor(COLOR_BLUE);                    break;
    case IR24_DEEPBLUE  : RemoteAction.changeColor(COLOR_DEEPBLUE);                break;
    case IR24_PURPLE    : RemoteAction.changeColor(COLOR_PURPLE);                  break;
    case IR24_MAGENTA   : RemoteAction.changeColor(COLOR_MAGENTA);                 break;
    case IR24_PINK      : RemoteAction.changeColor(COLOR_PINK);                    break;
    case IR24_WHITE     : RemoteAction.changeColorStatic(COLOR_WHITE);             break;
    case IR24_FLASH     : presetFallback(1, FX_MODE_COLORTWINKLE, effectPalette);  break;
    case IR24_STROBE    : presetFallback(2, FX_MODE_RAINBOW_CYCLE, effectPalette); break;
    case IR24_FADE      : presetFallback(3, FX_MODE_BREATH, effectPalette);        break;
    case IR24_SMOOTH    : presetFallback(4, FX_MODE_RAINBOW, effectPalette);       break;
    default: return;
  }
  lastValidCode = code;
}

static void decodeIR24OLD(uint32_t code)
{
  switch (code) {
    case IR24_OLD_BRIGHTER  : incBrightness();                                          break;
    case IR24_OLD_DARKER    : decBrightness();                                          break;
    case IR24_OLD_OFF       : RemoteAction.turnOff();                                   break;
    case IR24_OLD_ON        : RemoteAction.turnOn();                                    break;
    case IR24_OLD_RED       : RemoteAction.changeColor(COLOR_RED);                      break;
    case IR24_OLD_REDDISH   : RemoteAction.changeColor(COLOR_REDDISH);                  break;
    case IR24_OLD_ORANGE    : RemoteAction.changeColor(COLOR_ORANGE);                   break;
    case IR24_OLD_YELLOWISH : RemoteAction.changeColor(COLOR_YELLOWISH);                break;
    case IR24_OLD_YELLOW    : RemoteAction.changeColor(COLOR_YELLOW);                   break;
    case IR24_OLD_GREEN     : RemoteAction.changeColor(COLOR_GREEN);                    break;
    case IR24_OLD_GREENISH  : RemoteAction.changeColor(COLOR_GREENISH);                 break;
    case IR24_OLD_TURQUOISE : RemoteAction.changeColor(COLOR_TURQUOISE);                break;
    case IR24_OLD_CYAN      : RemoteAction.changeColor(COLOR_CYAN);                     break;
    case IR24_OLD_AQUA      : RemoteAction.changeColor(COLOR_AQUA);                     break;
    case IR24_OLD_BLUE      : RemoteAction.changeColor(COLOR_BLUE);                     break;
    case IR24_OLD_DEEPBLUE  : RemoteAction.changeColor(COLOR_DEEPBLUE);                 break;
    case IR24_OLD_PURPLE    : RemoteAction.changeColor(COLOR_PURPLE);                   break;
    case IR24_OLD_MAGENTA   : RemoteAction.changeColor(COLOR_MAGENTA);                  break;
    case IR24_OLD_PINK      : RemoteAction.changeColor(COLOR_PINK);                     break;
    case IR24_OLD_WHITE     : RemoteAction.changeColorStatic(COLOR_WHITE);              break;
    case IR24_OLD_FLASH     : presetFallback(1, FX_MODE_COLORTWINKLE, 0);               break;
    case IR24_OLD_STROBE    : presetFallback(2, FX_MODE_RAINBOW_CYCLE, 0);              break;
    case IR24_OLD_FADE      : presetFallback(3, FX_MODE_BREATH, 0);                     break;
    case IR24_OLD_SMOOTH    : presetFallback(4, FX_MODE_RAINBOW, 0);                    break;
    default: return;
  }
  lastValidCode = code;
}

static void decodeIR24CT(uint32_t code)
{
  switch (code) {
    case IR24_CT_BRIGHTER   : incBrightness();                                                 break;
    case IR24_CT_DARKER     : decBrightness();                                                 break;
    case IR24_CT_OFF        : RemoteAction.turnOff();                                          break;
    case IR24_CT_ON         : RemoteAction.turnOn();                                           break;
    case IR24_CT_RED        : RemoteAction.changeColor(COLOR_RED);                             break;
    case IR24_CT_REDDISH    : RemoteAction.changeColor(COLOR_REDDISH);                         break;
    case IR24_CT_ORANGE     : RemoteAction.changeColor(COLOR_ORANGE);                          break;
    case IR24_CT_YELLOWISH  : RemoteAction.changeColor(COLOR_YELLOWISH);                       break;
    case IR24_CT_YELLOW     : RemoteAction.changeColor(COLOR_YELLOW);                          break;
    case IR24_CT_GREEN      : RemoteAction.changeColor(COLOR_GREEN);                           break;
    case IR24_CT_GREENISH   : RemoteAction.changeColor(COLOR_GREENISH);                        break;
    case IR24_CT_TURQUOISE  : RemoteAction.changeColor(COLOR_TURQUOISE);                       break;
    case IR24_CT_CYAN       : RemoteAction.changeColor(COLOR_CYAN);                            break;
    case IR24_CT_AQUA       : RemoteAction.changeColor(COLOR_AQUA);                            break;
    case IR24_CT_BLUE       : RemoteAction.changeColor(COLOR_BLUE);                            break;
    case IR24_CT_DEEPBLUE   : RemoteAction.changeColor(COLOR_DEEPBLUE);                        break;
    case IR24_CT_PURPLE     : RemoteAction.changeColor(COLOR_PURPLE);                          break;
    case IR24_CT_MAGENTA    : RemoteAction.changeColor(COLOR_MAGENTA);                         break;
    case IR24_CT_PINK       : RemoteAction.changeColor(COLOR_PINK);                            break;
    case IR24_CT_COLDWHITE  : RemoteAction.changeColorStatic(COLOR_COLDWHITE2, 255);           break;
    case IR24_CT_WARMWHITE  : RemoteAction.changeColorStatic(COLOR_WARMWHITE2,   0);           break;
    case IR24_CT_CTPLUS     : RemoteAction.setWhiteAndChangeCctRelative(COLOR_COLDWHITE, 1);   break;
    case IR24_CT_CTMINUS    : RemoteAction.setWhiteAndChangeCctRelative(COLOR_WARMWHITE, -1);  break;
    case IR24_CT_MEMORY     : RemoteAction.changeColorStatic(COLOR_NEUTRALWHITE, 127);         break;
    default: return;
  }
  lastValidCode = code;
}

static void decodeIR40(uint32_t code)
{
  switch (code) {
    case IR40_BPLUS        : incBrightness();                                         break;
    case IR40_BMINUS       : decBrightness();                                         break;
    case IR40_OFF          : RemoteAction.turnOff();                                  break;
    case IR40_ON           : RemoteAction.turnOn();                                   break;
    case IR40_RED          : RemoteAction.changeColor(COLOR_RED);                     break;
    case IR40_REDDISH      : RemoteAction.changeColor(COLOR_REDDISH);                 break;
    case IR40_ORANGE       : RemoteAction.changeColor(COLOR_ORANGE);                  break;
    case IR40_YELLOWISH    : RemoteAction.changeColor(COLOR_YELLOWISH);               break;
    case IR40_YELLOW       : RemoteAction.changeColor(COLOR_YELLOW);                  break;
    case IR40_GREEN        : RemoteAction.changeColor(COLOR_GREEN);                   break;
    case IR40_GREENISH     : RemoteAction.changeColor(COLOR_GREENISH);                break;
    case IR40_TURQUOISE    : RemoteAction.changeColor(COLOR_TURQUOISE);               break;
    case IR40_CYAN         : RemoteAction.changeColor(COLOR_CYAN);                    break;
    case IR40_AQUA         : RemoteAction.changeColor(COLOR_AQUA);                    break;
    case IR40_BLUE         : RemoteAction.changeColor(COLOR_BLUE);                    break;
    case IR40_DEEPBLUE     : RemoteAction.changeColor(COLOR_DEEPBLUE);                break;
    case IR40_PURPLE       : RemoteAction.changeColor(COLOR_PURPLE);                  break;
    case IR40_MAGENTA      : RemoteAction.changeColor(COLOR_MAGENTA);                 break;
    case IR40_PINK         : RemoteAction.changeColor(COLOR_PINK);                    break;
    case IR40_WARMWHITE2   : RemoteAction.changeColorStatic(COLOR_WARMWHITE2,     0); break;
    case IR40_WARMWHITE    : RemoteAction.changeColorStatic(COLOR_WARMWHITE,     63); break;
    case IR40_WHITE        : RemoteAction.changeColorStatic(COLOR_NEUTRALWHITE, 127); break;
    case IR40_COLDWHITE    : RemoteAction.changeColorStatic(COLOR_COLDWHITE,    191); break;
    case IR40_COLDWHITE2   : RemoteAction.changeColorStatic(COLOR_COLDWHITE2,   255); break;
    case IR40_WPLUS        : RemoteAction.changeWhite(10);                            break;
    case IR40_WMINUS       : RemoteAction.changeWhite(-10);                           break;
    case IR40_WOFF         : RemoteAction.whiteOff();                                 break;
    case IR40_WON          : RemoteAction.whiteOn();                                  break;
    case IR40_W25          : RemoteAction.setBrightness(63);                          break;
    case IR40_W50          : RemoteAction.setBrightness(127);                         break;
    case IR40_W75          : RemoteAction.setBrightness(191);                         break;
    case IR40_W100         : RemoteAction.setBrightness(255);                         break;
    case IR40_QUICK        : incEffectSpeedOrHue();                                   break;
    case IR40_SLOW         : decEffectSpeedOrHue();                                   break;
    case IR40_JUMP7        : incEffectIntensityOrSaturation();                        break;
    case IR40_AUTO         : decEffectIntensityOrSaturation();                        break;
    case IR40_JUMP3        : presetFallback(1, FX_MODE_STATIC,       0);              break;
    case IR40_FADE3        : presetFallback(2, FX_MODE_BREATH,       0);              break;
    case IR40_FADE7        : presetFallback(3, FX_MODE_FIRE_FLICKER, 0);              break;
    case IR40_FLASH        : presetFallback(4, FX_MODE_RAINBOW,      0);              break;
    default: return;
  }
  lastValidCode = code;
}

static void decodeIR44(uint32_t code)
{
  switch (code) {
    case IR44_BPLUS       : incBrightness();                                          break;
    case IR44_BMINUS      : decBrightness();                                          break;
    case IR44_OFF         : RemoteAction.turnOff();                                   break;
    case IR44_ON          : RemoteAction.turnOn();                                    break;
    case IR44_RED         : RemoteAction.changeColor(COLOR_RED);                      break;
    case IR44_REDDISH     : RemoteAction.changeColor(COLOR_REDDISH);                  break;
    case IR44_ORANGE      : RemoteAction.changeColor(COLOR_ORANGE);                   break;
    case IR44_YELLOWISH   : RemoteAction.changeColor(COLOR_YELLOWISH);                break;
    case IR44_YELLOW      : RemoteAction.changeColor(COLOR_YELLOW);                   break;
    case IR44_GREEN       : RemoteAction.changeColor(COLOR_GREEN);                    break;
    case IR44_GREENISH    : RemoteAction.changeColor(COLOR_GREENISH);                 break;
    case IR44_TURQUOISE   : RemoteAction.changeColor(COLOR_TURQUOISE);                break;
    case IR44_CYAN        : RemoteAction.changeColor(COLOR_CYAN);                     break;
    case IR44_AQUA        : RemoteAction.changeColor(COLOR_AQUA);                     break;
    case IR44_BLUE        : RemoteAction.changeColor(COLOR_BLUE);                     break;
    case IR44_DEEPBLUE    : RemoteAction.changeColor(COLOR_DEEPBLUE);                 break;
    case IR44_PURPLE      : RemoteAction.changeColor(COLOR_PURPLE);                   break;
    case IR44_MAGENTA     : RemoteAction.changeColor(COLOR_MAGENTA);                  break;
    case IR44_PINK        : RemoteAction.changeColor(COLOR_PINK);                     break;
    case IR44_WHITE       : RemoteAction.changeColorStatic(COLOR_NEUTRALWHITE, 127);  break;
    case IR44_WARMWHITE2  : RemoteAction.changeColorStatic(COLOR_WARMWHITE2,     0);  break;
    case IR44_WARMWHITE   : RemoteAction.changeColorStatic(COLOR_WARMWHITE,     63);  break;
    case IR44_COLDWHITE   : RemoteAction.changeColorStatic(COLOR_COLDWHITE,    191);  break;
    case IR44_COLDWHITE2  : RemoteAction.changeColorStatic(COLOR_COLDWHITE2,   255);  break;
    case IR44_REDPLUS     : RemoteAction.nextEffect();                                break;
    case IR44_REDMINUS    : RemoteAction.prevEffect();                                break;
    case IR44_GREENPLUS   : RemoteAction.nextPalette();                               break;
    case IR44_GREENMINUS  : RemoteAction.prevPalette();                               break;
    case IR44_BLUEPLUS    : incEffectIntensityOrSaturation();                         break;
    case IR44_BLUEMINUS   : decEffectIntensityOrSaturation();                         break;
    case IR44_QUICK       : incEffectSpeedOrHue();                                    break;
    case IR44_SLOW        : decEffectSpeedOrHue();                                    break;
    case IR44_DIY1        : presetFallback(1, FX_MODE_STATIC,       0);               break;
    case IR44_DIY2        : presetFallback(2, FX_MODE_BREATH,       0);               break;
    case IR44_DIY3        : presetFallback(3, FX_MODE_FIRE_FLICKER, 0);               break;
    case IR44_DIY4        : presetFallback(4, FX_MODE_RAINBOW,      0);               break;
    case IR44_DIY5        : presetFallback(5, FX_MODE_METEOR,       0);               break;
    case IR44_DIY6        : presetFallback(6, FX_MODE_RAIN,         0);               break;
    case IR44_AUTO        : RemoteAction.changeEffect(FX_MODE_STATIC);                break;
    case IR44_FLASH       : RemoteAction.changeEffect(FX_MODE_PALETTE);               break;
    case IR44_JUMP3       : RemoteAction.setBrightness(63);                           break;
    case IR44_JUMP7       : RemoteAction.setBrightness(127);                          break;
    case IR44_FADE3       : RemoteAction.setBrightness(191);                          break;
    case IR44_FADE7       : RemoteAction.setBrightness(255);                          break;
    default: return;
  }
  lastValidCode = code;
}

static void decodeIR21(uint32_t code)
{
    switch (code) {
      case IR21_BRIGHTER:  incBrightness();                             break;
      case IR21_DARKER:    decBrightness();                             break;
      case IR21_OFF:       RemoteAction.turnOff();                      break;
      case IR21_ON:        RemoteAction.turnOn();                       break;
      case IR21_RED:       RemoteAction.changeColor(COLOR_RED);         break;
      case IR21_REDDISH:   RemoteAction.changeColor(COLOR_REDDISH);     break;
      case IR21_ORANGE:    RemoteAction.changeColor(COLOR_ORANGE);      break;
      case IR21_YELLOWISH: RemoteAction.changeColor(COLOR_YELLOWISH);   break;
      case IR21_GREEN:     RemoteAction.changeColor(COLOR_GREEN);       break;
      case IR21_GREENISH:  RemoteAction.changeColor(COLOR_GREENISH);    break;
      case IR21_TURQUOISE: RemoteAction.changeColor(COLOR_TURQUOISE);   break;
      case IR21_CYAN:      RemoteAction.changeColor(COLOR_CYAN);        break;
      case IR21_BLUE:      RemoteAction.changeColor(COLOR_BLUE);        break;
      case IR21_DEEPBLUE:  RemoteAction.changeColor(COLOR_DEEPBLUE);    break;
      case IR21_PURPLE:    RemoteAction.changeColor(COLOR_PURPLE);      break;
      case IR21_PINK:      RemoteAction.changeColor(COLOR_PINK);        break;
      case IR21_WHITE:     RemoteAction.changeColorStatic(COLOR_WHITE); break;
      case IR21_FLASH:     presetFallback(1, FX_MODE_COLORTWINKLE,  0); break;
      case IR21_STROBE:    presetFallback(2, FX_MODE_RAINBOW_CYCLE, 0); break;
      case IR21_FADE:      presetFallback(3, FX_MODE_BREATH,        0); break;
      case IR21_SMOOTH:    presetFallback(4, FX_MODE_RAINBOW,       0); break;
      default: return;
    }
    lastValidCode = code;
}

static void decodeIR6(uint32_t code)
{
  switch (code) {
    case IR6_POWER:        RemoteAction.turnOnOffToggle();             break;
    case IR6_CHANNEL_UP:   incBrightness();                            break;
    case IR6_CHANNEL_DOWN: decBrightness();                            break;
    case IR6_VOLUME_UP:    RemoteAction.nextEffect();                  break;
    case IR6_VOLUME_DOWN:  RemoteAction.nextColorAndPalette();         break;
    case IR6_MUTE:         RemoteAction.setToPlainStaticBrightWhite(); break;
    default: return;
  }
  lastValidCode = code;
}

static void decodeIR9(uint32_t code)
{
  switch (code) {
    case IR9_POWER      : RemoteAction.turnOnOffToggle();                          break;
    case IR9_A          : presetFallback(1, FX_MODE_COLORTWINKLE, effectPalette);  break;
    case IR9_B          : presetFallback(2, FX_MODE_RAINBOW_CYCLE, effectPalette); break;
    case IR9_C          : presetFallback(3, FX_MODE_BREATH, effectPalette);        break;
    case IR9_UP         : incBrightness();                                         break;
    case IR9_DOWN       : decBrightness();                                         break;
    case IR9_LEFT       : incEffectSpeedOrHue();                                   break;
    case IR9_RIGHT      : decEffectSpeedOrHue();                                   break;
    case IR9_SELECT     : RemoteAction.nextEffect();                               break;
    default: return;
  }
  lastValidCode = code;
}


/*
This allows users to customize IR actions without the need to edit C code and compile.
From the https://github.com/wled/WLED/wiki/Infrared-Control page, download the starter
ir.json file that corresponds to the number of buttons on your remote.
Many of the remotes with the same number of buttons emit the same codes, but will have
different labels or colors. Once you edit the ir.json file, upload it to your controller
using the /edit page.

Each key should be the hex encoded IR code. The "cmd" property should be the HTTP API
or JSON API command to execute on button press. If the command contains a relative change (SI=~16),
it will register as a repeatable command. If the command doesn't contain a "~" but is repeatable, add "rpt" property
set to true. Other properties are ignored but having labels and positions can assist with editing
the json file.

Sample:
{
  "0xFF629D": {"cmd": "T=2", "rpt": true, "label": "Toggle on/off"},  // HTTP command
  "0xFF9867": {"cmd": "A=~16", "label": "Inc brightness"},            // HTTP command with incrementing
  "0xFF38C7": {"cmd": {"bri": 10}, "label": "Dim to 10"},             // JSON command
  "0xFF22DD": {"cmd": "!presetFallback", "PL": 1, "FX": 16, "FP": 6,  // Custom command
               "label": "Preset 1, fallback to Saw - Party if not found"},
}
*/

static void decodeIRJson(uint32_t code)
{
  char objKey[10];
  char fileName[16];

  sprintf_P(objKey, PSTR("\"0x%lX\":"), (unsigned long)code);
  strcpy_P(fileName, PSTR("/ir.json")); // for FS.exists()

  UiJsonActionResult r = RemoteAction.runJson(13, fileName, objKey);

  lastValidCode = 0;

  switch (r)
  {
    case UiJsonActionResult_OK:
      break;
    case UiJsonActionResult_OK_REPEATABLE:
      lastValidCode = code;
      break;
    case UiJsonActionResult_ERR_NO_FILE:
      errorFlag = ERR_FS_IRLOAD; // ir.json file does not exist
      break;
    case UiJsonActionResult_ERR_LOCK:
    case UiJsonActionResult_ERR_CODE_NOT_IN_FILE:
    case UiJsonActionResult_ERR_CODE_NO_ACTION:
      break;
  }
}

static void applyRepeatActions()
{
  if (irEnabled == 8) {
    decodeIRJson(lastValidCode);
    return;
  } else switch (lastRepeatableAction) {
    case ACTION_BRIGHT_UP :      RemoteAction.incBrightness();                  return;
    case ACTION_BRIGHT_DOWN :    RemoteAction.decBrightness();                  return;
    case ACTION_SPEED_UP :       RemoteAction.incEffectSpeedOrHue();            return;
    case ACTION_SPEED_DOWN :     RemoteAction.decEffectSpeedOrHue();            return;
    case ACTION_INTENSITY_UP :   RemoteAction.incEffectIntensityOrSaturation(); return;
    case ACTION_INTENSITY_DOWN : RemoteAction.decEffectIntensityOrSaturation(); return;
    default: break;
  }
  if (lastValidCode == IR40_WPLUS) {
    RemoteAction.changeWhite(10);
  } else if (lastValidCode == IR40_WMINUS) {
    RemoteAction.changeWhite(-10);
  } else if ((lastValidCode == IR24_ON || lastValidCode == IR40_ON) && irTimesRepeated > 7 ) {
    RemoteAction.nightlightStart();
  }
}

static void decodeIR(uint32_t code)
{
  if (code == 0xFFFFFFFF) {
    //repeated code, continue brightness up/down
    irTimesRepeated++;
    applyRepeatActions();
    return;
  }
  lastValidCode = 0;
  irTimesRepeated = 0;
  lastRepeatableAction = ACTION_NONE;

  if (irEnabled == 8) { // any remote configurable with ir.json file
    decodeIRJson(code);
    return;
  }
  if (code > 0xFFFFFF) return; //invalid code

  switch (irEnabled) {
    case 1:
      if (code > 0xF80000) decodeIR24OLD(code); // white 24-key remote (old) - it sends 0xFF0000 values
      else                 decodeIR24(code);    // 24-key remote - 0xF70000 to 0xF80000
      break;
    case 2: decodeIR24CT(code); break; // white 24-key remote with CW, WW, CT+ and CT- keys
    case 3: decodeIR40(code);   break; // blue  40-key remote with 25%, 50%, 75% and 100% keys
    case 4: decodeIR44(code);   break; // white 44-key remote with color-up/down keys and DIY1 to 6 keys
    case 5: decodeIR21(code);   break; // white 21-key remote
    case 6: decodeIR6(code);    break; // black 6-key learning remote defaults: "CH" controls brightness,
                                       // "VOL +" controls effect, "VOL -" controls colour/palette, "MUTE"
                                       // sets bright plain white
    case 7: decodeIR9(code);    break;
    //case 8: return; // ir.json file, handled above switch statement
  }
}

void initIR()
{
  if (irEnabled > 0) {
    irrecv = new IRrecv(irPin);
    if (irrecv) irrecv->enableIRIn();
  } else irrecv = nullptr;
}

void deInitIR()
{
  if (irrecv) {
    irrecv->disableIRIn();
    delete irrecv;
  }
  irrecv = nullptr;
}

void handleIR()
{
  unsigned long currentTime = millis();
  unsigned timeDiff = currentTime - irCheckedTime;
  if (timeDiff > 120 && irEnabled > 0 && irrecv) {
    if (strip.isUpdating() && timeDiff < 240) return;  // be nice, but not too nice
    irCheckedTime = currentTime;
    if (irrecv->decode(&results)) {
      if (results.value != 0 && serialCanTX) { // only print results if anything is received ( != 0 )
        Serial.printf_P(PSTR("IR recv: 0x%lX\n"), (unsigned long)results.value);
      }
      decodeIR(results.value);
      irrecv->resume();
    }
  }
}

#endif
