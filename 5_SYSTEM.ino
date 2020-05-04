void rebootDevice()
{
  server.send(200, "text/plain", "Rebooting in 1 second...");
  delay(1000);
  ESP.restart();
}

void getVersion()
{
  server.send(200, "text/plain", DEVICE_VERSION);
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
  if (useDisplay)
  {
    returnMessage = "Using a display. Pins for SDA and SCL are ";
    returnMessage += PinToString(SDAPin) + " and " + PinToString(SCLPin);
    returnMessage += ". Showing the first sensor readings.<br>";
  }
  else
  {
    returnMessage = "Not using a display.<br>";
  }
  if (numberOfPTSensors > 0)
  {
    returnMessage += "Using PT sensors. Pins for  DI, DO and CLK are ";
    returnMessage += PinToString(PT_PINS[0]) + ", " + PinToString(PT_PINS[1]) + ", " + PinToString(PT_PINS[2]) + ". ";
  }
  else
  {
    returnMessage += "Not using PT sensors. ";
  }
  if (numberOfOneWireSensors > 0)
  {
    returnMessage += "Using OneWire sensors. OneWire bus is on pin " + PinToString(ONE_WIRE_BUS) + ".";
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
  StaticJsonBuffer<1024> jsonBuffer;
  JsonObject &config = jsonBuffer.createObject();
  config["mqttAddress"] = mqttAddress;
  config["useDisplay"] = useDisplay;

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
  // first pin message
  String firstPinMessage = "";
  if (useDisplay)
  {
    firstPinMessage += F("<option>");
    firstPinMessage += PinToString(SDAPin);
    firstPinMessage += F("</option><option disabled>──────────</option>");
  }
  firstPinMessage += freePins;
  // second pin message
  String secondPinMessage = "";
  if (useDisplay)
  {
    secondPinMessage += F("<option>");
    secondPinMessage += PinToString(SCLPin);
    secondPinMessage += F("</option><option disabled>──────────</option>");
  }
  secondPinMessage += freePins;

  // now add both to json
  config["firstDisplayPin"] = firstPinMessage;
  config["secondDisplayPin"] = secondPinMessage;

  String response;
  config.printTo(response);
  server.send(200, "application/json", response);
}

void setSysConfig()
{
  String string_mqtthost = server.arg(0);
  string_mqtthost.toCharArray(mqtthost, 16);
  String string_useDisplay = server.arg(1);
  if (string_useDisplay == "true")
  {
    useDisplay = true;
    SDAPin = StringToPin(server.arg(2));
    SCLPin = StringToPin(server.arg(3));
  }
  else
  {
    useDisplay = false;
  }
  saveConfig();
  rebootDevice();
}
