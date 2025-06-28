#include "wled.h"
#ifndef WLED_DISABLE_ESPNOW

#include "remote_action.h"

#define WIZMOTE_BUTTON_ON          1
#define WIZMOTE_BUTTON_OFF         2
#define WIZMOTE_BUTTON_NIGHT       3
#define WIZMOTE_BUTTON_ONE         16
#define WIZMOTE_BUTTON_TWO         17
#define WIZMOTE_BUTTON_THREE       18
#define WIZMOTE_BUTTON_FOUR        19
#define WIZMOTE_BUTTON_BRIGHT_UP   9
#define WIZMOTE_BUTTON_BRIGHT_DOWN 8

#define WIZ_SMART_BUTTON_ON          100
#define WIZ_SMART_BUTTON_OFF         101
#define WIZ_SMART_BUTTON_BRIGHT_UP   102
#define WIZ_SMART_BUTTON_BRIGHT_DOWN 103

// This is kind of an esoteric strucure because it's pulled from the "Wizmote"
// product spec. That remote is used as the baseline for behavior and availability
// since it's broadly commercially available and works out of the box as a drop-in
typedef struct WizMoteMessageStructure {
  uint8_t program;  // 0x91 for ON button, 0x81 for all others
  uint8_t seq[4];   // Incremetal sequence number 32 bit unsigned integer LSB first
  uint8_t dt1;      // Button Data Type (0x32)
  uint8_t button;   // Identifies which button is being pressed
  uint8_t dt2;      // Battery Level Data Type (0x01)
  uint8_t batLevel; // Battery Level 0-100
  
  uint8_t byte10;   // Unknown, maybe checksum
  uint8_t byte11;   // Unknown, maybe checksum
  uint8_t byte12;   // Unknown, maybe checksum
  uint8_t byte13;   // Unknown, maybe checksum
} message_structure_t;

static uint32_t last_seq = UINT32_MAX;
static int16_t ESPNowButton = -1; // set in callback if new button value is received

static bool remoteJson(int button)
{
  char objKey[10];
  char fileName[16];

  sprintf_P(objKey, PSTR("\"%d\":"), button);
  strcpy_P(fileName, PSTR("/remote.json"));

  UiJsonActionResult r = RemoteAction.runJson(22, fileName, objKey);

  return (r == UiJsonActionResult_OK || r == UiJsonActionResult_OK_REPEATABLE);
}

// Callback function that will be executed when data is received from a linked remote
void handleWiZdata(uint8_t *incomingData, size_t len) {
  message_structure_t *incoming = reinterpret_cast<message_structure_t *>(incomingData);

  if (len != sizeof(message_structure_t)) {
    DEBUG_PRINTF_P(PSTR("Unknown incoming ESP Now message received of length %u\n"), len);
    return;
  }

  uint32_t cur_seq = incoming->seq[0] | (incoming->seq[1] << 8) | (incoming->seq[2] << 16) | (incoming->seq[3] << 24);
  if (cur_seq == last_seq) {
    return;
  }

  DEBUG_PRINT(F("Incoming ESP Now Packet ["));
  DEBUG_PRINT(cur_seq);
  DEBUG_PRINT(F("] from sender ["));
  DEBUG_PRINT(last_signal_src);
  DEBUG_PRINT(F("] button: "));
  DEBUG_PRINTLN(incoming->button);

  ESPNowButton = incoming->button; // save state, do not process in callback (can cause glitches)
  last_seq = cur_seq;
}

// process ESPNow button data (acesses FS, should not be called while update to avoid glitches)
void handleRemote() {
  if(ESPNowButton >= 0) {
  if (!remoteJson(ESPNowButton))
    switch (ESPNowButton) {
      case WIZ_SMART_BUTTON_ON         :
      case WIZMOTE_BUTTON_ON           : RemoteAction.turnOn();                                        break;
      case WIZ_SMART_BUTTON_OFF        :
      case WIZMOTE_BUTTON_OFF          : RemoteAction.turnOff();                                       break;
      case WIZMOTE_BUTTON_ONE          : RemoteAction.presetWithFallback(1, FX_MODE_STATIC,        0); break;
      case WIZMOTE_BUTTON_TWO          : RemoteAction.presetWithFallback(2, FX_MODE_BREATH,        0); break;
      case WIZMOTE_BUTTON_THREE        : RemoteAction.presetWithFallback(3, FX_MODE_FIRE_FLICKER,  0); break;
      case WIZMOTE_BUTTON_FOUR         : RemoteAction.presetWithFallback(4, FX_MODE_RAINBOW,       0); break;
      case WIZMOTE_BUTTON_NIGHT        : RemoteAction.activateNightMode();                             break;
      case WIZ_SMART_BUTTON_BRIGHT_UP  :
      case WIZMOTE_BUTTON_BRIGHT_UP    : RemoteAction.incBrightness();                                 break;
      case WIZ_SMART_BUTTON_BRIGHT_DOWN:
      case WIZMOTE_BUTTON_BRIGHT_DOWN  : RemoteAction.decBrightness();                                 break;
      default: break;
    }
  }
  ESPNowButton = -1;
}

#else
void handleRemote() {}
#endif
