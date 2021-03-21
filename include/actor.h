#ifndef ACTOR
#define ACTOR
#include <Arduino.h>

class Actor
{
    unsigned long powerLast; // Time for High or Low
    int dutycycle_actor = 5000;
    byte OFF;
    byte ON;

public:
    byte pin_actor = 9; // the number of the LED pin
    String argument_actor;
    byte power_actor;
    bool isOn;
    bool isInverted = false;

    Actor(String pin, String argument, bool inverted);
    void update();
    void change(String pin, String argument, bool inverted);
    void mqtt_subscribe();
    void mqtt_unsubscribe();
    void handlemqtt(char *payload);
};

void handleActors();
void handleRequestActors();
void handleRequestActorConfig();
void handleSetActor();
void handleDelActor();
void handleRequestActorPins();

#endif