void rebootDevice() {
  server.send(200, "text/plain", "Rebooting in 1 second...");
  delay(1000);
  ESP.restart();
}

void getVersion() {
  server.send(200, "text/plain", DEVICE_VERSION);
}

void getMqttStatus() {
  String returnMessage;
  String mqttAddress(mqtthost);
  if(client.connected()){
     returnMessage = "Device successfully subscribed to MQTT Server at " + mqttAddress + ".";
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
    returnMessage = "Configured to use a display on pin ";
    returnMessage += PinToString(firstDisplayPin);
    returnMessage += " and pin ";
    returnMessage += PinToString(secondDisplayPin);
    returnMessage += ", showing the first sensor readings.";
  }
  else {
    returnMessage = "Not using a display.";
  }
  server.send(200, "text/plain", returnMessage);
}

void getSysConfig() {
  String mqttAddress(mqtthost);
  StaticJsonBuffer<1024> jsonBuffer;
  JsonObject &config = jsonBuffer.createObject();
  config["mqttAddress"] = mqttAddress;
  config["useDisplay"] = use_display;
  
  // get list of free pins
  String freePins = "";
  for (int i = 0; i < NUMBER_OF_PINS; i++) {
    if (pins_used[PINS[i]] == false) {
      freePins += F("<option>");
      freePins += PIN_NAMES[i];
      freePins += F("</option>");
    }
    yield();
  }
  // first pin message
  String firstPinMessage = "";
  if (use_display) {
    firstPinMessage += F("<option>");
    firstPinMessage += PinToString(firstDisplayPin);
    firstPinMessage += F("</option><option disabled>──────────</option>");
  }
  firstPinMessage += freePins;
  // second pin message
  String secondPinMessage = "";
  if (use_display) {
    secondPinMessage += F("<option>");
    secondPinMessage += PinToString(secondDisplayPin);
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

void setSysConfig() {
  String string_mqtthost = server.arg(0);
  string_mqtthost.toCharArray(mqtthost, 16);
  String string_use_display = server.arg(1);
  if(string_use_display == "true"){
    use_display = true;
    firstDisplayPin = StringToPin(server.arg(2));
    secondDisplayPin = StringToPin(server.arg(3));
  } else{
    use_display = false;
  }
  saveConfig();
  rebootDevice();
}
