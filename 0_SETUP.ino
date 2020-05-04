void setup()
{
  Serial.begin(115200);
  snprintf(deviceName, 25, "MQTTDevice-%08X", ESP.getChipId()); // Set device name

  // load SPIF file system
  ESP.wdtFeed();
  if (!SPIFFS.begin())
  {
    Serial.println("SPIFFS Mount failed");
  }

  // load settings from config.json
  ESP.wdtFeed();
  loadConfig();

  // declare OneWire and PT Pins as used
  if (numberOfOneWireSensors > 0)
  {
    pins_used[ONE_WIRE_BUS] = true;
    DS18B20.begin();  // start OneWire sensors
  }
  if (numberOfPTSensors > 0)
  {
    pins_used[PT_PINS[0]] = true;
    pins_used[PT_PINS[1]] = true;
    pins_used[PT_PINS[2]] = true;
  }


  // WiFi Manager
  ESP.wdtFeed();
  WiFi.hostname(deviceName);
  wifiManager.setTimeout(ACCESS_POINT_MODE_TIMEOUT);
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  if (!wifiManager.autoConnect(deviceName, AP_PASSPHRASE))
  {
    Serial.println("Connection not possible, timeout, restart!");
    rebootDevice();
  }

  // activate ArduinoOTA
  ESP.wdtFeed();
  setupOTA();

  // start mqtt
  client.setServer(mqtthost, MQTT_SERVER_PORT);
  client.setCallback(mqttcallback);

  // start webserver
  ESP.wdtFeed();
  setupServer();

  // start display if applicable
  if (useDisplay || useDistanceSensor)
  {
    pins_used[SDAPin] = true;
    pins_used[SCLPin] = true;
    Wire.begin(SDAPin, SCLPin); // SDA and SCL

    if (useDisplay) {
      if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
      { // Address 0x3C for 128x32
        Serial.println("Failed to start display");
      }
      drawDisplayContentError();
    }
    if (useDistanceSensor) {
      if (!distanceSensorChip.begin()) {
        Serial.println("Failed to start distance sensor");
      }
      else {
        Serial.println("Started distance sensor");
      }
    }
  }
}

void setupServer()
{
  server.on("/", handleRoot);

  // provides current sensor/actor/induction cooker readings
  server.on("/reqSensors", handleRequestSensors);
  server.on("/reqActors", handleRequestActors);
  server.on("/reqInduction", handleRequestInduction);

  // provides information about sensor/actor/induction cooker configuration
  server.on("/reqSensorConfig", handleRequestSensorConfig);
  server.on("/reqActorConfig", handleRequestActorConfig);
  server.on("/reqInductionConfig", handleRequestInductionConfig);

  // search for OneWire sensors on the bus
  server.on("/reqSearchSensorAdresses", handleRequestOneWireSensorAddresses);

  // returns the list of (named) currently free pins on this chip (takes a sensor id and type)
  server.on("/reqSensorPins", handleRequestSensorPins);
  // returns the list of (named) currently free pins on this chip (takes an actor id)
  server.on("/reqActorPins", handleRequestActorPins);

  // create or update sensor/actor/induction cooker
  server.on("/setSensor", handleSetSensor);
  server.on("/setActor", handleSetActor);
  server.on("/setIndu", handleSetIndu);

  // delete sensor/actor
  server.on("/delSensor", handleDelSensor);
  server.on("/delActor", handleDelActor);

  server.on("/getSysConfig", getSysConfig); // returns display config and mqtthost as json
  server.on("/setSysConfig", setSysConfig); // saves display config and mqtthost to config file

  server.on("/version", getVersion);        // returns the (hardcoded) firmware version of this device
  server.on("/mqttStatus", getMqttStatus);  // returns the current MQTT connection status
  server.on("/getOtherPins", getOtherPins); // returns other configuration (used pins)

  server.onNotFound(handleWebRequests); // fallback

  server.begin();
}

void setupOTA()
{
  Serial.print("Configuring OTA device...");
  TelnetServer.begin(); // necesary to autodetect OTA device
  ArduinoOTA.onStart([]() {
    Serial.println("OTA starting...");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("OTA update finished!");
    Serial.println("Rebooting...");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.print("OTA in progress: ");
    Serial.println((progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)
      Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA OK");
}
