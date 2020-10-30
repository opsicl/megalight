#include "Arduino.h"
#include "declarations.h"
#include "buttons.h"
#include "lights.h"
#include "fans.h"

void setlight(String lightattr, byte* payload, unsigned int length) {

  byte attrindex;
  int light;
  if (lightattr[1] == "/") {
    light = lightattr[0] - '0';
    attrindex = 1;
  } else {
    light = lightattr.substring(0,1).toInt();
    attrindex = 2;
  }
  //Serial.println(light);
  String attr = lightattr.substring(attrindex,(attrindex + 10));
  //Serial.println(attr);

  if (attr == "brightness") {
    //Serial.print(light);
    //Serial.print(F(" -> "));
    //Serial.println(value);
    in[light] = pltoint(payload,length);
  }
  //li[light] = jlight["brightness"];
  //ct[light] = jlight["color_temp"];
}

void applyintensities() {
  for (byte l = 0; l < conf.nrlights; l++) {
    if (conf.lights[l].tempadj) {
      //float cin = 2*in[l]/(2*ct[l]+1);
      //float win = 2*ct[l]*cin;
      float win = 2 * in[l] / ((2 * ct[l]) + 1);
      int cin = 2*ct[l];
      if (win > 255) {
        win = 255;
        cin = round(2*ct[l]/win);
      }
      if (cin > 255) {
        cin = 255;
        win = round(2*ct[l]*cin);
      }

      //if (millis() - lastPrint > 1000) {
      //  Serial.print("ct");
      //  Serial.println(ct[l]);
      //  Serial.print("in");
      //  Serial.println(in[l]);
      //  Serial.print("cin");
      //  Serial.println(cin);
      //  Serial.print("win");
      //  Serial.println(win);
      //  lastPrint = millis();
      //}
      analogWrite(conf.lights[l].cpin,cin);
      analogWrite(conf.lights[l].wpin,win);

    }
    else {
      if (millis() - lastIntSet[l] > 1) {
        if (in[l] > si[l]) {
          si[l] += 1;
          lastIntSet[l] = millis();
        }
        if (in[l] < si[l]) {
          si[l] -= 1;
          lastIntSet[l] = millis();
        }
      }

      analogWrite(conf.lights[l].cpin,si[l]);
    }
  }

}


void lightsbutton(byte butt) {
  //Serial.println("lightsbutton function called");

  //single press toggle
  if (shortpress[butt]) {
    bool ison = false;
    for (byte lindex = 0; lindex < conf.bmaps[butt].nrdev; lindex++) {
      byte l = conf.bmaps[butt].devices[lindex];
      if (in[l] > 0) {
        ison = true;
        break;
      }
    }
    for (byte lindex = 0; lindex < conf.bmaps[butt].nrdev; lindex++) {
      byte l = conf.bmaps[butt].devices[lindex];
      if (ison) {
        in[l] = 0;
      }
      else {
        if (li[l] == 0) {
          in[l] = 255;
          li[l] = 255;
        }
        else {
          in[l] = li[l];
        }
      }
    }
  }

  //longpress dimming   
  if (longpressing[butt] and (millis() - lastLPTime[butt] > 40)) {
    for (byte lindex = 0; lindex < conf.bmaps[butt].nrdev; lindex++) {
      byte l = conf.bmaps[butt].devices[lindex];
      if (dimming[l] == false) {
        if (in[l] == 1) {
          dimdir[l] = 1;
        } else {
          dimdir[l] = -1;
        }
      }

      if (in[l] > 0) {
        if (not (dimming[l] and (in[l] == 1 or in[l] >= 255))) {
          if (in[l] > 30) {
            in[l] = in[l] + 3 * dimdir[l];
            if (in[l] > 255) {
              in[l] = 255;
            }
          } else {
            in[l] = in[l] + 1 * dimdir[l];
          }
          li[l] = in[l];
        }
        lastLPTime[butt] = millis();
      }
      dimming[l] = true;
    }
  }

  //doublepress brightness reset
  if (doublepress[butt]) {
    for (byte lindex = 0; lindex < conf.bmaps[butt].nrdev; lindex++) {
      byte l = conf.bmaps[butt].devices[lindex];
      in[l] = 255;
      li[l] = 255;
    }
  }

  if (endpress[butt]) {
    for (byte lindex = 0; lindex < conf.bmaps[butt].nrdev; lindex++) {
      byte l = conf.bmaps[butt].devices[lindex];
      if (dimming[l]) {
        dimming[l] = false;
      }
    }
  }


}
