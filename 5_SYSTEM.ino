void rebootDevice()
{
  server.send(200, "text/plain", "Rebooting in 1 second...");
  delay(1000);
  ESP.restart();
}

void getNameAndVersion()
{
  StaticJsonDocument<256> jsonDocument;
  jsonDocument["version"] = DEVICE_VERSION;
  jsonDocument["name"] = customDeviceName;
  String response;
  serializeJson(jsonDocument, response);
  server.send(200, "application/json", response);
}

void getMqttStatus()
{
  String returnMessage;
  String mqttAddress(mqtthost);
  if (client.connected())
  {
    returnMessage = "Device successfully subscribed to MQTT Server at " + mqttAddress + ".";
  }
  else
  {
    // not configured
    if (mqttAddress.length() == 0)
    {
      returnMessage = "WARNING, MQTT ADDRESS IS UNDEFINED, PLEASE CONFIGURE DEVICE";
    }
    else
    {
      returnMessage = "WARNING, NOT SUBSCRIBED TO MQTT SERVER AT " + mqttAddress;
    }
  }
  server.send(200, "text/plain", returnMessage);
}

void getOtherPins()
{
  String returnMessage;
  if (numberOfPTSensors > 0)
  {
    returnMessage += "Pins for PT sensors (DI, DO and CLK) are ";
    returnMessage += PinToString(PT_PINS[0]) + ", " + PinToString(PT_PINS[1]) + ", " + PinToString(PT_PINS[2]) + ". ";
  }
  else
  {
    returnMessage += "Not using PT sensors. ";
  }
  if (numberOfOneWireSensors > 0)
  {
    returnMessage += "OneWire bus is on pin " + PinToString(ONE_WIRE_BUS) + ".";
  }
  else
  {
    returnMessage += "Not using OneWire sensors.";
  }
  server.send(200, "text/plain", returnMessage);
}

void getSysConfig()
{
  String mqttAddress(mqtthost);
  StaticJsonDocument<1024> jsonDocument;

  jsonDocument["deviceName"] = customDeviceName;
  jsonDocument["mqttAddress"] = mqttAddress;
  jsonDocument["useDisplay"] = useDisplay;

  // get list of free pins
  String freePins = "";
  for (int i = 0; i < NUMBER_OF_PINS; i++)
  {
    if (pins_used[PINS[i]] == false)
    {
      freePins += F("<option>");
      freePins += PIN_NAMES[i];
      freePins += F("</option>");
    }
    yield();
  }

  String SDAPinMessage = "";
  if (useDisplay || useDistanceSensor)
  {
    SDAPinMessage += F("<option>");
    SDAPinMessage += PinToString(SDAPin);
    SDAPinMessage += F("</option><option disabled>──────────</option>");
  }
  SDAPinMessage += freePins;

  String SCLPinMessage = "";
  if (useDisplay || useDistanceSensor)
  {
    SCLPinMessage += F("<option>");
    SCLPinMessage += PinToString(SCLPin);
    SCLPinMessage += F("</option><option disabled>──────────</option>");
  }
  SCLPinMessage += freePins;

  // add both to json
  jsonDocument["SDAPin"] = SDAPinMessage;
  jsonDocument["SCLPin"] = SCLPinMessage;

  String response;
  serializeJson(jsonDocument, response);
  server.send(200, "application/json", response);
}

void setSysConfig()
{
  String string_deviceName = server.arg(0);
  string_deviceName.toCharArray(customDeviceName, 10);
  String string_mqtthost = server.arg(1);
  string_mqtthost.toCharArray(mqtthost, 16);
  String string_useDisplay = server.arg(2);
  SDAPin = StringToPin(server.arg(3));
  SCLPin = StringToPin(server.arg(4));
  if (string_useDisplay == "true")
  {
    useDisplay = true;
  }
  else
  {
    useDisplay = false;
  }
  saveConfig();
  rebootDevice();
}
