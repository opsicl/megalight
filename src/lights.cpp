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

    if (millis() - lastIntSet[l] > 20) {
      if (in[l] == si[l]) {
        if (lastIntSet[l] != 0 and dimming[l] == false) {
          //we just finished setting the lights, sending state
          publish_metric("lights", String(l)+"/brightness", String(si[l]));
          lastIntSet[l] = 0;
        }
      } else {
        if (in[l] > si[l]) {
          if (in[l] - si[l] > 400) {
            si[l] += 400;
          } else {
            si[l] = in[l];
          }
          lastIntSet[l] = millis();
        }
        if (in[l] < si[l]) {
          if (si[l] - in[l] > 400) {
            si[l] -= 400;
          } else {
            si[l] = in[l];
          }
          lastIntSet[l] = millis();
        }
      }


      if (conf.lights[l].tempadj) {
        byte pwmindexw = conf.lights[l].wpin / 16;
        byte lindexw = conf.lights[l].wpin - 16*pwmindexw;

        //float cin = 2*in[l]/(2*ct[l]+1);
        //float win = 2*ct[l]*cin;
        float win = 2 * in[l] / ((2 * ct[l]) + 1);
        int cin = 2*ct[l];
        if (win > 4095) {
          win = 4095;
          cin = round(2*ct[l]/win);
        }
        if (cin > 4095) {
          cin = 4095;
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

        //analogWrite(conf.lights[l].cpin,cin);
        pwm[pwmindexc].setPWM(lindexc, 0, cin);

        //analogWrite(conf.lights[l].wpin,win);
        pwm[pwmindexw].setPWM(lindexw, 0, win);


      } else {
        pwm[pwmindexc].setPWM(lindexc, 0, si[l]);
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
  if (longpressing[butt] and (millis() - lastLPTime[butt] > 40)) {
    for (byte lindex = 0; lindex < conf.bmaps[butt].nrdev; lindex++) {
      byte l = conf.bmaps[butt].devices[lindex];
      if (dimming[l] == false or in[l] == 0) {
        if (in[l] <= 10) {
          dimdir[l] = 1;
        } else {
          dimdir[l] = -1;
        }
      }

      if (not (dimming[l] and (in[l] == 1 or in[l] >= 4095))) {
        if (in[l] > 300) {
          in[l] = in[l] + 50 * dimdir[l];
          if (in[l] > 4095) {
            in[l] = 4095;
          }
        } else {
          if (in[l]>60) {
            in[l] = in[l] + 5 * dimdir[l];
          } else {
            in[l] = in[l] + 1 * dimdir[l];
          }
          if (in[l] < 10) {
            in[l] = 10;
          }
        }
        li[l] = in[l];
      }
      lastLPTime[butt] = millis();
      dimming[l] = true;
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
