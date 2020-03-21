void loop()
{

  // Handle webserver and OTA first
  server.handleClient();
  ArduinoOTA.handle();
  
  // Now check if system update is needed
  if (millis() > lastToggled + UPDATE)
  {
    // Check wifi status
    if (WiFi.status() != WL_CONNECTED)
    {
      drawDisplayContentError();
      /* If the wifimanager runs into a timeout (20 seconds), restart device  
       * Prevents being stuck in Access Point mode when Wifi signal was
       * temporarily lost. 
       */
      if(!wifiManager.autoConnect(deviceName)) {
        Serial.println("Connection not possible, timeout, restart!");
        rebootDevice();
      } 
    }

    // Check mqtt status
    if (!client.connected())
    {
      mqttreconnect();
    }

    handleSensors();
    handleActors();
    handleInduction();
    drawDisplayContent();

    client.loop();
    lastToggled = millis();
  }
}
