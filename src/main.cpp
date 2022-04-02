#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <string.h>
#include <avr/wdt.h>
#include "declarations.h"
#include "buttons.h"
#include "lights.h"
#include "shutters.h"
#include "fans.h"
#include "MemoryFree.h"


byte id = 0x98;
char idString[] = "98";

byte mac[] = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, id };
IPAddress server(192,168,91,215);

char* metricsTopic = "metrics/";


EthernetClient ethClient;
PubSubClient mclient(ethClient);

Conf conf;
long lastPrint;
long configTime;
long lastPressTime[15];
long lastShortPress[15];
long duration[15];
long lastLPTime[15];
long lastIntSet[15];
long shutterStart[15];
long eottime[15];
long lastReconnectAttempt = 0;
long lastReport = 0;
long lastVarDump = 0;
byte debugpin;
bool lastpress[15];
//color temp
int ct[15];
//intensity
int in[15];
//last intensity
int li[15];
//currently set intensity
int si[15];
//dimming direction
int dimdir[15];
//dimming in progress
bool dimming[15];
int shuttgtstate[15];
int shutcurstate[15];
int shutinitstate[15];
bool shutinprogress[15];
bool interrupt[15];
bool fanison[15];
bool fanonhi[15];
bool pinsset = false;
bool longpressing[15];
bool shortpress[15];
bool doublepress[15];
bool endpress[15];

void callback(char* topic, byte* payload, unsigned int length);

void setup(void) {
  // change the timers frequency to try to avoid flickering
  int myEraser = 7;
  TCCR1B &= ~myEraser;
  TCCR2B &= ~myEraser;
  TCCR3B &= ~myEraser;
  TCCR4B &= ~myEraser;

  // frequencyes:
  //  prescaler = 1 ---> PWM frequency is 31000 Hz
  //  prescaler = 2 ---> PWM frequency is 4000 Hz
  //  prescaler = 3 ---> PWM frequency is 490 Hz (default value)
  //  prescaler = 4 ---> PWM frequency is 120 Hz
  //  prescaler = 5 ---> PWM frequency is 30 Hz
  //  prescaler = 6 ---> PWM frequency is <20 Hz
  int myPrescaler = 2;

  //TCCR0B will not be touched as it is used as main timer for millis() and other internals
  //pins 13 and 4 should not be used as light controllers for this reason
  TCCR1B |= myPrescaler; // pins 11,12
  TCCR2B |= myPrescaler; // pins 9,10
  TCCR3B |= myPrescaler; // pins 2,3,5
  TCCR4B |= myPrescaler; // pins 6,7,8


  // Pins 10,4, 50, 51, and 52 are used by the ethernet shield, avoid trying to use them for anything else
  // The 11 PWM usable pins on the mega are: 2,3,5,6,7,8,9,11,12,45,46

  // set all pins low for safety
  for (byte pin = 0; pin <= 54; pin++) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
  //enable the hardware watchdog at 2s
  wdt_enable(WDTO_2S);

  Serial.begin(9600); //Begin serial communication
  Serial.println(F("Arduino Lights/Shutters controller")); //Print a message

  

  strcat(metricsTopic, idString);
  Serial.println(F("Setup done"));
  //request config on startup
}

void configure(byte* payload) {

  Serial.println(F("Got reconfiguration request"));
  StaticJsonDocument<1024> jconf;
  DeserializationError error = deserializeJson(jconf, payload);
  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    //Serial.println(error.c_str());
    return;
  }

  conf.nrlights = jconf["l"].size();
  for (byte i = 0; i < conf.nrlights; i++) {
    conf.lights[i].tempadj = jconf["l"][i]["t"];
    conf.lights[i].cpin = jconf["l"][i]["c"];
    if (conf.lights[i].tempadj) {
      conf.lights[i].wpin = jconf["l"][i]["w"];
    }
  }

  conf.nrshutters = jconf["s"].size();
  for (byte i = 0; i < conf.nrshutters; i++) {
    conf.shutters[i].uppin = jconf["s"][i]["u"];
    conf.shutters[i].downpin = jconf["s"][i]["d"];
    conf.shutters[i].time = jconf["s"][i]["t"];
  }

  conf.nrfans = jconf["f"].size();
  for (byte i = 0; i < conf.nrfans; i++) {
    conf.fans[i].hispdpin = jconf["f"][i]["h"];
    conf.fans[i].lowspdpin = jconf["f"][i]["l"];
  }

  conf.nrbutt = jconf["b"].size();
  for (byte i = 0; i < conf.nrbutt; i++) {

    conf.bmaps[i].pin = jconf["b"][i]["p"];
    conf.bmaps[i].light = jconf["b"][i]["l"];
    conf.bmaps[i].fan = jconf["b"][i]["f"];
    conf.bmaps[i].shutterup = jconf["b"][i]["su"];
    conf.bmaps[i].shutterdown = jconf["b"][i]["sd"];
    conf.bmaps[i].nrdev = jconf["b"][i]["d"].size();
    for (byte j = 0; j < conf.bmaps[i].nrdev; j++) {
      conf.bmaps[i].devices[j]=jconf["b"][i]["d"][j];
    }
  }
  pinsset = false;
  //write config to eeprom
  //i suspect issues with the eeprom
  //EEPROM.put(0, conf);

}


void applystoredconfig() {

  //Serial.println("Applying stored config event");
  EEPROM.get(0, conf);

}


boolean reconnect() {
  Ethernet.init();
  //delay(100);
  if (Ethernet.begin(mac) != 0) {
    //Serial.println(Ethernet.localIP());
    mclient.setServer(server, 1883);
    mclient.setCallback(callback);
  }
  if (mclient.connect("arduinoClient2")) {
    // ... and resubscribe

    char topic[20];
    snprintf(topic, sizeof topic, "%s/lights/#", idString );
    mclient.subscribe(topic);
    snprintf(topic, sizeof topic, "%s/shutters/#", idString );
    mclient.subscribe(topic);
    snprintf(topic, sizeof topic, "%s/fans/#", idString );
    mclient.subscribe(topic);
    snprintf(topic, sizeof topic, "%s/setconfig", idString );
    mclient.subscribe(topic);
    Serial.println(F("Connected to mqtt"));
  }
  return mclient.connected();
}

void callback(char* topic, byte* payload, unsigned int length) {
  //Serial.print("Message arrived [");
  //Serial.print(topic);
  //Serial.print("] ");
  //Serial.println(length);
  for (unsigned int i=0;i<length;i++) {
    //Serial.print((char)payload[i]);
  }
  if (String(topic).substring(3,10) == "lights/") {
    //Serial.print(F("got light "));

    String lightattr = String(topic).substring(10,String(topic).length());
    setlight(lightattr,payload,length);
  }
  if (String(topic).substring(3,12) == "shutters/") {
    String shutterattr = String(topic).substring(12,String(topic).length());
    //Serial.print("Shutter command via MQTT ");
    //Serial.println(shutter.toInt());
    setshutter(shutterattr,payload,length);
  }
  if (String(topic).substring(3,8) == "fans/") {
    String fanattr = String(topic).substring(8,String(topic).length());
    //Serial.print("Fan command via MQTT ");
    //Serial.println(shutter.toInt());
    setfan(fanattr,payload,length);
  }


  if (String(topic) == "98/setconfig") {
    configure(payload);
  }

  //Serial.println();
}

void setpins() {
  for (byte l = 0; l < conf.nrlights; l++) {
    if (conf.lights[l].tempadj) {
      //Serial.print("setting pin ");
      //Serial.print(conf.lights[l].wpin);
      //Serial.println(" as output");
      pinMode(conf.lights[l].wpin, OUTPUT);
    }
    pinMode(conf.lights[l].cpin, OUTPUT);
    //Serial.print("setting pin ");
    //Serial.print(conf.lights[l].cpin);
    //Serial.println(" as output");

  }
  for (byte b = 0; b < conf.nrbutt; b++) {
    //Serial.print("setting pin ");
    //Serial.print(conf.bmaps[b].pin);
    //Serial.println(" as input");

    pinMode(conf.bmaps[b].pin, INPUT);
  }

  for (byte s = 0; s < conf.nrshutters; s++) {
    //Serial.print("setting pin ");
    //Serial.print(conf.shutters[s].uppin);
    //Serial.println(" as output");
    //Serial.print("setting pin ");
    //Serial.print(conf.shutters[s].downpin);
    //Serial.println(" as output");

    pinMode(conf.shutters[s].uppin, OUTPUT);
    pinMode(conf.shutters[s].downpin, OUTPUT);
    digitalWrite(conf.shutters[s].uppin, LOW);
    digitalWrite(conf.shutters[s].downpin, LOW);
  }

  for (byte f = 0; f < conf.nrfans; f++) {
    //Serial.print("setting pin ");
    //Serial.print(conf.fans[f].hispdpin);
    //Serial.println(" as output");
    //Serial.print("setting pin ");
    //Serial.print(conf.fans[f].lowspdpin);
    //Serial.println(" as output");

    pinMode(conf.fans[f].lowspdpin, OUTPUT);
    pinMode(conf.fans[f].hispdpin, OUTPUT);
    digitalWrite(conf.fans[f].lowspdpin, LOW);
    digitalWrite(conf.fans[f].hispdpin, LOW);
  }



  pinsset = true;
}

int pltoint(byte* payload, unsigned int length) {
  char temp[length+1];

  strncpy(temp, payload, length);
  //temp[length] = '\0';
  return atoi(temp);
}


void loop(void) { 

  wdt_reset();

  Ethernet.maintain();
  if (!mclient.connected()) {
    //attempt to reconnect to mqtt every 20s
    if (millis() - lastReconnectAttempt > 20000 or lastReconnectAttempt == 0) {
      Serial.println(F("MQTT not connected"));
      lastReconnectAttempt = millis();
      if (reconnect()) {
        lastReconnectAttempt = 0;
        mclient.publish("configreq",idString);
        configTime = millis();
      }
    }
  }
  else {
    mclient.loop();
  }


  //alow 5s to receive config, else use the locally stored one
  if (conf.nrlights == 0 and conf.nrshutters == 0 and conf.nrfans == 0)  {
    if (millis() - configTime > 5000) {
      //applystoredconfig();
      //if (conf.nrlights == 0 and conf.nrshutters == 0 and conf.nrfans == 0) {
        //Serial.println("publishing configreq");
        mclient.publish("configreq",idString);
        configTime = millis();
      }
  }

  else {
    if (not pinsset) {
      setpins();
    }
    handlebuttons();
    applyintensities();
    controlshutters();
    // Print debug info evrey 10s
    //if (millis() - lastVarDump > 10000) {
    //  Serial.print("shutter 0 current state ");
    //  Serial.println(shutcurstate[0]);
    //  Serial.print("target state ");
    //  Serial.println(shuttgtstate[0]);
    //  Serial.print("pin last used ");
    //  Serial.println(debugpin);
    //  lastVarDump = millis();
    //}
  }

  //report current state every 30s
  if (millis() - lastReport > 10000) {
    //char topic[10] = "metrics/" + idString;
    //char msg[12] = "freeram " + String(FreeMemory());
    //mclient.publish(strcat("metrics/", idString), "test!");
    //Serial.println(metricsTopic);
    char topic[20];
    snprintf(topic, sizeof topic, "%s/freeram", metricsTopic );
    char value[5];
    snprintf(value, sizeof value, "%i", freeMemory());

    Serial.print(topic);
    Serial.println(freeMemory());
    mclient.publish(topic, value, true);

    //strcat("freeram ", freeMemory()));
    //Serial.print(F("Free RAM = ")); //F function does the same and is now a built in library, in IDE > 1.0.0
    //Serial.println(freeMemory(), DEC);  // print how much RAM is available.
//    //report current state
//    Serial.println(F("lighs:"));
//    for (byte l=0; l<conf.nrlights; l++) {
//      Serial.print(l);
//      Serial.print(F("->"));
//      Serial.println(in[l]);
//    }
//    Serial.println(F("shutters:"));
//    for (byte s=0; s<conf.nrshutters; s++) {
//      Serial.print(s);
//      Serial.print(F("->"));
//      Serial.println(shutcurstate[s]);
//    }
//    Serial.println(F("fans:"));
//    for (byte f=0; f<conf.nrfans; f++) {
//      Serial.print(f);
//      Serial.print(F("->"));
//      Serial.println(fanison[f]);
//    }
//    Serial.print(F("Ethernet: "));
//    Serial.println(Ethernet.maintain());
//



    lastReport = millis();

  }

}
