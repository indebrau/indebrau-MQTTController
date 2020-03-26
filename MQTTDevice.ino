/*
 * Sketch for ESP8266
 *
 * MQTT communication with CraftBeerPi v3
 *
 * Currently supports:
 * - Control of GGM Induction cooker IDS2 (emulates original control device)
 * - DS18B20 sensors
 * - PT100/1000 sensors (using the Adafruit max31865 amplifier and library)
 * - GIPO controlled actors with emulated PWM
 * - OverTheAir firmware updates
 *
*/

/*########## INCLUDES ##########*/
#include <OneWire.h> // OneWire communication
#include <DallasTemperature.h> // Easier usage of DS18B20 sensors

// Display
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#include <Adafruit_MAX31865.h> // PT100/1000

#include <ESP8266WiFi.h> // General WiFi functionality
#include <ESP8266WebServer.h> // Webserver support
#include <WiFiManager.h> // For configuring in access point mode
#include <DNSServer.h> // Needed for WiFiManager
#include <PubSubClient.h> // MQTT
#include <EEPROM.h> // Stores the config file


#include <FS.h> // SPIFFS access
#include <ArduinoJson.h> // JSON support

// OTA
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

/*########## CONSTANTS #########*/
const String DEVICE_VERSION = "v1.0.4 (26.03.2020)";

// Default PINS (for initialization, can be configured via Web frontend, see also 99_PINMAP_WEMOS_D1Mini)
const byte ONE_WIRE_BUS = D8;
/*
 * Common pins across all PT100/1000 sensors: DI, DO, CLK.
 * When using multiple sensors, reuse these pins and only (re)define
 * the CS PIN, meaning you need one additional pin per sensor.
*/
const byte PT_PINS[3] = {D4, D3, D0};

// Ranges from 9 to 12, higher is better (and slower!)
const byte ONE_WIRE_RESOLUTION = 10;
// 430.0 for PT100 and 4300.0 for PT1000
const float RREF = 430.0;
// 100.0 for PT100, 1000.0 for PT1000
const float RNOMINAL = 100.0;
// Default pin for the CS of a PT sensor (placeholder)
const byte DEFAULT_CS_PIN = D1;

// WiFi and MQTT
const int WEB_SERVER_PORT = 80;
const int TELNET_SERVER_PORT = 8266;
const int MQTT_SERVER_PORT = 1883;
const byte ACCESS_POINT_MODE_TIMEOUT = 20; // In seconds, device restarts if no one connected to AP during this time
const char AP_PASSPHRASE[16] = "indebrau"; // Passphrase to access the Wifi access point
const int UPDATE = 500; // How often should the system update state ("call all routines")
const int DEFAULT_SENSOR_UPDATE_INTERVAL = 2000; // how often should sensors provide new readings (>= UPDATE)

// Display
const byte SCREEN_WIDTH = 128; // Display width, in pixels
const byte SCREEN_HEIGHT = 32;// Display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Differentiate between the two currently supported sensor types
const String SENSOR_TYPE_ONE_WIRE = "OneWire";
const String SENSOR_TYPE_PT = "PTSensor";

// MQTT reconnect settings
const long MQTT_CONNECT_DELAY = 10000;
const byte MQTT_NUMBER_OF_TRIES = 1;

// Induction cooker signal timings
const int SIGNAL_HIGH = 5120;
const int SIGNAL_HIGH_TOL = 1500;
const int SIGNAL_LOW = 1280;
const int SIGNAL_LOW_TOL = 500;
const int SIGNAL_START = 25;
const int SIGNAL_START_TOL = 10;
const int SIGNAL_WAIT = 10;
const int SIGNAL_WAIT_TOL = 5;

// Percentage steps (induction cooker)
const byte PWR_STEPS[] = {0, 20, 40, 60, 80, 100};

// Error messages of the induction cooker
const String ERROR_MESSAGES[10] = {"E0", "E1", "E2", "E3", "E4", "E5", "E6", "E7", "E8", "EC"};

const byte NUMBER_OF_PINS = 9; // the "configurable" pins
const byte PINS[NUMBER_OF_PINS] = {D0, D1, D2, D3, D4, D5, D6, D7, D8};
const String PIN_NAMES[NUMBER_OF_PINS] = {"D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8"};

const byte NUMBER_OF_SENSORS_MAX = 6; // max number of sensors per type (if changed, please init accordingly!)
const byte NUMBER_OF_ACTORS_MAX = 6; // max number of actors

/*########## GLOBAL VARIABLES #########*/

ESP8266WebServer server(WEB_SERVER_PORT);
WiFiManager wifiManager;
WiFiClient espClient;
PubSubClient client(espClient);
WiFiServer TelnetServer(TELNET_SERVER_PORT); // OTA

// Init binary signals for induction cooker (not constants, changing!)
int CMD[6][33] = {
  {1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0}, // Off
  {1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0}, // P1
  {1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0}, // P2
  {1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0}, // P3
  {1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0}, // P4
  {1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0}  // P5
};

bool pins_used[17]; // careful here, these are not only the Wemos-numbered GIPO (D0-D8), but all of them!

byte numberOfPTSensors = 0; // Current number of PT100 sensors
byte numberOfOneWireSensors = 0; // Current number of OneWire sensors
byte numberOfActors = 0; // Current number of actors

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
byte oneWireAddressesFound[NUMBER_OF_SENSORS_MAX][8];
byte numberOfOneWireSensorsFound = 0; // OneWire sensors found on the bus

// if display is used, two pins are occupied (configured in Web frontend)
bool useDisplay = false;
byte firstDisplayPin;
byte secondDisplayPin;

char mqtthost[16] = ""; // mqtt server ip
long mqttconnectlasttry;
char deviceName[25]; // device name, also the name this device will use to register at the mqtt server

// last system update timestamp
unsigned long lastToggled = 0;
