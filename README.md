# Indebrau MQTTDevice

![Overview Image](/img/overview.png)

## Introduction
The Indebrau MQTTDevice is an Arduino sketch based on the ESP8266 to enable stable wireless communication between [CraftBeerPi V3](https://github.com/Manuel83/craftbeerpi3) and actors and sensors, based on the MQTT protocol.
It is based on the "original" MQTTDevice project, started here [here](https://github.com/matschie1/MQTTDevice) and continued here [here](https://github.com/MQTTDevice/MQTTDevice).
The main feature difference is the support of the more accurate (yet a little more expensive) PT100/1000 RTD sensors, using Adafruit's "Temperature Sensor Amplifier MAX31865" chip and library.

## Features
* Sensors
  * PT100/1000 support
  * OneWire support
  * Offset calibration
* Actors
  * Choose PIN
  * Used PINS are not shown
  * Inverted GPIO
  * Power percentage: If a value between 0 and 100% is sent, the ESP "pulses" with a duty cycle of 1000ms
* Induction
  * Control of a GGM induction cooker via serial communication
* Display showing the readings of a sensor (optional)
* Fully configurable (sensors, induction cooker usage, actors, host ip, display usage..) via Web interface

## Wiring, PCBs and cases
(to come, check out the different controller repositories for (currently) undocumented 3d-print case- and pcb layouts)

## Needed libraries (please install)
* OneWire (by Jim Studt..) version 2.3.5
* DallasTemperature (by Miles Burton...) version 3.8.0
* PubSubClient (by Nick O''Leary) version 2.7.0
* ArduinoJson (by Benoit Bianchon) version 5.13.5 (Not 6.X.X!)
* WiFiManager (by tzapu) version 0.15.0
* Adafruit MAX31865 (by Adafruit) version 1.0.3
* Adafruit SSD1306 (by Adafruit) version 2.2.1

## Limitations
* Display has to be 128x32 px
* Display will always show the value of the first sensor readings.

## Setup
[German Tutorial](https://hobbybrauer.de/forum/viewtopic.php?f=58&t=19036&p=309196#p309196)
(more to come)