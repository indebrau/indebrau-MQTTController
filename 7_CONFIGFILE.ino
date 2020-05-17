bool loadConfig()
{
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile)
  {
    Serial.println("Failed to open config file! (does it exist?)");
    return false;
  }
  else
  {
    Serial.println("Opened config file");
  }

  size_t size = configFile.size();
  if (size > 1024)
  {
    Serial.println("Config file size is too large!");
    return false;
  }

  std::unique_ptr<char[]> buf(new char[size]);

  configFile.readBytes(buf.get(), size);

  StaticJsonDocument<1024> jsonDocument;
  DeserializationError error = deserializeJson(jsonDocument, buf.get());
  if (error)
  {
    Serial.print("Error reading JSON: ");
    Serial.println(error.c_str());
    return false;
  }

  // read Actors
  JsonArray jsonactors = jsonDocument["actors"];
  numberOfActors = jsonactors.size();
  Serial.print("Number of actors loaded: ");
  Serial.println(numberOfActors);

  if (numberOfActors > NUMBER_OF_ACTORS_MAX)
  {
    numberOfActors = NUMBER_OF_ACTORS_MAX;
  }

  for (int i = 0; i < numberOfActors; i++)
  {
    if (i < numberOfActors)
    {
      JsonObject jsonactor = jsonactors[i];
      String pin = jsonactor["PIN"];
      String topic = jsonactor["TOPIC"];
      bool inverted = jsonactor["INVERTED"];
      actors[i].change(pin, topic, inverted);
    }
  }

  // read OneWire sensors
  JsonArray jsonOneWireSensors = jsonDocument["OneWireSensors"];
  numberOfOneWireSensors = jsonOneWireSensors.size();
  if (numberOfOneWireSensors > NUMBER_OF_SENSORS_MAX)
  {
    numberOfOneWireSensors = NUMBER_OF_SENSORS_MAX;
  }
  Serial.print("Number of OneWire sensors loaded: ");
  Serial.println(numberOfOneWireSensors);

  for (int i = 0; i < NUMBER_OF_SENSORS_MAX; i++)
  {
    if (i < numberOfOneWireSensors)
    {
      JsonObject jsonsensor = jsonOneWireSensors[i];
      String address = jsonsensor["ADDRESS"];
      String topic = jsonsensor["TOPIC"];
      float offset = jsonsensor["OFFSET"];

      oneWireSensors[i].change(address, topic, offset);
    }
    else
    {
      oneWireSensors[i].change("", "", 0);
    }
  }

  // read PT100/1000 sensors
  JsonArray jsonPTSensors = jsonDocument["PTSensors"];
  numberOfPTSensors = jsonPTSensors.size();
  if (numberOfPTSensors > NUMBER_OF_SENSORS_MAX)
  {
    numberOfPTSensors = NUMBER_OF_SENSORS_MAX;
  }
  Serial.print("Number of PT100/1000 sensors loaded: ");
  Serial.println(numberOfPTSensors);

  for (int i = 0; i < NUMBER_OF_SENSORS_MAX; i++)
  {
    if (i < numberOfPTSensors)
    {
      JsonObject jsonPTSensor = jsonPTSensors[i];
      String csPin = jsonPTSensor["CSPIN"];
      byte numberOfWires = jsonPTSensor["WIRES"];
      String topic = jsonPTSensor["TOPIC"];
      float offset = jsonPTSensor["OFFSET"];

      ptSensors[i].change(csPin, numberOfWires, topic, offset);
    }
    else
    {
      ptSensors[i].change("", 0, "", 0);
    }
  }

  // Read induction
  JsonObject jsinduction = jsonDocument["induction"];
  bool is_enabled = jsinduction["ENABLED"];
  if (is_enabled)
  {
    String pin_white = jsinduction["PINWHITE"];
    String pin_yellow = jsinduction["PINYELLOW"];
    String pin_blue = jsinduction["PINBLUE"];
    String js_mqtttopic = jsinduction["TOPIC"];
    long delayoff = jsinduction["DELAY"];

    inductionCooker.change(StringToPin(pin_white), StringToPin(pin_yellow), StringToPin(pin_blue), js_mqtttopic, delayoff, is_enabled);
  }

  // read display and distance sensor
  JsonObject jsonI2C = jsonDocument["I2C"];
  useDisplay = jsonI2C["USEDISPLAY"];
  useDistanceSensor = jsonI2C["USEDISTANCESENSOR"];
  if (useDisplay || useDistanceSensor)
  {
    SDAPin = StringToPin(jsonI2C["SDAPIN"]);
    SCLPin = StringToPin(jsonI2C["SCLPIN"]);
    if (useDistanceSensor)
    {
      String mqttTopic = jsonI2C["SENSORTOPIC"];
      distanceSensor.change(mqttTopic);
    }
  }
  Serial.print("Display loaded: ");
  Serial.println(useDisplay);
  Serial.print("Distance sensor loaded: ");
  Serial.println(useDistanceSensor);

  // finally read MQTT host and device name
  String json_mqtthost = jsonDocument["MQTTHOST"];
  json_mqtthost.toCharArray(mqtthost, 16);
  String json_deviceName = jsonDocument["DEVICENAME"];
  json_deviceName.toCharArray(customDeviceName, 10);

  return true;
}

bool saveConfig()
{
  StaticJsonDocument<1024> jsonDocument;

  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile)
  {
    Serial.println("Failed to open config file for writing!");
    return false;
  }

  // write actors
  JsonArray jsactors = jsonDocument.createNestedArray("actors");
  for (int i = 0; i < numberOfActors; i++)
  {
    JsonObject jsactor = jsactors.createNestedObject();
    jsactor["PIN"] = PinToString(actors[i].pin_actor);
    jsactor["TOPIC"] = actors[i].argument_actor;
    jsactor["INVERTED"] = actors[i].isInverted;
  }

  // Write OneWire sensors
  JsonArray jsonOneWireSensors = jsonDocument.createNestedArray("OneWireSensors");
  for (int i = 0; i < numberOfOneWireSensors; i++)
  {
    JsonObject jsonOneWireSensor = jsonOneWireSensors.createNestedObject();
    jsonOneWireSensor["ADDRESS"] = oneWireSensors[i].getSens_address_string();
    jsonOneWireSensor["TOPIC"] = oneWireSensors[i].mqtttopic;
    jsonOneWireSensor["OFFSET"] = oneWireSensors[i].offset;
  }

  // Write PT100/1000 sensors
  JsonArray jsonPTSensors = jsonDocument.createNestedArray("PTSensors");
  for (int i = 0; i < numberOfPTSensors; i++)
  {
    JsonObject jsonPTSensor = jsonPTSensors.createNestedObject();
    jsonPTSensor["CSPIN"] = PinToString(ptSensors[i].csPin);
    jsonPTSensor["WIRES"] = ptSensors[i].numberOfWires;
    jsonPTSensor["TOPIC"] = ptSensors[i].mqttTopic;
    jsonPTSensor["OFFSET"] = ptSensors[i].offset;
  }

  // Write induction
  JsonObject jsinduction = jsonDocument.createNestedObject("induction");
  jsinduction["ENABLED"] = inductionCooker.isEnabled;
  if (inductionCooker.isEnabled)
  {
    jsinduction["PINWHITE"] = PinToString(inductionCooker.PIN_WHITE);
    jsinduction["PINYELLOW"] = PinToString(inductionCooker.PIN_YELLOW);
    jsinduction["PINBLUE"] = PinToString(inductionCooker.PIN_INTERRUPT);
    jsinduction["TOPIC"] = inductionCooker.mqtttopic;
    jsinduction["DELAY"] = inductionCooker.delayAfteroff;
  }

  // Write display and distance sensor
  JsonObject jI2C = jsonDocument.createNestedObject("I2C");
  jI2C["USEDISPLAY"] = useDisplay;
  jI2C["USEDISTANCESENSOR"] = useDistanceSensor;

  if (useDisplay || useDistanceSensor)
  {
    jI2C["SDAPIN"] = PinToString(SDAPin);
    jI2C["SCLPIN"] = PinToString(SCLPin);
    if (useDistanceSensor)
    {
      jI2C["SENSORTOPIC"] = distanceSensor.mqttTopic;
    }
  }

  // finally write MQTT host and device name
  jsonDocument["MQTTHOST"] = mqtthost;
  jsonDocument["DEVICENAME"] = customDeviceName;

  serializeJsonPretty(jsonDocument, configFile);
  return true;
}

/* Needed for the WifiManager */
void saveConfigCallback()
{
  Serial.println("saveConfigCallback: did not do nothing (as intentded)");
}
