/*
 *  WLED usermod for keypad and brightness-pot.
 *  3'2020 https://github.com/hobbyquaker
 */

#include <Keypad.h>
#include "remote_action.h"
const byte keypad_rows = 4;
const byte keypad_cols = 4;
char keypad_keys[keypad_rows][keypad_cols] = {
        {'1', '2', '3', 'A'},
        {'4', '5', '6', 'B'},
        {'7', '8', '9', 'C'},
        {'*', '0', '#', 'D'}
};

byte keypad_colPins[keypad_rows] = {D3, D2, D1, D0};
byte keypad_rowPins[keypad_cols] = {D7, D6, D5, D4};

Keypad myKeypad = Keypad(makeKeymap(keypad_keys), keypad_rowPins, keypad_colPins, keypad_rows, keypad_cols);

void userSetup()
{

}

void userConnected()
{

}

long lastTime = 0;
int delayMs = 20; //we want to do something every 2 seconds

void userLoop()
{
    if (millis()-lastTime > delayMs)
    {
        long analog = analogRead(0);
        int new_bri = 1;
        if (analog > 900) {
            new_bri = 255;
        } else if (analog > 30) {
            new_bri = dim8_video(map(analog, 31, 900, 16, 255));
        }
        if (bri != new_bri) {
            RemoteAction.setBrightness(new_bri);
        }

        char myKey = myKeypad.getKey();
        if (myKey != '\0') {
            switch (myKey) {
                case '1': RemoteAction.preset(1);            break;
                case '2': RemoteAction.preset(2);            break;
                case '3': RemoteAction.preset(3);            break;
                case '4': RemoteAction.preset(4);            break;
                case '5': RemoteAction.preset(5);            break;
                case '6': RemoteAction.preset(6);            break;
                case 'A': RemoteAction.preset(7);            break;
                case 'B': RemoteAction.preset(8);            break;
                case '7': RemoteAction.nextEffect();         break;
                case '*': RemoteAction.prevEffect();         break;
                case '8': RemoteAction.incEffectSpeed();     break;
                case '0': RemoteAction.decEffectSpeed();     break;
                case '9': RemoteAction.incEffectIntensity(); break;
                case '#': RemoteAction.decEffectIntensity(); break;
                case 'C': RemoteAction.nextPalette();        break;
                case 'D': RemoteAction.prevPalette();        break;
            }
        }

        lastTime = millis();
    }

}