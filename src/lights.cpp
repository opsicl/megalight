#include "Arduino.h"
#include "declarations.h"
#include "buttons.h"
#include "lights.h"
#include "fans.h"




void setlight(String lightattr, byte* payload, unsigned int length) {

  Serial.println("herexx");
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
      //Serial.println("here_light");
      byte lindexc;
      byte pwmindexc;
      if (conf.lights[l].cpin >= 16) {
        pwmindexc = 0;
        lindexc = conf.lights[l].cpin - 16;
      }
      else {
        pwmindexc = 1;
        lindexc = conf.lights[l].cpin ;
      }
      pwm[pwmindexc].setPWM(lindexc, 0, cin);

      //analogWrite(conf.lights[l].wpin,win);
      byte lindexw;
      byte pwmindexw;
      if (conf.lights[l].wpin >= 16) {
        pwmindexw = 0;
        lindexw = conf.lights[l].cpin - 16;
      }
      else {
        pwmindexw = 1;
        lindexw = conf.lights[l].wpin ;
      }
      pwm[pwmindexc].setPWM(lindexw, 0, win);

      //Serial.println("here_light2");

    }
    else {
      if (millis() - lastIntSet[l] > 1) {
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
      }

      //Serial.println("here_light3");
      byte pwmindex;
      byte lindex;
      if (conf.lights[l].cpin >= 16) {
        pwmindex = 1;
        lindex = conf.lights[l].cpin - 16;
      }
      else {
        pwmindex = 0;
        lindex = conf.lights[l].cpin;
      }
      pwm[pwmindex].setPWM(conf.lights[l].cpin, 0, si[l]);
      //analogWrite(conf.lights[l].cpin,si[l]);
      //Serial.println("here_light4");
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
