#include "remote_action.h"

//Use userVar0 and userVar1 (API calls &U0=,&U1=, uint16_t)

static long lastTime = 0;
static int delayMs = 10;
static const int pinA = D6; //data
static const int pinB = D7; //clk
static int oldA = LOW;

//gets called once at boot. Do all initialization that doesn't depend on network here
void userSetup() {
  pinMode(pinA, INPUT_PULLUP);
  pinMode(pinB, INPUT_PULLUP);
}

//gets called every time WiFi is (re-)connected. Initialize own network interfaces here
void userConnected() {
}

//loop. You can use "if (WLED_CONNECTED)" to check for successful connection
void userLoop() {
  if (millis()-lastTime > delayMs) {
    int A = digitalRead(pinA);
    int B = digitalRead(pinB);

    if (oldA == LOW && A == HIGH) {
      if (oldB == HIGH) {
      // TODO syntax error above - unmatched {

      RemoteAction.nextEffect();
    }
    else {
      RemoteAction.prevEffect();
    }
    oldA = A;

    lastTime = millis();
  }
}
