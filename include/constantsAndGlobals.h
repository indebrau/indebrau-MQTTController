#ifndef CONST_AND_GLOBALS
#define CONST_AND_GLOBALS

/*########## INCLUDES ##########*/
#include <Arduino.h>
#include <OneWire.h>           // OneWire communication
#include <DallasTemperature.h> // easier usage of DS18B20 sensors

// display and distance sensor
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_SSD1306.h>

#include <Adafruit_VL53L0X.h> // distance sensor

#include <Adafruit_MAX31865.h> // PT100/1000

#include <ESP8266WiFi.h>      // general WiFi functionality
#include <ESP8266WebServer.h> // Webserver support
#include <WiFiManager.h>      // for configuring in access point mode
#include <DNSServer.h>        // needed for WiFiManager
#include <PubSubClient.h>     // MQTT
#include <EEPROM.h>           // stores the config file

#include <LittleFS.h>
#include <ArduinoJson.h>

// OTA
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// Actors and Sensors
#include "actor.h"
#include "sensor.h"
#include "induction.h"

/*########## CONSTANTS #########*/
extern const String DEVICE_VERSION;

extern const byte ONE_WIRE_BUS; // default PIN for initialization, in the future to be configured via Web frontend

/*
 * Common pins across all PT100/1000 sensors: DI, DO, CLK.
 * When using multiple sensors, reuse these pins and only (re)define
 * the CS PIN, meaning you need one additional pin per sensor.
*/
extern const byte PT_PINS[3];

extern const byte ONE_WIRE_RESOLUTION; // ranges betwee 9 and 12, higher is better (and slower)
extern const float RREF;               // 430.0 for PT100 and 4300.0 for PT1000
extern const float RNOMINAL;           // 100.0 for PT100, 1000.0 for PT1000

// WiFi and MQTT
extern const int WEB_SERVER_PORT;
extern const int TELNET_SERVER_PORT;
extern const int MQTT_SERVER_PORT;
extern const byte ACCESS_POINT_MODE_TIMEOUT;     // in seconds, device restarts if no one connected to AP during this time
extern const char AP_PASSPHRASE[16];             // passphrase to access the Wifi access point
extern const int UPDATE;                         // how often should the system update state ("call all routines")
extern const int DEFAULT_SENSOR_UPDATE_INTERVAL; // how often should sensors provide new readings (>= UPDATE)

// display
extern const byte SCREEN_WIDTH;  // display width, in pixels
extern const byte SCREEN_HEIGHT; // display height, in pixels
extern Adafruit_SSD1306 display;

// differentiate between the three currently supported sensor types
extern const String SENSOR_TYPE_ONE_WIRE;
extern const String SENSOR_TYPE_PT;
extern const String SENSOR_TYPE_DISTANCE;

//  MQTT reconnect settings
extern const byte MQTT_CONNECT_DELAY_SECONDS;

extern const byte NUMBER_OF_PINS; // the "configurable" pins
extern const byte PINS[];
extern const String PIN_NAMES[];

extern const byte NUMBER_OF_SENSORS_MAX; // max number of sensors per type (if changed, please init accordingly!)
extern const byte NUMBER_OF_ACTORS_MAX;  // max number of actors (if changed, please init accordingly!)

// induction cooker signal timings
extern const int SIGNAL_HIGH;
extern const int SIGNAL_LOW;
extern const byte SIGNAL_START;
extern const byte SIGNAL_WAIT;

// percentage steps (induction cooker)
extern const byte PWR_STEPS[];

// signals for induction cooker
extern const int CMD[6][33];

/*########## GLOBAL VARIABLES #########*/
extern ESP8266WebServer server;
extern WiFiManager wifiManager;
extern WiFiClient espClient;
extern PubSubClient client;
extern WiFiServer TelnetServer; // OTA

// careful here, these are not only the Wemos-numbered GIPO (D0-D8), but all of them (see also Pinmap)
extern bool pins_used[17];

extern byte numberOfOneWireSensors; // current number of OneWire sensors
extern byte numberOfPTSensors;      // current number of PT100 sensors
extern byte numberOfActors;         // current number of actors

extern OneWire oneWire;
extern DallasTemperature DS18B20;
extern byte oneWireAddressesFound[][8];
extern byte numberOfOneWireSensorsFound; // OneWire sensors found on the bus

// if display and/or Distance Sensor is used, two pins are occupied (configured in Web frontend)
extern bool useDisplay;
extern bool useDistanceSensor;
extern Adafruit_VL53L0X distanceSensorChip;
extern byte SDAPin;
extern byte SCLPin;

extern char mqtthost[16]; // mqtt server ip
extern long mqttconnectlasttry;
extern char customDeviceName[10]; // name that can be given to the device in the frontend
extern char deviceName[30];       // complete device name, also the name this device will use to register at the mqtt server

extern unsigned long lastToggled; // last system update timestamp
extern Actor actors[];
extern OneWireSensor oneWireSensors[];
extern PTSensor ptSensors[];
extern DistanceSensor distanceSensor;
extern Induction inductionCooker;

#endif