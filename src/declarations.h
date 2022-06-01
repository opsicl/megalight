#include <Adafruit_PWMServoDriver.h>
int pltoint(byte* payload, unsigned int length);
void publish_metric (String metric, String tag, String value);

struct light {
  bool tempadj;
  byte wpin;
  byte cpin;
};

struct shutter {
  byte uppin;
  byte downpin;
  byte time;
};

struct fan {
  byte hispdpin;
  byte lowspdpin;
};

struct bmap {
  bool shutterup;
  bool shutterdown;
  bool light;
  bool fan;
  byte nrdev;
  byte devices[10];
  byte pin;
};

struct Conf {
  byte nrlights;
  byte nrshutters;
  byte nrfans;
  byte nrbutt;
  bmap bmaps[35];
  shutter shutters[15];
  light lights[35];
  fan fans[15];
};

extern Conf conf;
extern long lastPrint;
extern long lastPressTime[35];
extern long lastShortPress[35];
extern long duration[35];
extern long lastLPTime[35];
extern long lastIntSet[35];
extern long shutterStart[15];
extern long eottime[15];
extern long lastReconnectAttempt;
extern bool lastpress[35];
extern bool longpressing[35];
extern bool shortpress[35];
extern bool doublepress[35];
extern bool endpress[35];
extern byte debugpin;
//color temp
extern int ct[35];
//intensity
extern int in[35];
//last intensity
extern int li[35];
//currently set intensity
extern int si[35];
//dimming direction
extern int dimdir[35];
//dimming in progress
extern bool dimming[35];
extern int shuttgtstate[15];
extern int shutcurstate[15];
extern int shutinitstate[15];
extern bool shutinprogress[15];
extern bool interrupt[15];
extern bool fanison[15];
extern bool fanonhi[15];
extern long configTime;
extern bool pinsset;
extern Adafruit_PWMServoDriver pwm[];
extern Adafruit_PWMServoDriver onoff[];


