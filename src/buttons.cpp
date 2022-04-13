#include "Arduino.h"
#include "declarations.h"
#include "buttons.h"
#include "lights.h"
#include "shutters.h"
#include "fans.h"

void handlebuttons() {
  //Serial.print("buttons ");
  //Serial.println(conf.nrbutt);
  for (byte butt = 0; butt < conf.nrbutt; butt++) {
    handlebutton(butt);
  }
}

void handlebutton(byte butt) {

  //Serial.print("here");
  //Serial.println(butt);
  int press = digitalRead(conf.bmaps[butt].pin);
  if (press == HIGH) {
    if (lastpress[butt] == false) {
      lastPressTime[butt] = millis();
      lastpress[butt] = true;
    }
  }

  //long press
  if ((press == HIGH) and (millis() - lastPressTime[butt] > 300)) {
    if (not longpressing[butt]) {
      publish_metric("button", String(butt)+"/longpress", "1");
    }
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
    if (longpressing[butt]) {
      publish_metric("button", String(butt)+"/longpress", "0");
    }
    longpressing[butt] = false;
    
    endpress[butt] = true;
  }

  //short press
  if ((duration[butt] > 20) and (millis() - lastLPTime[butt] > 300)) {
    shortpress[butt] = true;
    publish_metric("button", String(butt)+"/shortpress", "1");

    //double short press
    if (millis() - lastShortPress[butt] < 500) {
      //Serial.print("Double press on: ");
      //Serial.println(butt);
      doublepress[butt] = true;
      publish_metric("button", String(butt)+"/doublepress", "1");
    }

    lastShortPress[butt] = millis();
    //Serial.print("Button pressed: ");
    //Serial.println(butt);
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
