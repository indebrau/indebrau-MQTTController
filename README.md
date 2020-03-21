# MQTTDevice

![Overview Image](/img/overview.png)

## General Introduction

### What is it?

The MQTTDevice is an Arduino sketch based on the ESP8266 to enable wireless stable communication between [CraftBeerPi V3](https://github.com/Manuel83/craftbeerpi3) actors and sensors.

### When do I need it?

If you have a CBPi installation to collect and show all information at a central point, but you have sensors and actors all over the place. E.g. the fridge in the basement, the fermenter somewhere in the house and the brewery again someplace else. This device offers a wireless conntection via the MQTT protocol from CBPi to your sensors and actors.

### What does it offer?

* Web interface for configuration
* Sensors
  * PT100/1000 and OneWire sensor support
  * Value is read each second and sent to CBPi
  * Offset calibration
* Actors
  * Choose PIN
  * Used PINS are not shown
  * Inverted GPIO
  * Power percentage: If a value between 0 and 100% is sent, the ESP "pulses" with a duty cycle of 1000ms
* Induction
  * Control of a GGM induction cooker via serial communication

## Wiring setup
* Ports D1 and D2 are currently hardcoded to be used for the display (if you chose to use it), and the display will always show the value of the first sensor readings.
* Display has to have 128x32 px

### Needed libraries (please install)
* OneWire (by Jim Studt..) version 2.3.5
* DallasTemperature (by Miles Burton...) version 3.8.0
* PubSubClient (by Nick O''Leary) version 2.7.0
* ArduinoJson (by Benoit Bianchon) version 5.13.5 (Not 6.X.X!)
* WiFiManager (by tzapu) version 0.15.0
* Adafruit MAX31865 (by Adafruit) version 1.0.3
* Adafruit SSD1306 (by Adafruit) version 2.2.1

[German Tutorial](https://hobbybrauer.de/forum/viewtopic.php?f=58&t=19036&p=309196#p309196)
