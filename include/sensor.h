#ifndef SENSOR
#define SENSOR
#include <Arduino.h>
#include <Adafruit_MAX31865.h> // PT100/1000

class OneWireSensor
{
private:
  unsigned long lastCalled;
  float internalValues[3] = {0, 0, 0}; // smoothen signal by averaging the last three values
  byte lastValueIndex = 0;

public:
  char mqttTopic[50];
  byte address[8]; // one wire address
  float value;     // value to be send
  float offset;

  String getSens_address_string();

  OneWireSensor(String, String, float);

  void update();
  void change(String, String, float);
  void publishmqtt();
};

class PTSensor
{
private:
  unsigned long lastCalled;            // timestamp
  float internalValues[3] = {0, 0, 0}; // smoothen signal by averaging the last three values
  byte lastValueIndex = 0;
  Adafruit_MAX31865 maxChip = Adafruit_MAX31865(D1, D4, D3, D0); // initial values (will be overwritten in "constructor")

public:
  byte csPin = D1; // set to new default, otherwise would be "D3" (-> Arduino 0)
  byte numberOfWires;
  char mqttTopic[50];
  float value; // value to be send
  float offset;

  PTSensor(String, byte, String, float);

  void update();
  void change(String, byte, String, float);
  void publishmqtt();
};

class DistanceSensor
{
private:
  unsigned long lastCalled;            // timestamp
  float internalValues[3] = {0, 0, 0}; // smoothen signal by averaging the last three values
  byte lastValueIndex = 0;

public:
  char mqttTopic[50];
  float value; // value to be send in cm

  DistanceSensor(String);

  void update();
  void change(String);
  void publishmqtt();
};

/* Called in loop() */
void handleSensors();

/* Search the OneWire bus for available sensor addresses */
byte searchOneWireSensors();

/* Convert the OneWire sensor address to a (printable) string */
String OneWireAddressToString(byte[8]);

/* Update sensor attributes or create new sensor.*/
void handleSetSensor();

/* Delete a sensor.*/
void handleDelSensor();

/* Provides search results of OneWire bus search */
void handleRequestOneWireSensorAddresses();

/* Similar as the actor pin request function, but this time for PT sensor CS pin */
void handleRequestSensorPins();

/* Returns a JSON array with the current sensor data of all sensors. */
void handleRequestSensors();

/* Returns information for one specific sensor (for update / delete menu in frontend) */
void handleRequestSensorConfig();

#endif