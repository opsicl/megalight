#include "Arduino.h"
#include "declarations.h"
#include "buttons.h"
#include "lights.h"
#include "shutters.h"
#include "fans.h"



void readpcf(byte c) {
  int res = pcf[c].read8();
  //publish_metric("log", "pcf"+String(c), String(res));
  for (byte p = 0; p < 8; p++) {
    byte idx = c * 8 + p;
    //publish_metric("log", "btnval"+String(p), String(res >> p & 1));
    if ((res >> p & 1) == 0) {
      //publish_metric("log", "btn", String(idx));
      btn[idx] = true;
    } else {
      btn[idx] = false;
    }
  }
}

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
  int press = btn[conf.bmaps[butt].pin];
  if (press) {
    if (lastpress[butt] == false) {
      lastPressTime[butt] = millis();
      lastpress[butt] = true;
    }
  }

  //long press
  if (press  and (millis() - lastPressTime[butt] > 300)) {
    if (not longpressing[butt]) {
      publish_metric("button", String(conf.bmaps[butt].pin)+"/longpress", "1");
    }
    longpressing[butt] = true;
  }

  //end press
  if (not press) {
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
      publish_metric("button", String(conf.bmaps[butt].pin)+"/longpress", "0");
    }
    longpressing[butt] = false;
    
    endpress[butt] = true;
  }

  //short press
  if ((duration[butt] > 10) and (millis() - lastLPTime[butt] > 500)) {
    shortpress[butt] = true;

    //double short press
    if (millis() - lastShortPress[butt] < 300) {
      //Serial.print("Double press on: ");
      //Serial.println(butt);
      doublepress[butt] = true;
      publish_metric("button", String(conf.bmaps[butt].pin)+"/doublepress", "1");
    } else {
      publish_metric("button", String(conf.bmaps[butt].pin)+"/shortpress", "1");
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
