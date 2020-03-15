void setup()
{
  Serial.begin(115200);

  // declare OneWire and PT Pins as used
  pins_used[ONE_WIRE_BUS] = true;
  pins_used[PT_PINS[0]] = true;
  pins_used[PT_PINS[1]] = true;
  pins_used[PT_PINS[2]] = true;
  

  
  // Start sensors
  DS18B20.begin();

  // Load spif file system
  ESP.wdtFeed();
  if (!SPIFFS.begin())
  {
    Serial.println("SPIFFS Mount failed");
  }

  // Set device name
  snprintf(deviceName, 25, "MQTTDevice-%08X", ESP_CHIP_ID);
  
  // WiFi Manager
  ESP.wdtFeed();
  WiFi.hostname(deviceName);
  wifiManager.setTimeout(20);
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  if(!wifiManager.autoConnect(deviceName)) {
    Serial.println("Connection not possible, timeout, restart!");
    rebootDevice();
  }

  // activate ArduinoOTA
  ESP.wdtFeed();
  setupOTA();

  // Load settings
  ESP.wdtFeed();
  loadConfig();
  // declare display pins as used (if applicable)
  if(use_display){
    pins_used[D1] = true;
    pins_used[D2] = true;
  }

  // start mqtt
  client.setServer(mqtthost, MQTT_SERVER_PORT);
  client.setCallback(mqttcallback);

  // start webserver
  ESP.wdtFeed();
  setupServer();

  // start display if applicable
  if(use_display) {
    Wire.begin(D2, D1);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    }
    drawDisplayContentError();
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

  // returns the list of (named) currently free pins on this chip (takes a pt sensor id)
  server.on("/reqSensorPins", handleRequestPtSensorPins);
  // returns the list of (named) currently free pins on this chip (takes an actor id)
  server.on("/reqActorPins", handleRequestPins);

  // create or update sensor/actor/induction cooker
  server.on("/setSensor", handleSetSensor);
  server.on("/setActor", handleSetActor);
  server.on("/setIndu", handleSetIndu);

  // delete sensor/actor
  server.on("/delSensor", handleDelSensor);
  server.on("/delActor", handleDelActor);

  server.on("/getSysConfig", getSysConfig); // returns use_display and mqtthost as json
  server.on("/setSysConfig", setSysConfig); // saves use_display and mqtthost to config file

  server.on("/reboot", rebootDevice); // reboots the device
  server.on("/version", getVersion); // returns the (hardcoded) firmware version of this device
  server.on("/mqttStatus", getMqttStatus); // returns the current MQTT connection status
  server.on("/getUseDisplay", getUseDisplay); // returns, if the display has been configured

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
