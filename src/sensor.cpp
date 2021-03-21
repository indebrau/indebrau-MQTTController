/*
  Sensor classes and related functions.
  Currently supports OneWire and PT100/1000 sensors.
*/
#include "sensor.h"
#include "constantsAndGlobals.h"
#include "utils.h"
#include "system.h"
#include "config.h"

String OneWireSensor::getSens_address_string()
{
  return OneWireAddressToString(address);
}

OneWireSensor::OneWireSensor(String address, String mqttTopic, float offset)
{
  change(address, mqttTopic, offset);
}

void OneWireSensor::update()
{
  if (millis() > (lastCalled + DEFAULT_SENSOR_UPDATE_INTERVAL))
  {

    DS18B20.requestTemperatures();
    float currentValue = DS18B20.getTempC(address);

    if (currentValue == -127.0)
    {
      Serial.print("OneWire Sensor ");
      Serial.print(mqttTopic);
      Serial.println(" not found!");
    }
    else
    {
      if (currentValue == 85.0)
      {
        Serial.print("One Wire Sensor ");
        Serial.print(mqttTopic);
        Serial.println(" Error!");
      }
      else
      {
        lastValueIndex = (lastValueIndex + 1) % 3;
        internalValues[lastValueIndex] = currentValue + offset;                  // after error checking, add offset
        value = (internalValues[0] + internalValues[1] + internalValues[2]) / 3; // smoothen signal
        publishmqtt();                                                           // and send
      }
    }

    lastCalled = millis();
  }
}

void OneWireSensor::change(String new_address, String new_mqtttopic, float new_offset)
{
  new_mqtttopic.toCharArray(mqttTopic, new_mqtttopic.length() + 1);
  offset = new_offset;
  // if this check fails, this could also mean a call from array init
  // (no actual sensor defined for this array entry, so skip init here)
  if (new_address.length() == 16)
  {
    char address_char[16];

    new_address.toCharArray(address_char, 17);

    char hexbyte[2];
    int octets[8];

    for (int d = 0; d < 16; d += 2)
    {
      // Assemble a digit pair into the hexbyte string
      hexbyte[0] = address_char[d];
      hexbyte[1] = address_char[d + 1];

      // Convert the hex pair to an integer
      sscanf(hexbyte, "%x", &octets[d / 2]);
      yield();
    }
    Serial.print("Starting OneWire sensor: ");
    for (int i = 0; i < 8; i++)
    {
      address[i] = octets[i];
      Serial.print(address[i], HEX);
    }
    Serial.println("");
    DS18B20.setResolution(address, ONE_WIRE_RESOLUTION);
  }
}

void OneWireSensor::publishmqtt()
{
  if (client.connected())
  {
    char buf[8];
    dtostrf(value, 3, 2, buf);
    client.publish(mqttTopic, buf);
  }
}

PTSensor::PTSensor(String csPin, byte numberOfWires, String mqtttopic, float offset)
{
  change(csPin, numberOfWires, mqtttopic, offset);
}

void PTSensor::update()
{
  if (millis() > (lastCalled + DEFAULT_SENSOR_UPDATE_INTERVAL))
  {
    float currentValue = maxChip.temperature(RNOMINAL, RREF);

    // sensor reads very low or high temps if disconnected
    if (maxChip.readFault() || currentValue < -100 || currentValue > 110)
    {
      currentValue = -127.0;
      maxChip.clearFault();
    }
    else
    {
      lastValueIndex = (lastValueIndex + 1) % 3;
      internalValues[lastValueIndex] = currentValue + offset; // after error checking, add offset
      value = (internalValues[0] + internalValues[1] + internalValues[2]) / 3;
      publishmqtt(); // and send
    }
    lastCalled = millis();
  }
}

void PTSensor::change(String newCSPin, byte newNumberOfWires, String newMqttTopic, float newOffset)
{
  // check for initial empty array entry initialization
  // (no actual sensor defined in this call)
  if (newCSPin != "")
  {
    byte byteNewCSPin = StringToPin(newCSPin);
    // if this pin fits the bill, go for it..
    if (isPin(byteNewCSPin))
    {
      pins_used[csPin] = false;
      csPin = byteNewCSPin;
      pins_used[csPin] = true;
      newMqttTopic.toCharArray(mqttTopic, newMqttTopic.length() + 1);
      numberOfWires = newNumberOfWires;
      offset = newOffset;
      maxChip = Adafruit_MAX31865(csPin, PT_PINS[0], PT_PINS[1], PT_PINS[2]);
      Serial.print("Starting PT sensor with ");
      Serial.print(newNumberOfWires);
      Serial.print(" wires. CS Pin is ");
      Serial.println(PinToString(csPin));
      if (newNumberOfWires == 4)
      {
        maxChip.begin(MAX31865_4WIRE);
      }
      else if (newNumberOfWires == 3)
      {
        maxChip.begin(MAX31865_3WIRE);
      }
      else
      {
        maxChip.begin(MAX31865_2WIRE); // set to 2 wires as default
      }
    }
  }
}

void PTSensor::publishmqtt()
{
  if (client.connected())
  {
    char buf[8];
    dtostrf(value, 3, 2, buf);
    client.publish(mqttTopic, buf);
  }
}

DistanceSensor::DistanceSensor(String mqttTopic)
{
  Serial.println("Init sensor " + mqttTopic);
  change(mqttTopic);
}

void DistanceSensor::update()
{
  if (millis() > (lastCalled + DEFAULT_SENSOR_UPDATE_INTERVAL))
  {
    VL53L0X_RangingMeasurementData_t measure;
    distanceSensorChip.rangingTest(&measure, false);
    // Range between 20 and 800 millimeter is more or less accurate
    if (measure.RangeStatus != 4 && measure.RangeMilliMeter < 800 && measure.RangeMilliMeter > 20)
    {

      lastValueIndex = (lastValueIndex + 1) % 3;
      internalValues[lastValueIndex] = measure.RangeMilliMeter / 10.0;
      value = (internalValues[0] + internalValues[1] + internalValues[2]) / 3;
      publishmqtt();
    }
    else
    {
      value = -127.0;
    }
    lastCalled = millis();
  }
}

void DistanceSensor::change(String newMqttTopic)
{
  // check for non-empty topic
  if (newMqttTopic != "")
  {
    newMqttTopic.toCharArray(mqttTopic, newMqttTopic.length() + 1);
    Serial.print("Starting distance sensor with topic ");
    Serial.println(newMqttTopic);
  }
}

void DistanceSensor::publishmqtt()
{
  if (client.connected())
  {
    char buf[8];
    dtostrf(value, 3, 2, buf);
    client.publish(mqttTopic, buf);
  }
}

/* Called in loop() */
void handleSensors()
{
  for (int i = 0; i < numberOfOneWireSensors; i++)
  {
    oneWireSensors[i].update();
    yield();
  }
  for (int i = 0; i < numberOfPTSensors; i++)
  {
    ptSensors[i].update();
    yield();
  }
  if (useDistanceSensor)
  {
    distanceSensor.update();
  }
}

/* Search the OneWire bus for available sensor addresses */
byte searchOneWireSensors()
{
  byte n = 0;
  byte addr[8];
  while (oneWire.search(addr))
  {
    if (OneWire::crc8(addr, 7) == addr[7])
    {
      Serial.print("Sensor found: ");
      for (int i = 0; i < 8; i++)
      {
        oneWireAddressesFound[n][i] = addr[i];
        Serial.print(addr[i], HEX);
      }
      Serial.println("");
      n += 1;
    }
    yield();
  }
  oneWire.reset_search();
  return n;
}

/* Convert the OneWire sensor address to a (printable) string */
String OneWireAddressToString(byte addr[8])
{
  char charbuffer[50];
  sprintf(charbuffer, "%02x%02x%02x%02x%02x%02x%02x%02x", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);
  return charbuffer;
}

/* All following fuctions called from frontend (mapping in 0_Setup.ino) */

/* Update sensor attributes or create new sensor.*/
void handleSetSensor()
{
  String type = server.arg(0);
  String newTopic = server.arg(1);

  if (type == SENSOR_TYPE_ONE_WIRE)
  {
    int id = server.arg(2).toInt();
    // means: create new sensor request
    if (id == -1)
    {
      id = numberOfOneWireSensors;
      numberOfOneWireSensors += 1;
      // first sensor, block OneWire sensor pins
      if (numberOfOneWireSensors == 1)
      {
        pins_used[ONE_WIRE_BUS] = true;
      }
    }

    String newAddress = server.arg(3);
    float newOffset = server.arg(4).toFloat();
    oneWireSensors[id].change(newAddress, newTopic, newOffset);
  }
  else if (type == SENSOR_TYPE_PT)
  {
    int id = server.arg(2).toInt();
    // means: create new sensor request
    if (id == -1)
    {
      id = numberOfPTSensors;
      numberOfPTSensors += 1;
      // first sensor, block PT sensor pins
      if (numberOfPTSensors == 1)
      {
        pins_used[PT_PINS[0]] = true;
        pins_used[PT_PINS[1]] = true;
        pins_used[PT_PINS[2]] = true;
      }
    }
    String newCsPin = server.arg(3);
    byte newNumberOfWires = server.arg(4).toInt();
    float newOffset = server.arg(5).toFloat();
    ptSensors[id].change(newCsPin, newNumberOfWires, newTopic, newOffset);
  }
  else if (type == SENSOR_TYPE_DISTANCE)
  {
    distanceSensor.change(newTopic);
    if (!useDistanceSensor)
    { // if sensor did not run yet, restart needed
      useDistanceSensor = true;
      saveConfig();
      rebootDevice();
    }
  }
  else
  {
    server.send(400, "text/plain", "Unknown Sensor Type");
    return;
  }
  // all done, save and exit
  saveConfig();
  server.send(201, "text/plain", "created");
}

/* Delete a sensor.*/
void handleDelSensor()
{
  int id = server.arg(0).toInt();
  String type = server.arg(1);
  // OneWire
  if (type == SENSOR_TYPE_ONE_WIRE)
  {
    // move all sensors following the given id one to the front of array,
    // effectively overwriting the sensor to be deleted..
    for (int i = id; i < numberOfOneWireSensors; i++)
    {
      oneWireSensors[i].change(oneWireSensors[i + 1].getSens_address_string(), oneWireSensors[i + 1].mqttTopic, oneWireSensors[i + 1].offset);
      yield();
    }
    // ..and declare the array's content to one sensor less
    numberOfOneWireSensors -= 1;
    // last sensor removed, free pin usage
    if (numberOfOneWireSensors == 0)
    {
      pins_used[ONE_WIRE_BUS] = false;
    }
  }
  else if (type == SENSOR_TYPE_PT)
  {
    // first declare the pin unused
    pins_used[ptSensors[id].csPin] = false;
    // move all sensors following the given id one to the front of array,
    // effectively overwriting the sensor to be deleted..
    for (int i = id; i < numberOfPTSensors; i++)
    {
      String csPinString = String(ptSensors[i + 1].csPin); // yeah, not very nice or efficient..
      ptSensors[i].change(csPinString, ptSensors[i + 1].numberOfWires, ptSensors[i + 1].mqttTopic, ptSensors[i + 1].offset);
      yield();
    }
    // ..and declare the array's content to one sensor less
    numberOfPTSensors -= 1;
    // last sensor removed, free pin usages
    if (numberOfPTSensors == 0)
    {
      pins_used[PT_PINS[0]] = false;
      pins_used[PT_PINS[1]] = false;
      pins_used[PT_PINS[2]] = false;
    }
  }
  else if (type == SENSOR_TYPE_DISTANCE)
  {
    useDistanceSensor = false;
    // if also display is not used, reboot to free i2c pins
    if (!useDisplay)
    {
      saveConfig();
      rebootDevice();
    }
  }
  else
  {
    server.send(400, "text/plain", "Unknown sensor type");
    return;
  }
  // all done, save and exit
  saveConfig();
  server.send(200, "text/plain", "deleted");
}

/* Provides search results of OneWire bus search */
void handleRequestOneWireSensorAddresses()
{
  numberOfOneWireSensorsFound = searchOneWireSensors();
  int id = server.arg(0).toInt();
  String message = "";
  // if id given, render this sensor's address first
  // and check if id is valid (client could send nonsense for id..)
  if (id != -1 && id < numberOfOneWireSensors)
  {
    message += F("<option>");
    message += oneWireSensors[id].getSens_address_string();
    message += F("</option><option disabled>──────────</option>");
  }
  // Now render all found addresses, except the one already assigned to the sensor
  for (int i = 0; i < numberOfOneWireSensorsFound; i++)
  {
    String foundAddress = OneWireAddressToString(oneWireAddressesFound[i]);
    if (id == -1 || !(oneWireSensors[id].getSens_address_string() == foundAddress))
    {
      message += F("<option>");
      message += foundAddress;
      message += F("</option>");
    }
    yield();
  }
  server.send(200, "text/html", message);
}

/* Similar as the actor pin request function, but this time for PT sensor CS pin */
void handleRequestSensorPins()
{
  int id = server.arg(0).toInt();
  String message;

  if (id != -1) // given pin
  {
    message += F("<option>");
    if (server.arg(1) == SENSOR_TYPE_PT)
    {
      message += PinToString(ptSensors[id].csPin);
      message += F("</option><option disabled>──────────</option>");
    }
  }
  // now add all free pins
  for (int i = 0; i < NUMBER_OF_PINS; i++)
  {
    if (pins_used[PINS[i]] == false)
    {
      message += F("<option>");
      message += PIN_NAMES[i];
      message += F("</option>");
    }
    yield();
  }
  server.send(200, "text/plain", message);
}

/*
  Returns a JSON array with the current sensor data of all sensors.
*/
void handleRequestSensors()
{
  StaticJsonDocument<1024> jsonDocument;
  jsonDocument.to<JsonArray>(); // needed to prevent "null" responses

  for (int i = 0; i < numberOfOneWireSensors; i++)
  {
    JsonObject sensorResponse = jsonDocument.createNestedObject();
    if ((oneWireSensors[i].value != -127.0) && (oneWireSensors[i].value != 85.0))
    {
      char buf[8];
      dtostrf(oneWireSensors[i].value, 3, 2, buf);
      sensorResponse["value"] = buf;
    }
    else
    {
      sensorResponse["value"] = "ERR";
    }
    sensorResponse["mqtt"] = oneWireSensors[i].mqttTopic;
    sensorResponse["type"] = SENSOR_TYPE_ONE_WIRE;
    sensorResponse["id"] = i;
    sensorResponse["offset"] = oneWireSensors[i].offset;
    yield();
  }
  for (int i = 0; i < numberOfPTSensors; i++)
  {
    JsonObject sensorResponse = jsonDocument.createNestedObject();
    // While it is technically possible to read this low value with a PT sensor
    // We reuse "OneWire Error Codes" here for simplicity
    if (ptSensors[i].value != -127.0)
    {
      char buf[8];
      dtostrf(ptSensors[i].value, 3, 2, buf);
      sensorResponse["value"] = buf;
    }
    else
    {
      sensorResponse["value"] = "ERR";
    }
    sensorResponse["mqtt"] = ptSensors[i].mqttTopic;
    sensorResponse["type"] = SENSOR_TYPE_PT;
    sensorResponse["id"] = i;
    sensorResponse["offset"] = ptSensors[i].offset;
    yield();
  }
  if (useDistanceSensor)
  {
    JsonObject sensorResponse = jsonDocument.createNestedObject();
    // We reuse "OneWire Error Codes" here for simplicity
    if (distanceSensor.value != -127.0)
    {
      char buf[8];
      dtostrf(distanceSensor.value, 3, 2, buf);
      sensorResponse["value"] = buf;
    }
    else
    {
      sensorResponse["value"] = "ERR";
    }
    sensorResponse["mqtt"] = distanceSensor.mqttTopic;
    sensorResponse["type"] = SENSOR_TYPE_DISTANCE;
    yield();
  }
  String response;
  serializeJson(jsonDocument, response);
  server.send(200, "application/json", response);
}

/* Returns information for one specific sensor (for update / delete menu in frontend) */
void handleRequestSensorConfig()
{
  int id = server.arg(0).toInt();
  String type = server.arg(1);
  String request = server.arg(2);
  String response;

  if (type == SENSOR_TYPE_ONE_WIRE)
  {
    if (id == -1 || id > numberOfOneWireSensors)
    {
      response = "not found";
      server.send(404, "text/plain", response);
    }
    else
    {
      StaticJsonDocument<1024> jsonDocument;
      jsonDocument["topic"] = oneWireSensors[id].mqttTopic;
      jsonDocument["offset"] = oneWireSensors[id].offset;
      serializeJson(jsonDocument, response);
      server.send(200, "application/json", response);
    }
  }
  else if (type == SENSOR_TYPE_PT)
  {
    if (id == -1 || id > numberOfPTSensors)
    {
      response = "not found";
      server.send(404, "text/plain", response);
    }
    else
    {
      StaticJsonDocument<1024> jsonDocument;
      jsonDocument["topic"] = ptSensors[id].mqttTopic;
      jsonDocument["csPin"] = PinToString(ptSensors[id].csPin);
      jsonDocument["numberOfWires"] = ptSensors[id].numberOfWires;
      jsonDocument["offset"] = ptSensors[id].offset;
      serializeJson(jsonDocument, response);
      server.send(200, "application/json", response);
    }
  }
  else if (type == SENSOR_TYPE_DISTANCE)
  {
    if (!useDistanceSensor)
    {
      response = "not found";
      server.send(404, "text/plain", response);
    }
    else
    {
      StaticJsonDocument<128> jsonDocument;
      jsonDocument["topic"] = distanceSensor.mqttTopic;
      serializeJson(jsonDocument, response);
      server.send(200, "application/json", response);
    }
  }
  else
  {
    response = "unknown type: ";
    response += type;
    server.send(406, "text/plain", response);
  }
}
