#include "Arduino.h"
#include "declarations.h"
#include "buttons.h"
#include "shutters.h"

void setshutter(String shutterattr, byte* payload, unsigned int length) {

  byte attrindex;
  int shutter;
  //Serial.println(shutterattr);
  if (shutterattr[1] == "/") {
    shutter = shutterattr[0] - '0';
    attrindex = 1;
  } else {
    shutter = shutterattr.substring(0,1).toInt();
    attrindex = 2;
  }

  String attr = shutterattr.substring(attrindex,(attrindex + 4));
  //Serial.println(attr);

  if (attr == "open") {
    payload[length] = '\0';
    int value = atoi((char *)payload);
    shuttgtstate[shutter] = value;
    shutinprogress[shutter] = true;
    if (shuttgtstate[shutter] >= shutcurstate[shutter]) {
      directionup[shutter] = true;
    } else {
      directionup[shutter] = false;
    }
  }
}

void controlshutters() {
  for (byte s = 0; s < conf.nrshutters; s++) {
    controlshutter(s);
  }
}

void controlshutter(byte shutter) {

  byte ctrlindexup = conf.shutters[shutter].uppin / 16;
  byte uppin = conf.shutters[shutter].uppin - 16*ctrlindexup;
  byte ctrlindexdn = conf.shutters[shutter].downpin / 16;
  byte downpin = conf.shutters[shutter].downpin - 16*ctrlindexdn;
  
  if (shutinprogress[shutter]) {
    if ((directionup[shutter] and shutcurstate[shutter] >= shuttgtstate[shutter]) or (directionup[shutter] == false and shutcurstate[shutter] <= shuttgtstate[shutter]) or interrupt[shutter]) {
      if (eottime[shutter] == 0  and (shuttgtstate[shutter] == 100 or shuttgtstate[shutter] == 0)) {
        eottime[shutter] = millis();
      } 
      if (millis() - eottime[shutter] > 3000 or interrupt[shutter]) {

        //digitalWrite(conf.shutters[shutter].uppin, LOW);
        onoff[ctrlindexup].setPWM(uppin, 0, 4096);

        //digitalWrite(conf.shutters[shutter].downpin, LOW);
        onoff[ctrlindexdn].setPWM(downpin, 0, 4096);
        shutterStart[shutter] = 0;
        if (interrupt[shutter]) {
          interrupt[shutter] = false;
        } else {
          shutinprogress[shutter] = false;
        }
        eottime[shutter] = 0;
      }
    } else {
      byte pin;
      byte ctrlindexp;
      byte otherpin;
      byte ctrlindexo;
      if (shuttgtstate[shutter] > shutcurstate[shutter]) {
        pin = uppin;
        ctrlindexp = ctrlindexup;
        otherpin = downpin;
        ctrlindexo = ctrlindexdn;
        directionup[shutter] = true;
      } else {
        pin = downpin;
        ctrlindexp = ctrlindexdn;
        otherpin = uppin;
        ctrlindexo = ctrlindexup;
        directionup[shutter] = false;
      }

      if (shutterStart[shutter] == 0) {
        shutterStart[shutter] = millis();
        shutinitstate[shutter] = shutcurstate[shutter];
      }

      //digitalWrite(otherpin, LOW);
      onoff[ctrlindexo].setPWM(otherpin, 0, 4096);
      //digitalWrite(pin, HIGH);
      onoff[ctrlindexp].setPWM(pin, 4096, 0);
      // debugpin = pin;
      int delta = (millis() - shutterStart[shutter]) * 100 / (conf.shutters[shutter].time * 1000);
      if (directionup[shutter]) {
        shutcurstate[shutter] = shutinitstate[shutter] + delta;
      } else {
        shutcurstate[shutter] = shutinitstate[shutter] - delta;
      }

      //wtf should never happen
      if (shutcurstate[shutter] > 100) {
        shutcurstate[shutter] = 100;
      }
      if (shutcurstate[shutter] < 0) {
        shutcurstate[shutter] = 0;
      }

    }
  } else {
    return;
  }
}

void shuttersbutton(byte butt) {

  for (byte sb = 0; sb < conf.bmaps[butt].nrdev; sb++) {
    byte s = conf.bmaps[butt].devices[sb];
    byte pin;
    byte tgtstate;
    
    if (conf.bmaps[butt].shutterup) {
      pin = conf.shutters[s].uppin;
      tgtstate = 100;
      directionup[s] = true;
    }
    if (conf.bmaps[butt].shutterdown) {
      pin = conf.shutters[s].downpin;
      tgtstate = 0;
      directionup[s] = false;
    }
    if (shortpress[butt]) {
      if (shutinprogress[s]) {
        interrupt[s] = true;
        shuttgtstate[s] = shutcurstate[s];
      } else {
        shutinprogress[s] = true;
        shuttgtstate[s] = tgtstate;
      }
      //Serial.print("Shutters button pressed: ");
      //Serial.println(butt);
    }
    if (longpressing[butt]) {
      if (millis() - lastLPTime[butt] > 30 and shutinprogress[s]) {
        interrupt[s] = true;
      } else {
        shutinprogress[s] = true;
        if (directionup[s]) {
          if (shutcurstate[s] < 100) {
            shuttgtstate[s] = shutcurstate[s] + 1;
          }
        } else {
          if (shutcurstate[s] > 0) {
            shuttgtstate[s] = shutcurstate[s] - 1;
          }
        } 

        if (shutcurstate[s] == 100) {
          shutcurstate[s] = 99;
        }
        if (shutcurstate[s] == 0) {
          shutcurstate[s] = 1;
        }
      }
      lastLPTime[butt] = millis();
    }
  }

}

