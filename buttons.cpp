#include "Arduino.h"
#include "declarations.h"
#include "buttons.h"
#include "lights.h"
#include "shutters.h"
#include "fans.h"

void handlebuttons() {
  for (byte butt = 0; butt < conf.nrbutt; butt++) {
    handlebutton(butt);
  }
}

void handlebutton(byte butt) {

  int press = digitalRead(conf.bmaps[butt].pin);
  if (press == HIGH) {
    if (lastpress[butt] == false) {
      lastPressTime[butt] = millis();
      lastpress[butt] = true;
    }
  }

  //long press
  if ((press == HIGH) and (millis() - lastPressTime[butt] > 300)) {
    longpressing[butt] = true;
  }

  //end press
  if (press == LOW) {
    if (lastpress[butt]) {
      lastpress[butt] = false;
      duration[butt] = millis() - lastPressTime[butt];
    }
    else {
      duration[butt] = 0;
    }
    shortpress[butt] = false;
    doublepress[butt] = false;
    longpressing[butt] = false;
    endpress[butt] = true;
  }

  //short press
  if ((duration[butt] > 50) and (millis() - lastLPTime[butt] > 300)) {
    shortpress[butt] = true;

    //double short press
    if (millis() - lastShortPress[butt] < 500) {
      Serial.print("Double press on: ");
      Serial.println(butt);
      doublepress[butt] = true;
    }

    lastShortPress[butt] = millis();
    Serial.print("Button pressed: ");
    Serial.println(butt);
  }

  //if any button pressed or released, call the appropriate function
  if (shortpress[butt] or longpressing[butt] or endpress[butt]) {
    if (conf.bmaps[butt].light) {
      lightsbutton(butt);
    }
    else if (conf.bmaps[butt].shutterup or conf.bmaps[butt].shutterdown) {
      shuttersbutton(butt);
    }
    else if (conf.bmaps[butt].fan) {
      fansbutton(butt);
    }
    if (endpress[butt]) {
      endpress[butt] = false;
    }
  }

}
