#include <Adafruit_PWMServoDriver.h>
// Update these with values suitable for your network.
int pltoint(byte* payload, unsigned int length);
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
  bmap bmaps[15];
  shutter shutters[15];
  light lights[15];
  fan fans[15];
};

extern Conf conf;
extern long lastPrint;
extern long lastPressTime[15];
extern long lastShortPress[15];
extern long duration[15];
extern long lastLPTime[15];
extern long lastIntSet[15];
extern long shutterStart[15];
extern long eottime[15];
extern long lastReconnectAttempt;
extern bool lastpress[15];
extern bool longpressing[15];
extern bool shortpress[15];
extern bool doublepress[15];
extern bool endpress[15];
extern byte debugpin;
//color temp
extern int ct[15];
//intensity
extern int in[15];
//last intensity
extern int li[15];
//currently set intensity
extern int si[15];
//dimming direction
extern int dimdir[15];
//dimming in progress
extern bool dimming[15];
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


