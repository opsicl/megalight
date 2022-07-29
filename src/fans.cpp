#include "Arduino.h"
#include "declarations.h"
#include "buttons.h"
#include "fans.h"

void setfan(String fanattr, byte* payload, unsigned int length) {

  byte attrindex;
  int fan;
  if (fanattr[1] == String("/")[0]) {
    fan = fanattr[0] - '0';
    attrindex = 1;
  } else {
    fan = fanattr.substring(0,1).toInt();
    attrindex = 2;
  }

  String attr = fanattr.substring(attrindex,(attrindex + 5));
  //Serial.print(fan);
  //Serial.print(F(" -> "));
  //Serial.println(attr);
  

  if (attr == "speed") {
    payload[length] = '\0';
    int value = atoi((char *)payload);
    //Serial.println(value);

    byte hiindex = conf.fans[fan].hispdpin/16;
    byte hipin = conf.fans[fan].hispdpin - 16 * hiindex;

    byte loindex = conf.fans[fan].lowspdpin/16;
    byte lopin = conf.fans[fan].lowspdpin - 16 * loindex;

    publish_metric("fans", String(fan)+"/speed", String(value));

    if (value == 0) {
      fanison[fan] = false;
      onoff[hiindex].setPWM(hipin, 0, 4096);
      onoff[loindex].setPWM(lopin, 0, 4096);
      //digitalWrite(conf.fans[fan].lowspdpin,LOW);
      //digitalWrite(conf.fans[fan].hispdpin,LOW);
    }
    if (value == 1) {
      fanison[fan] = true;
      fanonhi[fan] = false;

      onoff[hiindex].setPWM(hipin, 0, 4096);
      onoff[loindex].setPWM(lopin, 4096, 0);
      //digitalWrite(conf.fans[fan].hispdpin,LOW);
      //digitalWrite(conf.fans[fan].lowspdpin,HIGH);
    }
    if (value == 2) {
      fanison[fan] = true;
      fanonhi[fan] = true;
      onoff[loindex].setPWM(lopin, 0, 4096);
      onoff[hiindex].setPWM(hipin, 4096, 0);
      //digitalWrite(conf.fans[fan].lowspdpin,LOW);
      //digitalWrite(conf.fans[fan].hispdpin,HIGH);
    }
  }

}

void fansbutton(byte butt) {
  //Serial.print("Fans button pressed: ");
  //Serial.println(butt);
  for (byte fb = 0; fb < conf.bmaps[butt].nrdev; fb++) {


    byte f = conf.bmaps[butt].devices[fb];
    byte hiindex = conf.fans[f].hispdpin/16;
    byte hipin = conf.fans[f].hispdpin - 16 * hiindex;

    byte loindex = conf.fans[f].lowspdpin/16;
    byte lopin = conf.fans[f].lowspdpin - 16 * loindex;

    if (shortpress[butt]) {
      if (fanison[f]) {
        fanison[f] = false;
        publish_metric("fans", String(f)+"/speed", String(0));

        onoff[loindex].setPWM(lopin, 0, 4096);
        onoff[hiindex].setPWM(hipin, 0, 4096);

        //digitalWrite(lpin,LOW);
        //digitalWrite(hpin,LOW);
      } else {
        fanison[f] = true;
        publish_metric("fans", String(f)+"/speed", String(1));

        onoff[hiindex].setPWM(hipin, 0, 4096);
        onoff[loindex].setPWM(lopin, 4096, 0);

        //digitalWrite(lpin,HIGH);
        fanonhi[f] = false;
      }
    }
  }
}
