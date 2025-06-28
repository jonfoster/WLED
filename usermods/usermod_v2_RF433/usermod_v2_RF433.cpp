#include "wled.h"
#include "remote_action.h"
#include "Arduino.h"
#include <RCSwitch.h>


class RF433Usermod : public Usermod
{
private:
  RCSwitch mySwitch = RCSwitch();
  unsigned long lastCommand = 0;
  unsigned long lastTime = 0;

  bool modEnabled = true;
  int8_t receivePin = -1;

  static const char _modName[];
  static const char _modEnabled[];
  static const char _receivePin[];

  bool initDone = false;

public:

  void setup()
  {
    mySwitch.disableReceive();
    if (modEnabled)
    {
      mySwitch.enableReceive(receivePin);
    }
    initDone = true;
  }

  /*
   * connected() is called every time the WiFi is (re)connected
   * Use it to initialize network interfaces
   */
  void connected()
  {
  }

  void loop()
  {
    if (!modEnabled || strip.isUpdating()) 
      return;

    if (mySwitch.available())
    {
      unsigned long receivedCommand = mySwitch.getReceivedValue();
      mySwitch.resetAvailable();

      // Discard duplicates, limit long press repeat
      if (lastCommand == receivedCommand && millis() - lastTime < 800)
        return;

      lastCommand = receivedCommand;
      lastTime = millis();

      DEBUG_PRINT(F("RF433 Receive: "));
      DEBUG_PRINTLN(receivedCommand);
      
      if(!remoteJson433(receivedCommand))
        DEBUG_PRINTLN(F("RF433: unknown button"));
    }
  }

  // Add last received button to info pane
  void addToJsonInfo(JsonObject &root)
  {
    if (!initDone)
      return; // prevent crash on boot applyPreset()
    JsonObject user = root["u"];
    if (user.isNull())
      user = root.createNestedObject("u");

    JsonArray switchArr = user.createNestedArray("RF433 Last Received"); // name
    switchArr.add(lastCommand);
  }

  void addToConfig(JsonObject &root)
  {
    JsonObject top = root.createNestedObject(FPSTR(_modName)); // usermodname
    top[FPSTR(_modEnabled)] = modEnabled;
    JsonArray pinArray = top.createNestedArray("pin");
    pinArray.add(receivePin);

    DEBUG_PRINTLN(F(" config saved."));
  }

  bool readFromConfig(JsonObject &root)
  {
    JsonObject top = root[FPSTR(_modName)];
    if (top.isNull())
    {
      DEBUG_PRINT(FPSTR(_modName));
      DEBUG_PRINTLN(F(": No config found. (Using defaults.)"));
      return false;
    }
    getJsonValue(top[FPSTR(_modEnabled)], modEnabled);
    getJsonValue(top["pin"][0], receivePin);

    DEBUG_PRINTLN(F("config (re)loaded."));

    // Redo init on update
    if(initDone)
      setup();

    return true;
  }

  /*
   * getId() allows you to optionally give your V2 usermod an unique ID (please define it in const.h!).
   * This could be used in the future for the system to determine whether your usermod is installed.
   */
  uint16_t getId()
  {
    return USERMOD_ID_RF433;
  }

  bool remoteJson433(int button)
  {
    char objKey[14];
    char fileName[20];

    sprintf_P(objKey, PSTR("\"%d\":"), button);
    strcpy_P(fileName, PSTR("/remote433.json"));

    UiJsonActionResult r = RemoteAction.runJson(22, fileName, objKey);

    switch (r)
    {
      case UiJsonActionResult_OK:
      case UiJsonActionResult_OK_REPEATABLE:
        return true;
      case UiJsonActionResult_ERR_NO_FILE:
      case UiJsonActionResult_ERR_LOCK:
      case UiJsonActionResult_ERR_CODE_NOT_IN_FILE:
      case UiJsonActionResult_ERR_CODE_NO_ACTION:
        return false;
    }
  }
};

const char RF433Usermod::_modName[]          PROGMEM = "RF433 Remote";
const char RF433Usermod::_modEnabled[]       PROGMEM = "Enabled";
const char RF433Usermod::_receivePin[]       PROGMEM = "RX Pin";

static RF433Usermod usermod_v2_RF433;
REGISTER_USERMOD(usermod_v2_RF433);
