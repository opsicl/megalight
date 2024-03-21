#include <ETH.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <string.h>
#include <OneWire.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "declarations.h"
#include "buttons.h"
#include "lights.h"
#include "shutters.h"
#include "fans.h"
#include <PCF8574.h>
#include "Version.h"
#include "ControllerName.h"



byte id = 0x98;
String idString = CONTROLLER_NAME;

IPAddress server(192,168,91,215);

String metricsTopic = "metrics/";

Adafruit_PWMServoDriver pwm[4];
//{Adafruit_PWMServoDriver(0x48), Adafruit_PWMServoDriver(0x44)};
Adafruit_PWMServoDriver onoff[4];
//{Adafruit_PWMServoDriver(0x41)};
PCF8574 pcf[8];
//{PCF8574(0x20), PCF8574(0x21),PCF8574(0x22), PCF8574(0x23)};


static bool eth_connected = false;
WiFiClient ethClient;
PubSubClient mclient(ethClient);

bool configsuccess = false;
Conf conf;
long last_irq_time[4];
bool irq[4];
bool btn[32];
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
//currently set color temp
int st[35];
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
bool fanison[15];
bool fanonhi[15];
bool pinsset = false;
bool longpressing[35];
bool shortpress[35];
bool doublepress[35];
bool endpress[35];

void IRAM_ATTR pcf_irq0() {
  irq[0] = true;
  last_irq_time[0] = millis();
}
void IRAM_ATTR pcf_irq1() {
  irq[1] = true;
  last_irq_time[1] = millis();
}
void IRAM_ATTR pcf_irq2() {
  irq[2] = true;
  last_irq_time[2] = millis();
}
void IRAM_ATTR pcf_irq3() {
  irq[3] = true;
  last_irq_time[3] = millis();
}


//function to read a hex value from a string of the type "0x4b" and return 0x4b
byte readHex(String str) {
  byte val = 0;
  for (byte i=0; i<str.length(); i++) {
    char c = str.charAt(i);
    if (c >= '0' && c <= '9') {
      val = val * 16 + c - '0';
    } else if (c >= 'a' && c <= 'f') {
      val = val * 16 + c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
      val = val * 16 + c - 'A' + 10;
    }
  }
  return val;
}


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

//Adafruit_PWMServoDriver onoff[] = {Adafruit_PWMServoDriver(0x41)};
//PCF8574 pcf[4] {PCF8574(0x20), PCF8574(0x21),PCF8574(0x22), PCF8574(0x23)};

  conf.nrpwm = 0;
  if (jconf.containsKey("pwm")) {
    conf.nrpwm = jconf["pwm"].size();
    for (byte ctrl=0; ctrl < jconf["pwm"].size(); ctrl++) {
      pwm[ctrl] = Adafruit_PWMServoDriver(readHex(jconf["pwm"][ctrl]));
    }
  }

  conf.nronoff = 0;
  if (jconf.containsKey("onoff")) {
    conf.nronoff = jconf["onoff"].size();
    for (byte ctrl=0; ctrl < jconf["onoff"].size(); ctrl++) {
      publish_metric("log", "onoff", String(readHex(jconf["onoff"][ctrl])));
      onoff[ctrl] = Adafruit_PWMServoDriver(readHex(jconf["onoff"][ctrl]));
    }
  }

  conf.nrpcf = 0;
  if (jconf.containsKey("pcf")) {
    conf.nrpcf = jconf["pcf"].size();
    for (byte ctrl=0; ctrl < jconf["pcf"].size(); ctrl++) {
      pcf[ctrl] = PCF8574(readHex(jconf["pcf"][ctrl]));
    }
  }

  conf.nrlights = 0;
  if (jconf.containsKey("lights")) {
    conf.nrlights = jconf["lights"].size();

    for (byte i = 0; i < conf.nrlights; i++) {
      if (jconf["lights"][i]["tempadjustable"] == 1) {
        conf.lights[i].tempadj = true;
      } else {
        conf.lights[i].tempadj = false;
      }
      conf.lights[i].cpin = jconf["lights"][i]["cold_pin"];
      if (conf.lights[i].tempadj) {
        conf.lights[i].wpin = jconf["lights"][i]["warm_pin"];
      }
    }
  }

  conf.nrshutters = 0;
  if (jconf.containsKey("shutters")) {
    conf.nrshutters = jconf["shutters"].size();
    for (byte i = 0; i < conf.nrshutters; i++) {
      conf.shutters[i].uppin = jconf["shutters"][i]["up_pin"];
      conf.shutters[i].downpin = jconf["shutters"][i]["down_pin"];
      conf.shutters[i].time = jconf["shutters"][i]["travel_time"];
    }
  }

  conf.nrfans = 0;
  if (jconf.containsKey("fans")) {
    conf.nrfans = jconf["fans"].size();
    for (byte i = 0; i < conf.nrfans; i++) {
      conf.fans[i].hispdpin = jconf["fans"][i]["high_speed_pin"];
      conf.fans[i].lowspdpin = jconf["fans"][i]["low_speed_pin"];
    }
  }

  conf.nrbutt = 0;
  if (jconf.containsKey("buttons")) {
    conf.nrbutt = jconf["buttons"].size();
    //Serial.println(conf.nrbutt);
    for (byte i = 0; i < conf.nrbutt; i++) {

      conf.bmaps[i].pin = jconf["buttons"][i]["p"];
      conf.bmaps[i].light = jconf["buttons"][i]["l"];
      conf.bmaps[i].fan = jconf["buttons"][i]["f"];
      conf.bmaps[i].shutterup = jconf["buttons"][i]["su"];
      conf.bmaps[i].shutterdown = jconf["buttons"][i]["sd"];
      conf.bmaps[i].nrdev = jconf["buttons"][i]["d"].size();
      for (byte j = 0; j < conf.bmaps[i].nrdev; j++) {
        conf.bmaps[i].devices[j]=jconf["buttons"][i]["d"][j];
      }
    }
  }
  
  if (conf.nrbutt > 0 or conf.nrlights > 0 or conf.nrshutters > 0 or conf.nrfans > 0) {
    configsuccess = true;
    publish_metric("config", "accepted", String(1));
    //write config to eeprom
    //i suspect issues with the eeprom
    //EEPROM.put(0, conf);
  } else {
    configsuccess = false;
    publish_metric("config", "accepted", String(0));
  }

  for (byte i=0; i < conf.nrpwm; i++) {
    Serial.println(i);
    pwm[i].begin();
    //int rndfreq = rand() % 601 + 1000;
    pwm[i].setPWMFreq(1200);
  }

  for (byte i=0; i < conf.nronoff; i++) {
    Serial.println(i);
    onoff[i].begin();
    onoff[i].setPWMFreq(100);
  }

  for (byte c=0; c < conf.nrpcf; c++) {
    pcf[c].begin();
    for (byte p=0; p<8; p++) {
      pcf[c].write(p, 1);
    }
  }
  if (conf.nrpcf > 0) {
    pinMode(0, INPUT_PULLUP);
    pinMode(1, INPUT_PULLUP);
    pinMode(3, INPUT_PULLUP);
    pinMode(32, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(3), pcf_irq0, FALLING);
    attachInterrupt(digitalPinToInterrupt(0), pcf_irq1, FALLING);
    attachInterrupt(digitalPinToInterrupt(32), pcf_irq2, FALLING);
    attachInterrupt(digitalPinToInterrupt(1), pcf_irq3, FALLING);
  }


  pinsset = false;
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

  for (byte f = 0; f < conf.nrfans; f++) {
    byte ctrlindexlo = conf.fans[f].lowspdpin / 16;
    byte lopin = conf.fans[f].lowspdpin - 16*ctrlindexlo;
    //byte ctrlindexdn = conf.shutters[s].downpin / 16;
    //byte downpin = conf.shutters[s].downpin - 16*ctrlindexdn;

    onoff[ctrlindexlo].setPWM(lopin, 0, 4096);

  }



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
  if (conf.nrlights == 0 and conf.nrshutters == 0 and conf.nrfans == 0 and conf.nrbutt == 0)  {
    if (millis() - cfgTime > 5000) {
      //applystoredconfig();
      //if (conf.nrlights == 0 and conf.nrshutters == 0 and conf.nrfans == 0) {
        //Serial.println("publishing configreq");
        mclient.publish("configreq",idString.c_str());
        cfgTime = millis();
      }
  }
  

  else {
    for (byte p; p < 4; p++) {
      if (irq[p] and millis() - last_irq_time[p] >= 5) {
        readpcf(p);
        publish_metric("log", "handling_irq", String(p));
        irq[p] = false;
      }
    }
    if (conf.nrbutt > 0) {
      handlebuttons();
    }
    if (conf.nrlights > 0) {
      applyintensities();
    }
    if (conf.nrshutters > 0) {
      controlshutters();
    }
  }

  //report current state every 60s
  if (millis() - lastReport > 60000) {
    //publish_metric("log", "conf_nrlights", String(conf.nrlights));
    //publish_metric("log", "conf_nrfans", String(conf.nrfans));
    //publish_metric("log", "conf_nrshutters", String(conf.nrshutters));
    
    publish_metric("log", "version", String(VERSION));
    publish_metric("log", "build", String(BUILD_TIMESTAMP));


    for (byte l = 0; l < conf.nrlights; l++) {
      publish_metric("lights", String(l)+"/brightness", String(li[l]));
      publish_metric("lights", String(l)+"/brightness_percent", String(topercent(li[l])));
      publish_metric("lights", String(l)+"/dimming", String(dimming[l]));
      //publish_metric("log", String(l) +" tempadj", String(conf.lights[l].tempadj));
      if (conf.lights[l].tempadj) {
        publish_metric("lights", String(l)+"/colortemp", String(ct[l]));
      }
      if (in[l] > 0) {
        publish_metric("lights", String(l)+"/onoff", String(1));
      } else {
        publish_metric("lights", String(l)+"/onoff", String(0));
      }

    }

    for (byte s = 0; s < conf.nrshutters; s++) {
      publish_metric("shutters", String(s)+"/open", String(shutcurstate[s]));
    }
    //for (byte b = 0; b < conf.nrbutt; b++) {
    //  publish_metric("buttons", String(b)+"/pressed", String(btn[b]));
    //}

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
