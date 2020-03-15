void rebootDevice() {
  server.send(200, "text/plain", "Rebooting in 1 second...");
  delay(1000);
  // CAUTION: known (library) issue: only works if you (hardware) button-reset once after flashing a new firmware to the device
  ESP.restart();
}

void getVersion() {
  server.send(200, "text/plain", DEVICE_VERSION);
}

void getMqttStatus() {
  String returnMessage;
  String mqttAddress(mqtthost);
  if(client.connected()){
     returnMessage = "Successfully subscribed to MQTT Server at " + mqttAddress;
  }
  else{
    // not configured
    if(mqttAddress.length() == 0){
      returnMessage = "WARNING, MQTT ADDRESS IS UNDEFINED, PLEASE CONFIGURE DEVICE";
    }
    else{
      returnMessage = "WARNING, NOT SUBSCRIBED TO MQTT SERVER AT " + mqttAddress;
    }
  }
  server.send(200, "text/plain", returnMessage);
}

void getUseDisplay() {
  String returnMessage;
  if(use_display) {
  returnMessage = "Configured to use a display on pin D1 and D2, showing the first sensor readings.";
  }
  else {
    returnMessage = "Not using a display.";
  }
  server.send(200, "text/plain", returnMessage);
}

void getSysConfig() {
  String mqttAddress(mqtthost);
  StaticJsonBuffer<256> jsonBuffer;
  JsonObject &config = jsonBuffer.createObject();
  config["mqttAddress"] = mqttAddress;
  config["useDisplay"] = use_display;
  String response;
  config.printTo(response);
  server.send(200, "application/json", response);
}

void setSysConfig() {
  String string_mqtthost = server.arg(0);
  string_mqtthost.toCharArray(mqtthost, 16);
  String string_use_display = server.arg(1);
  if(string_use_display == "true")
    use_display = true;
  else{
    use_display = false;
  }
  saveConfig();
  rebootDevice();
}
