void rebootDevice() {
  server.send(200, "text/plain", "rebooting...");
  delay(1000);
  // CAUTION: known (library) issue: only works if you (hardware) button-reset once after flashing the device
  ESP.restart();
}

void getVersion() {
  server.send(200, "text/plain", DEVICE_VERSION);
}

void getMqttStatus() {
  String returnMessage;
  String mqttAddress(mqtthost);
  if(client.connected()){
     returnMessage = "successfully subscribed to " + mqttAddress;
  }
  else{
    returnMessage = "WARNING, NOT SUBSCRIBED TO " + mqttAddress;
  }
  server.send(200, "text/plain", returnMessage);
}
