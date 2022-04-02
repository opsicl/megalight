#include "Arduino.h"
#include "declarations.h"
#include "buttons.h"
#include "fans.h"

void setfan(String fanattr, byte* payload, unsigned int length) {

  byte attrindex;
  int fan;
  if (fanattr[1] == "/") {
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

    if (value == 0) {
      fanison[fan] = false;
      digitalWrite(conf.fans[fan].lowspdpin,LOW);
      digitalWrite(conf.fans[fan].hispdpin,LOW);
    }
    if (value == 1) {
      fanison[fan] = true;
      fanonhi[fan] = false;
      digitalWrite(conf.fans[fan].lowspdpin,HIGH);
      digitalWrite(conf.fans[fan].hispdpin,LOW);
    }
    if (value == 2) {
      fanison[fan] = true;
      fanonhi[fan] = true;
      digitalWrite(conf.fans[fan].lowspdpin,LOW);
      digitalWrite(conf.fans[fan].hispdpin,HIGH);
    }
  }

}

void fansbutton(byte butt) {
  //Serial.print("Fans button pressed: ");
  //Serial.println(butt);
  for (byte fb = 0; fb < conf.bmaps[butt].nrdev; fb++) {
    byte f = conf.bmaps[butt].devices[fb];
    byte lpin = conf.fans[f].lowspdpin;
    byte hpin = conf.fans[f].hispdpin;
    if (shortpress[butt]) {
      if (fanison[f]) {
        fanison[f] = false;
        digitalWrite(lpin,LOW);
        digitalWrite(hpin,LOW);
      } else {
        fanison[f] = true;
        digitalWrite(lpin,HIGH);
        fanonhi[f] = false;
      }
    }
  }
}
