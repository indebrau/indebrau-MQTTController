#include "induction.h"
#include "constantsAndGlobals.h"
#include "utils.h"
#include "config.h"

Induction::Induction(){};

void Induction::change(byte pinwhite, byte pinyellow, byte pinblue, String topic, long delayoff, bool is_enabled)
{
  if (isEnabled)
  {
    // aktuelle PINS deaktivieren
    if (isPin(PIN_WHITE))
    {
      digitalWrite(PIN_WHITE, HIGH);
      pins_used[PIN_WHITE] = false;
    }
    if (isPin(PIN_YELLOW))
    {
      digitalWrite(PIN_YELLOW, HIGH);
      pins_used[PIN_YELLOW] = false;
    }
    if (isPin(PIN_INTERRUPT))
    {
      digitalWrite(PIN_INTERRUPT, HIGH);
      pins_used[PIN_INTERRUPT] = false;
    }
    mqtt_unsubscribe();
  }

  PIN_WHITE = pinwhite;
  PIN_YELLOW = pinyellow;
  PIN_INTERRUPT = pinblue;

  mqtttopic = topic;
  delayAfteroff = delayoff;

  isEnabled = is_enabled;

  if (isEnabled)
  {
    // neue PINS aktiveren
    if (isPin(PIN_WHITE))
    {
      pinMode(PIN_WHITE, OUTPUT);
      digitalWrite(PIN_WHITE, LOW);
      pins_used[PIN_WHITE] = true;
    }
    if (isPin(PIN_YELLOW))
    {
      pinMode(PIN_YELLOW, OUTPUT);
      digitalWrite(PIN_YELLOW, LOW);
      pins_used[PIN_YELLOW] = true;
    }
    if (isPin(PIN_INTERRUPT))
    {
      pinMode(PIN_INTERRUPT, INPUT_PULLUP);
      pins_used[PIN_INTERRUPT] = true;
    }
    if (client.connected())
    {
      mqtt_subscribe();
    }
  }
}

void Induction::mqtt_subscribe()
{
  if (isEnabled)
  {
    if (client.connected())
    {
      char subscribemsg[50];
      mqtttopic.toCharArray(subscribemsg, 50);
      Serial.print("Subscribing to ");
      Serial.println(subscribemsg);
      client.subscribe(subscribemsg);
    }
  }
}

void Induction::mqtt_unsubscribe()
{
  if (client.connected())
  {
    char subscribemsg[50];
    mqtttopic.toCharArray(subscribemsg, 50);
    Serial.print("Unsubscribing from ");
    Serial.println(subscribemsg);
    client.unsubscribe(subscribemsg);
  }
}

void Induction::handlemqtt(char *payload)
{
  StaticJsonDocument<128> jsonDocument;
  DeserializationError error = deserializeJson(jsonDocument, payload);
  if (error)
  {
    Serial.print("Error reading JSON: ");
    Serial.println(error.c_str());
    return;
  }
  bool state = jsonDocument["on"];
  if (!state)
  {
    newPower = 0;
    return;
  }
  else if (state)
  {
    newPower = jsonDocument["power"];
  }
}

bool Induction::updateRelay()
{
  if (isInduOn == true && isRelayOn == false)
  {
    digitalWrite(PIN_WHITE, HIGH);
    return true;
  }
  if (isInduOn == false && isRelayOn == true)
  {
    if (millis() > timeTurnedOff + (delayAfteroff * 1000))
    {
      digitalWrite(PIN_WHITE, LOW);
      return false;
    }
  }
  if (isInduOn == false && isRelayOn == false)
  { /* Ist aus, bleibt aus. */
    return false;
  }
  return true; /* Ist an, bleibt an. */
}

void Induction::update()
{
  updatePower();

  isRelayOn = updateRelay();

  if (isInduOn && power > 0)
  {
    if (millis() > powerLast + powerSampletime)
    {
      powerLast = millis();
    }
    if (millis() > powerLast + powerHigh)
    {
      sendCommand(CMD[CMD_CUR - 1]);
      isPower = false;
    }
    else
    {
      sendCommand(CMD[CMD_CUR]);
      isPower = true;
    }
  }
  else if (isRelayOn)
  {
    sendCommand(CMD[0]);
  }
}

void Induction::updatePower()
{
  lastCommand = millis();
  if (power != newPower)
  {
    if (newPower > 100)
    {
      newPower = 100;
    }
    if (newPower < 0)
    {
      newPower = 0;
    }
    power = newPower;
    timeTurnedOff = 0;
    isInduOn = true;
    long difference = 0;
    if (power == 0)
    {
      CMD_CUR = 0;
      timeTurnedOff = millis();
      isInduOn = false;
      difference = 0;
      goto setPowerLevel;
    }
    for (int i = 1; i < 7; i++)
    {
      if (power <= PWR_STEPS[i])
      {
        CMD_CUR = i;
        difference = PWR_STEPS[i] - power;
        goto setPowerLevel;
      }
    }
  setPowerLevel: /* Wie lange "HIGH" oder "LOW" */
    if (difference != 0)
    {
      powerLow = powerSampletime * difference / 20L;
      powerHigh = powerSampletime - powerLow;
    }
    else
    {
      powerHigh = powerSampletime;
      powerLow = 0;
    };
  }
}

void Induction::sendCommand(const int command[33])
{
  digitalWrite(PIN_YELLOW, HIGH);
  delay(SIGNAL_START);
  digitalWrite(PIN_YELLOW, LOW);
  delay(SIGNAL_WAIT);

  for (int i = 0; i < 33; i++)
  {
    digitalWrite(PIN_YELLOW, HIGH);
    delayMicroseconds(command[i]);
    digitalWrite(PIN_YELLOW, LOW);
    delayMicroseconds(SIGNAL_LOW);
  }
}

void handleInduction()
{
  if (inductionCooker.isEnabled)
  {
    inductionCooker.update();
  }
}

void handleRequestInduction()
{
  StaticJsonDocument<512> jsonDocument;

  jsonDocument["enabled"] = inductionCooker.isEnabled;
  if (inductionCooker.isEnabled)
  {
    jsonDocument["relayOn"] = inductionCooker.isRelayOn;
    jsonDocument["power"] = inductionCooker.power;
    jsonDocument["relayOn"] = inductionCooker.isRelayOn;
    jsonDocument["topic"] = inductionCooker.mqtttopic;
    if (inductionCooker.isPower)
    {
      jsonDocument["powerLevel"] = inductionCooker.CMD_CUR;
    }
    else
    {
      jsonDocument["powerLevel"] = max(0, inductionCooker.CMD_CUR - 1);
    }
  }
  String response;
  serializeJson(jsonDocument, response);
  server.send(200, "application/json", response);
}

void handleRequestInductionConfig()
{
  String request = server.arg(0);
  String message;

  if (request == "isEnabled")
  {
    if (inductionCooker.isEnabled)
    {
      message = "1";
    }
    else
    {
      message = "0";
    }
    goto SendMessage;
  }
  if (request == "topic")
  {
    message = inductionCooker.mqtttopic;
    goto SendMessage;
  }
  if (request == "delay")
  {
    message = inductionCooker.delayAfteroff;
    goto SendMessage;
  }
  if (request == "pins")
  {
    int id = server.arg(1).toInt();
    byte pinswitched = 255; // some default value
    switch (id)
    {
    case 0:
      pinswitched = inductionCooker.PIN_WHITE;
      break;
    case 1:
      pinswitched = inductionCooker.PIN_YELLOW;
      break;
    case 2:
      pinswitched = inductionCooker.PIN_INTERRUPT;
      break;
    }
    if (isPin(pinswitched))
    {
      message += F("<option>");
      message += PinToString(pinswitched);
      message += F("</option><option disabled>──────────</option>");
    }

    for (int i = 0; i < NUMBER_OF_PINS; i++)
    {
      if (pins_used[PINS[i]] == false)
      {
        message += F("<option>");
        message += PIN_NAMES[i];
        message += F("</option>");
      }
      yield();
    }
    goto SendMessage;
  }

SendMessage:
  server.send(200, "text/plain", message);
}

void handleSetIndu()
{

  byte pin_white = inductionCooker.PIN_WHITE;
  byte pin_blue = inductionCooker.PIN_INTERRUPT;
  byte pin_yellow = inductionCooker.PIN_YELLOW;
  long delayoff = inductionCooker.delayAfteroff;
  bool is_enabled = inductionCooker.isEnabled;
  String topic = inductionCooker.mqtttopic;

  for (int i = 0; i < server.args(); i++)
  {
    if (server.argName(i) == "enabled")
    {
      if (server.arg(i) == "1")
      {
        is_enabled = true;
      }
      else
      {
        is_enabled = false;
      }
    }
    if (server.argName(i) == "topic")
    {
      topic = server.arg(i);
    }
    if (server.argName(i) == "pinwhite")
    {
      pin_white = StringToPin(server.arg(i));
    }
    if (server.argName(i) == "pinyellow")
    {
      pin_yellow = StringToPin(server.arg(i));
    }
    if (server.argName(i) == "pinblue")
    {
      pin_blue = StringToPin(server.arg(i));
    }
    if (server.argName(i) == "delay")
    {
      delayoff = server.arg(i).toInt();
    }
    yield();
  }

  inductionCooker.change(pin_white, pin_yellow, pin_blue, topic, delayoff, is_enabled);

  saveConfig();
  server.send(201, "text/plain", "created");
}
