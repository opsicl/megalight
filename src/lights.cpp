#include "Arduino.h"
#include "declarations.h"
#include "buttons.h"
#include "lights.h"
#include "fans.h"




void setlight(String lightattr, String payload) {

  byte attrindex = lightattr.indexOf("/");
  String lightstr = lightattr.substring(0,attrindex);
  publish_metric("log","lights_on",lightstr);
  int light = lightstr.toInt();

  //Serial.println(light);
  String attr = lightattr.substring(attrindex + 1,lightattr.length());
  //Serial.println(attr);

  publish_metric("log","lights_on_attr",String(light) + " " + attr);
  if (attr == "brightness") {
    //Serial.print(light);
    //Serial.print(F(" -> "));
    //Serial.println(value);
    if (in[light] != 0) {
      in[light] = payload.toInt();
    }
    li[light] = payload.toInt();
    publish_metric("lights", String(light) + "/brightness", String(li[light]));

    //publish_metric("log","lights_on",String(light) + " " + String(in[light]));
  }

  if (attr == "colortemp") {
    ct[light] = payload.toInt();
  }

  if (attr == "onoff") {
    if (payload.toInt() == 0) {
      in[light] = 0;
      publish_metric("lights", String(light) + "/onoff", String(0));
    }
    if (payload.toInt() == 1) {
        if (li[light] == 0) {
          in[light] = 4095;
          li[light] = 4095;
        }
        else {
          in[light] = li[light];
        }

      publish_metric("lights", String(light) + "/onoff", String(1));
    }

  }

}

void applyintensities() {
  for (byte l = 0; l < conf.nrlights; l++) {

    byte pwmindexc = conf.lights[l].cpin / 16;
    byte lindexc = conf.lights[l].cpin - 16*pwmindexc;

    //only do stuff if something changed
    if ((in[l] != si[l] or ct[l] != st[l]) and (millis() - lastIntSet[l] > 1)) {

      if (in[l] > si[l]) {
        if (in[l] - si[l] > 20) {
          si[l] += 20;
        } else {
          si[l] = in[l];
        }
        lastIntSet[l] = millis();
      }
      if (in[l] < si[l]) {
        if (si[l] - in[l] > 20) {
          si[l] -= 20;
        } else {
          si[l] = in[l];
        }
        lastIntSet[l] = millis();
      }


      if (conf.lights[l].tempadj) {

        if (ct[l] > 333 or ct[l] < 250) {
          ct[l] = 292;
        }

        byte pwmindexw = conf.lights[l].wpin / 16;
        byte lindexw = conf.lights[l].wpin - 16*pwmindexw;

        //invent a color temp metric adjusted to the intensity
        int tdelta = si[l] / 42 * (ct[l] - 292);
        //publish_metric("log", String(l)+" tmetric", String(tdelta));

        int cin = si[l];
        int win = si[l];

        if (tdelta > 0) {
          cin = win - tdelta;
        }
        if (tdelta < 0) {
          win = cin + tdelta; 
        }

        pwm[pwmindexc].setPWM(lindexc, 0, cin);

        pwm[pwmindexw].setPWM(lindexw, 0, win);
        st[l] = ct[l];


      } else {
        pwm[pwmindexc].setPWM(lindexc, 0, si[l]);
        st[l] = ct[l];
      }
      if (in[l] == si[l]) {
        if (lastIntSet[l] != 0 and not dimming[l]) {
          //we just finished setting the lights, sending state
          publish_metric("lights", String(l)+"/brightness", String(si[l]));
          publish_metric("lights", String(l)+"/colortemp", String(st[l]));
          publish_metric("log", String(l)+"/dimming", String(dimming[l]));
          lastIntSet[l] = 0;
        }
      }
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
          in[l] = 4095;
          li[l] = 4095;
        }
        else {
          in[l] = li[l];
        }
      }
    }
  }

  //longpress dimming   
  if (longpressing[butt] and (millis() - lastLPTime[butt] > 15)) {
    for (byte lindex = 0; lindex < conf.bmaps[butt].nrdev; lindex++) {
      byte l = conf.bmaps[butt].devices[lindex];
      if (not dimming[l] or in[l] == 0) {
        if (in[l] <= 10) {
          dimdir[l] = 1;
        } else {
          dimdir[l] = -1;
        }
        dimming[l] = true;
      }

      int dimstep = in[l]/50;
      if (dimstep < 1) {
        dimstep = 1;
      }

      in[l] = in[l] + dimstep * dimdir[l];
        if (in[l] > 4095) {
          in[l] = 4095;
        }
        if (in[l] < 10) {
          in[l] = 10;
        }
      //}
      li[l] = in[l];
      lastLPTime[butt] = millis();
    }
  }

  //doublepress brightness reset
  if (doublepress[butt]) {
    for (byte lindex = 0; lindex < conf.bmaps[butt].nrdev; lindex++) {
      byte l = conf.bmaps[butt].devices[lindex];
      in[l] = 4095;
      li[l] = 4095;
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
