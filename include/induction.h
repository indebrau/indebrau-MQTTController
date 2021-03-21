#ifndef INDUCTION
#define INDUCTION
#include <Arduino.h>

class Induction
{
  unsigned long timeTurnedOff;

  long timeOutCommand = 5000;  // TimeOut for serial command
  long timeOutReaction = 2000; // TimeOut für induction cooker
  unsigned long lastInterrupt;
  unsigned long lastCommand;
  bool inputStarted = false;
  byte inputCurrent = 0;
  byte inputBuffer[33];
  bool isError = false;
  byte error = 0;

  long powerSampletime = 20000;
  unsigned long powerLast;
  long powerHigh = powerSampletime; // Dauer des "HIGH"-Anteils im Schaltzyklus
  long powerLow = 0;

public:
  byte PIN_WHITE = 9;     // Relais
  byte PIN_YELLOW = 9;    // Out to induction
  byte PIN_INTERRUPT = 9; // In from induction
  byte power = 0;
  byte newPower = 0;
  byte CMD_CUR = 0; // current command
  bool isRelayOn = false;
  bool isInduOn = false; // Status: is power > 0?
  bool isPower = false;
  String mqtttopic = "";
  bool isEnabled = false;
  int delayAfteroff = 120; // in seconds

  Induction();

  void change(byte pinwhite, byte pinyellow, byte pinblue, String topic, long delayoff, bool is_enabled);
  void mqtt_subscribe();
  void mqtt_unsubscribe();
  void handlemqtt(char *payload);
  bool updateRelay();
  void update();
  void updatePower();
  void sendCommand(const int command[33]);
};

void handleInduction();
void handleRequestInduction();
void handleRequestInductionConfig();
void handleSetIndu();

#endif