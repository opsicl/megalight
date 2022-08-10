#include <ETH.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <string.h>
#include "declarations.h"
#include "buttons.h"
#include "lights.h"
#include "shutters.h"
#include "fans.h"
#include <OneWire.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>


byte id = 0x98;
String idString = "downstairs-lights";

IPAddress server(192,168,91,215);

String metricsTopic = "metrics/";

Adafruit_PWMServoDriver pwm[] = {Adafruit_PWMServoDriver(0x48), Adafruit_PWMServoDriver(0x44)};
Adafruit_PWMServoDriver onoff[] = {Adafruit_PWMServoDriver(0x41)};

static bool eth_connected = false;
WiFiClient ethClient;
PubSubClient mclient(ethClient);

Conf conf;
long lastPrint;
long cfgTime;
long lastPressTime[35];
long lastShortPress[35];
long duration[35];
long lastLPTime[35];
long lastIntSet[35];
long shutterStart[15];
long lastReconnectAttempt = 0;
long lastReport = 0;
long lastVarDump = 0;
byte debugpin;
bool lastpress[35];
//color temp
int ct[35];
//intensity
int in[35];
//last intensity
int li[35];
//currently set intensity
int si[35];
//dimming direction
int dimdir[35];
//dimming in progress
bool dimming[35];
int shuttgtstate[15];
int shutcurstate[15];
int shutinitstate[15];
bool shutinprogress[15];
bool interrupt[15];
bool fanison[15];
bool fanonhi[15];
bool pinsset = false;
bool longpressing[35];
bool shortpress[35];
bool doublepress[35];
bool endpress[35];

void callback(char* topic, byte* payload, unsigned int length);

void setup(void) {
  Serial.begin(9600); //Begin serial communication
  Serial.println(F("Arduino Lights/Shutters controller")); //Print a message

  ETH.begin();
  //OTA
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();
  //END OTA



  for (byte i=0; i < 2; i++) {
    Serial.println(i);
    pwm[i].begin();
    pwm[i].setPWMFreq(1600);
  }

  for (byte i=0; i < 1; i++) {
    Serial.println(i);
    onoff[i].begin();
    onoff[i].setPWMFreq(100);
  }


  //// Pins 10,4, 50, 51, and 52 are used by the ethernet shield, avoid trying to use them for anything else
  //// The 11 PWM usable pins on the mega are: 2,3,5,6,7,8,9,11,12,45,46

  //// set all pins low for safety
  //for (byte pin = 0; pin <= 54; pin++) {
  //  pinMode(pin, OUTPUT);
  //  digitalWrite(pin, LOW);
  //}
  //enable the hardware watchdog at 2s
  //wdt_enable(WDTO_4S);

  

  metricsTopic += idString;
  Serial.println(F("Setup done"));
  //request config on startup
}

void publish_metric (String metric, String tag, String value) {
  //Serial.print(metric);
  //Serial.print(F("/"));
  //Serial.print(tag);
  //Serial.print(F(" "));
  //Serial.println(value);
  //char pubtopic[70];
  //char val[10];

  //value.toCharArray(val,value.length()+1);
  //snprintf(pubtopic, sizeof pubtopic, "%s/%s/%s", metricsTopic.c_str(), metric.c_str(), tag.c_str());
  String pubtopic = metricsTopic + "/" + metric + "/" + tag;
  Serial.println("publishig " + pubtopic + " " + value);
  mclient.publish(pubtopic.c_str(), value.c_str(), true);

}

void configure(String payload) {

  //Serial.println(F("Got reconfiguration request"));
  //publish_metric("config", "received", String(1));
  //StaticJsonDocument<256> jconf;
  DynamicJsonDocument jconf(4096);
  DeserializationError error = deserializeJson(jconf, payload);
  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    //Serial.println(error.c_str());
    publish_metric("config", "deserialize", error.c_str());

    return;
  }
  publish_metric("config", "deserialize", "success");


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
  //Serial.println(conf.nrbutt);
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
  publish_metric("config", "accepted", String(1));
  //write config to eeprom
  //i suspect issues with the eeprom
  //EEPROM.put(0, conf);

}


void applystoredconfig() {

  //Serial.println("Applying stored config event");
  EEPROM.get(0, conf);

}


boolean reconnect() {
  mclient.setBufferSize(4096);
  mclient.setServer(server, 1883);
  mclient.setCallback(callback);


  if (mclient.connect(idString.c_str())) {

    // ... and resubscribe
    //char topic[20];
    //snprintf(topic, sizeof topic, "%s/lights/#", idString.c_str() );
    //mclient.subscribe(topic);
    //snprintf(topic, sizeof topic, "%s/shutters/#", idString.c_str() );
    //mclient.subscribe(topic);
    //snprintf(topic, sizeof topic, "%s/fans/#", idString.c_str() );
    //mclient.subscribe(topic);
    //snprintf(topic, sizeof topic, "%s/setconfig", idString.c_str() );
    //mclient.subscribe(topic);

    String topic;
    topic = idString + "/lights/#";
    mclient.subscribe(topic.c_str());
    topic = idString + "/shutters/#";
    mclient.subscribe(topic.c_str());
    topic = idString + "/fans/#";
    mclient.subscribe(topic.c_str());
    topic = idString + "/fans/#";
    mclient.subscribe(topic.c_str());
    topic = idString + "/setconfig";
    mclient.subscribe(topic.c_str());



    Serial.println(F("Connected to mqtt"));
  }
  return mclient.connected();
}

void callback(char* topic, byte* payload, unsigned int length) {

  char charpl[length+1];  //a buffer to hold the string
  memcpy(charpl, payload, length); //copy the payload to the buffer
  charpl[length] = '\0';
  String strPayload = String(charpl);
  String strTopic = String(topic);
  //log("Received message on topic: " + strTopic);
  //log(stringpl);

  //publish_metric("log", "msg_received_size", String(length));
  //publish_metric("log", "msg_received_topic", strTopic);
  //Serial.print("Message arrived [");
  //Serial.print(topic);
  //Serial.print("] ");
  //Serial.println(length);
  //for (unsigned int i=0;i<length;i++) {
  //  Serial.print((char)payload[i]);
  //}


  String lightsprefix = idString + "/lights/";
  String shuttersprefix = idString + "/shutters/";
  String fansprefix = idString + "/fans/";

  if (strTopic.indexOf(lightsprefix) == 0) {
    //Serial.print(F("got light "));

    String lightattr = String(topic).substring(lightsprefix.length(),strTopic.length());
    setlight(lightattr,strPayload);
  }

  if (strTopic.indexOf(shuttersprefix) == 0) {
    //Serial.print(F("got light "));

    String shutterattr = String(topic).substring(shuttersprefix.length(),strTopic.length());
    setshutter(shutterattr,strPayload);
  }

  if (strTopic.indexOf(fansprefix) == 0) {
    //Serial.print(F("got light "));

    String fanattr = String(topic).substring(fansprefix.length(),strTopic.length());
    setfan(fanattr,strPayload);
  }

  if (strTopic == idString + "/setconfig") {
    configure(strPayload);
  }

  //Serial.println();
}

void setpins() {
//  for (byte l = 0; l < conf.nrlights; l++) {
//    if (conf.lights[l].tempadj) {
//      //Serial.print("setting pin ");
//      //Serial.print(conf.lights[l].wpin);
//      //Serial.println(" as output");
//      pinMode(conf.lights[l].wpin, OUTPUT);
//    }
//    pinMode(conf.lights[l].cpin, OUTPUT);
//    //Serial.print("setting pin ");
//    //Serial.print(conf.lights[l].cpin);
//    //Serial.println(" as output");
//
//  }
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

    byte ctrlindexup = conf.shutters[s].uppin / 16;
    byte uppin = conf.shutters[s].uppin - 16*ctrlindexup;
    byte ctrlindexdn = conf.shutters[s].downpin / 16;
    byte downpin = conf.shutters[s].downpin - 16*ctrlindexdn;

    onoff[ctrlindexup].setPWM(uppin, 0, 4096);
    onoff[ctrlindexdn].setPWM(downpin, 0, 4096);

    //digitalWrite(conf.shutters[s].uppin, LOW);
    //digitalWrite(conf.shutters[s].downpin, LOW);
  }

//  for (byte f = 0; f < conf.nrfans; f++) {
//    //Serial.print("setting pin ");
//    //Serial.print(conf.fans[f].hispdpin);
//    //Serial.println(" as output");
//    //Serial.print("setting pin ");
//    //Serial.print(conf.fans[f].lowspdpin);
//    //Serial.println(" as output");
//
//    pinMode(conf.fans[f].lowspdpin, OUTPUT);
//    pinMode(conf.fans[f].hispdpin, OUTPUT);
//    digitalWrite(conf.fans[f].lowspdpin, LOW);
//    digitalWrite(conf.fans[f].hispdpin, LOW);
//  }



  pinsset = true;
}

int pltoint(byte* payload, unsigned int length) {
  char temp[length+1];

  strncpy(temp, (char*)payload, length);
  //temp[length] = '\0';
  return atoi(temp);
}


void loop(void) { 

  //OTA code update
  ArduinoOTA.handle();

  if (!mclient.connected()) {
    //attempt to reconnect to mqtt every 20s
    if (millis() - lastReconnectAttempt > 20000 or lastReconnectAttempt == 0) {
      Serial.println(F("MQTT not connected"));
      lastReconnectAttempt = millis();
      if (reconnect()) {
        lastReconnectAttempt = 0;
        mclient.publish("configreq",idString.c_str());
        cfgTime = millis();
      }
    }
  }
  else {
    mclient.loop();
  }


  //alow 5s to receive config, else use the locally stored one
  if (conf.nrlights == 0 and conf.nrshutters == 0 and conf.nrfans == 0)  {
    if (millis() - cfgTime > 5000) {
      //applystoredconfig();
      //if (conf.nrlights == 0 and conf.nrshutters == 0 and conf.nrfans == 0) {
        //Serial.println("publishing configreq");
        mclient.publish("configreq",idString.c_str());
        cfgTime = millis();
      }
  }
  

  else {
    //Serial.println("here1");
    //if (not pinsset) {
    //  setpins();
    //}
    //Serial.println("here2");
    handlebuttons();
    //Serial.println("here3");
    applyintensities();
    //Serial.println("here4");
    controlshutters();
    //Serial.println("here5");
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
  if (millis() - lastReport > 5000) {
    //publish_metric("log", "conf_nrlights", String(conf.nrlights));
    //publish_metric("log", "conf_nrfans", String(conf.nrfans));
    //publish_metric("log", "conf_nrshutters", String(conf.nrshutters));


    for (byte l = 0; l < conf.nrlights; l++) {
      publish_metric("lights", String(l)+"/brightness", String(in[l]));
    }

    for (byte s = 0; s < conf.nrshutters; s++) {
      publish_metric("shutters", String(s)+"/open", String(shutcurstate[s]));
    }
    for (byte f = 0; f < conf.nrfans; f++) {
      byte speed = 0;
      if (fanison[f]) {
        speed = 1;
        if (fanonhi[f]) {
          speed = 2;
        }
      }
      publish_metric("fans", String(f)+"/speed", String(speed));
    }


    lastReport = millis();

  }

}
