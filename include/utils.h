#ifndef UTILS
#define UTILS
#include <Arduino.h>

byte StringToPin(String);
String PinToString(byte);
bool isPin(byte);
byte convertCharToHex(char);

#endif