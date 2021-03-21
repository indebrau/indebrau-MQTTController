#include "constantsAndGlobals.h"
#include "web.h"

void handleRoot()
{
  server.sendHeader("Location", "/index.html", true); // redirect to our html web page
  server.send(302, "text/plain", "");
}

void handleWebRequests()
{
  if (loadFromLittleFS(server.uri()))
  {
    return;
  }
  String message = "File Not Detected\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++)
  {
    message += " NAME:" + server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

bool loadFromLittleFS(String path)
{
  String dataType = "text/plain";
  if (path.endsWith("/"))
    path += "index.html";

  if (path.endsWith(".src"))
    path = path.substring(0, path.lastIndexOf("."));
  else if (path.endsWith(".html"))
    dataType = "text/html";
  else if (path.endsWith(".htm"))
    dataType = "text/html";
  else if (path.endsWith(".css"))
    dataType = "text/css";
  else if (path.endsWith(".js"))
    dataType = "application/javascript";
  else if (path.endsWith(".png"))
    dataType = "image/png";
  else if (path.endsWith(".gif"))
    dataType = "image/gif";
  else if (path.endsWith(".jpg"))
    dataType = "image/jpeg";
  else if (path.endsWith(".ico"))
    dataType = "image/x-icon";
  else if (path.endsWith(".xml"))
    dataType = "text/xml";
  else if (path.endsWith(".pdf"))
    dataType = "application/pdf";
  else if (path.endsWith(".zip"))
    dataType = "application/zip";

  if (!LittleFS.exists(path.c_str()))
  {
    return false;
  }
  File dataFile = LittleFS.open(path.c_str(), "r");
  if (server.hasArg("download"))
    dataType = "application/octet-stream";
  if (server.streamFile(dataFile, dataType) != dataFile.size())
  {
  }
  dataFile.close();
  return true;
}

void mqttreconnect()
{
  // try to connect, if client is not yet connected
  if (!client.connected())
  {
    // Delay prüfen
    if (millis() > (unsigned)mqttconnectlasttry + MQTT_CONNECT_DELAY_SECONDS * 1000)
    {
      String mqttAddress(mqtthost);
      Serial.print("MQTT trying to connect to ");
      Serial.print(mqttAddress);
      Serial.print(" with name ");
      Serial.print(deviceName);
      if (client.connect(deviceName))
      {
        Serial.println(" successful. Subscribing.");
      }
      else
      {
        mqttconnectlasttry = millis();
        Serial.print(" failed. Trying again in ");
        Serial.print(MQTT_CONNECT_DELAY_SECONDS);
        Serial.println(" seconds.");
        return;
      }
    }
  }
  for (int i = 0; i < numberOfActors; i++)
  {
    actors[i].mqtt_subscribe();
    yield();
  }
  inductionCooker.mqtt_subscribe();
}

void mqttcallback(char *topic, byte *payload, unsigned int length)
{
  char payload_msg[length];
  for (unsigned int i = 0; i < length; i++)
  {
    payload_msg[i] = payload[i];
  }

  if (inductionCooker.mqtttopic == topic)
  {
    inductionCooker.handlemqtt(payload_msg);
  }
  for (int i = 0; i < numberOfActors; i++)
  {
    if (actors[i].argument_actor == topic)
    {
      actors[i].handlemqtt(payload_msg);
    }
    yield();
  }
}
