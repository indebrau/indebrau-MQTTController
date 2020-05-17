/*
 *
 * Sketch for ESP8266
 *
 * MQTT communication with CraftBeerPi v3
 *
 * Currently supports:
 * - Control of GGM Induction cooker IDS2 (emulates original control device)
 * - DS18B20 sensors
 * - PT100/1000 sensors (using the Adafruit MAX31865 amplifier and library)
 * - Distance sensor (using Adafruit VL53L0X library, useful for measuring fill levels)
 * - GIPO controlled actors with emulated PWM
 * - OverTheAir firmware updates
 *
*/

/*########## INCLUDES ##########*/
#include <OneWire.h>           // OneWire communication
#include <DallasTemperature.h> // easier usage of DS18B20 sensors

// display and distance sensor
#include <SPI.h>
#include <Wire.h>
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

/*########## CONSTANTS #########*/
const String DEVICE_VERSION = "v1.2.1 (17.05.2020)";

const byte ONE_WIRE_BUS = D3; // default PIN for initialization, in the future to be configured via Web frontend

/*
 * Common pins across all PT100/1000 sensors: DI, DO, CLK.
 * When using multiple sensors, reuse these pins and only (re)define
 * the CS PIN, meaning you need one additional pin per sensor.
*/
const byte PT_PINS[3] = {D4, D3, D0};

const byte ONE_WIRE_RESOLUTION = 11; // ranges betwee 9 and 12, higher is better (and slower)
const float RREF = 430.0;            // 430.0 for PT100 and 4300.0 for PT1000
const float RNOMINAL = 100.0;        // 100.0 for PT100, 1000.0 for PT1000
const byte DEFAULT_CS_PIN = D1;      // default pin for the CS of a PT sensor (placeholder)

// WiFi and MQTT
const int WEB_SERVER_PORT = 80;
const int TELNET_SERVER_PORT = 8266;
const int MQTT_SERVER_PORT = 1883;
const byte ACCESS_POINT_MODE_TIMEOUT = 20;       // in seconds, device restarts if no one connected to AP during this time
const char AP_PASSPHRASE[16] = "indebrau";       // passphrase to access the Wifi access point
const int UPDATE = 500;                          // how often should the system update state ("call all routines")
const int DEFAULT_SENSOR_UPDATE_INTERVAL = 1000; // how often should sensors provide new readings (>= UPDATE)

// display
const byte SCREEN_WIDTH = 128; // display width, in pixels
const byte SCREEN_HEIGHT = 32; // display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// differentiate between the three currently supported sensor types
const String SENSOR_TYPE_ONE_WIRE = "OneWire";
const String SENSOR_TYPE_PT = "PTSensor";
const String SENSOR_TYPE_DISTANCE = "Distance";

//  MQTT reconnect settings
const byte MQTT_CONNECT_DELAY_SECONDS = 10;
const byte MQTT_NUMBER_OF_TRIES = 1;

const byte NUMBER_OF_PINS = 9; // the "configurable" pins
const byte PINS[NUMBER_OF_PINS] = {D0, D1, D2, D3, D4, D5, D6, D7, D8};
const String PIN_NAMES[NUMBER_OF_PINS] = {"D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8"};

const byte NUMBER_OF_SENSORS_MAX = 6; // max number of sensors per type (if changed, please init accordingly!)
const byte NUMBER_OF_ACTORS_MAX = 6;  // max number of actors (if changed, please init accordingly!)

// induction cooker signal timings
const int SIGNAL_HIGH = 5120;
const int SIGNAL_LOW = 1280;
const byte SIGNAL_START = 25;
const byte SIGNAL_WAIT = 10;

// percentage steps (induction cooker)
const byte PWR_STEPS[] = {0, 20, 40, 60, 80, 100};

// signals for induction cooker
const int CMD[6][33] = {
    {SIGNAL_HIGH, SIGNAL_LOW, SIGNAL_HIGH, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW,
     SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW,
     SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW,
     SIGNAL_HIGH, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW}, // Off
    {SIGNAL_HIGH, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_HIGH, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW,
     SIGNAL_HIGH, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW,
     SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_HIGH, SIGNAL_LOW,
     SIGNAL_HIGH, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW}, // PSIGNAL_HIGH
    {SIGNAL_HIGH, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_HIGH, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW,
     SIGNAL_LOW, SIGNAL_HIGH, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW,
     SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_HIGH,
     SIGNAL_HIGH, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW}, // P2
    {SIGNAL_HIGH, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_HIGH, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW,
     SIGNAL_HIGH, SIGNAL_HIGH, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW,
     SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_HIGH, SIGNAL_HIGH,
     SIGNAL_HIGH, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW}, // P3
    {SIGNAL_HIGH, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_HIGH, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW,
     SIGNAL_LOW, SIGNAL_LOW, SIGNAL_HIGH, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW,
     SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW,
     SIGNAL_LOW, SIGNAL_HIGH, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW}, // P4
    {SIGNAL_HIGH, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_HIGH, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW,
     SIGNAL_HIGH, SIGNAL_LOW, SIGNAL_HIGH, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW,
     SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_HIGH, SIGNAL_LOW,
     SIGNAL_LOW, SIGNAL_HIGH, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW, SIGNAL_LOW} // P5
};

/*########## GLOBAL VARIABLES #########*/
ESP8266WebServer server(WEB_SERVER_PORT);
WiFiManager wifiManager;
WiFiClient espClient;
PubSubClient client(espClient);
WiFiServer TelnetServer(TELNET_SERVER_PORT); // OTA

// careful here, these are not only the Wemos-numbered GIPO (D0-D8), but all of them (see also Pinmap)
bool pins_used[17];

byte numberOfOneWireSensors = 0; // current number of OneWire sensors
byte numberOfPTSensors = 0;      // current number of PT100 sensors
byte numberOfActors = 0;         // current number of actors

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
byte oneWireAddressesFound[NUMBER_OF_SENSORS_MAX][8];
byte numberOfOneWireSensorsFound = 0; // OneWire sensors found on the bus

// if display and/or Distance Sensor is used, two pins are occupied (configured in Web frontend)
bool useDisplay = false;
bool useDistanceSensor = false;
Adafruit_VL53L0X distanceSensorChip = Adafruit_VL53L0X();
byte SDAPin;
byte SCLPin;

char mqtthost[16] = ""; // mqtt server ip
long mqttconnectlasttry;
char customDeviceName[10] = ""; // name that can be given to the device in the frontend
char deviceName[30];            // complete device name, also the name this device will use to register at the mqtt server

unsigned long lastToggled = 0; // last system update timestamp
