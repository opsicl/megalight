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
  }
}

void controlshutters() {
  for (byte s = 0; s < conf.nrshutters; s++) {
    controlshutter(s);
  }
}

void controlshutter(byte shutter) {

  if (shutinprogress[shutter]) {
    if (shutcurstate[shutter] == shuttgtstate[shutter] or interrupt[shutter]) {
      if (eottime[shutter] == 0  and (shuttgtstate[shutter] == 100 or shuttgtstate[shutter] == 0)) {
        eottime[shutter] = millis();
      } 
      if (millis() - eottime[shutter] > 3000 or interrupt[shutter]) {

				digitalWrite(conf.shutters[shutter].uppin, LOW);
				digitalWrite(conf.shutters[shutter].downpin, LOW);
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
      byte otherpin;
      bool directionup;
      if (shuttgtstate[shutter] > shutcurstate[shutter]) {
        pin = conf.shutters[shutter].uppin;
        otherpin = conf.shutters[shutter].downpin;
        directionup = true;
      } else {
        pin = conf.shutters[shutter].downpin;
        otherpin = conf.shutters[shutter].uppin;
        directionup = false;
      }

      if (shutterStart[shutter] == 0) {
        shutterStart[shutter] = millis();
        shutinitstate[shutter] = shutcurstate[shutter];
      }

      digitalWrite(pin, HIGH);
      digitalWrite(otherpin, LOW);
      // debugpin = pin;
      int delta = (millis() - shutterStart[shutter]) * 100 / (conf.shutters[shutter].time * 1000);
      if (directionup) {
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
    bool directionup;
    
    if (conf.bmaps[butt].shutterup) {
      pin = conf.shutters[s].uppin;
      tgtstate = 100;
      directionup = true;
    }
    if (conf.bmaps[butt].shutterdown) {
      pin = conf.shutters[s].downpin;
      tgtstate = 0;
      directionup = false;
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
        if (directionup) {
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

