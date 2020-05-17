class Actor
{
  unsigned long powerLast; // Zeitmessung für High oder Low
  int dutycycle_actor = 5000;
  byte OFF;
  byte ON;

public:
  byte pin_actor = 9; // the number of the LED pin
  String argument_actor;
  byte power_actor;
  bool isOn;
  bool isInverted = false;

  Actor(String pin, String argument, bool inverted)
  {
    change(pin, argument, inverted);
  }

  void update()
  {
    if (isPin(pin_actor))
    {
      if (isOn && power_actor > 0)
      {
        if (millis() > powerLast + dutycycle_actor)
        {
          powerLast = millis();
        }
        if (millis() > powerLast + (dutycycle_actor * power_actor / 100L))
        {
          digitalWrite(pin_actor, OFF);
        }
        else
        {
          digitalWrite(pin_actor, ON);
        }
      }
      else
      {
        digitalWrite(pin_actor, OFF);
      }
    }
  }

  void change(String pin, String argument, bool inverted)
  {
    // set old pin to high
    if (isPin(pin_actor))
    {
      digitalWrite(pin_actor, HIGH);
      pins_used[pin_actor] = false;
      delay(5);
    }
    // set new pin to high
    pin_actor = StringToPin(pin);
    if (isPin(pin_actor))
    {
      pinMode(pin_actor, OUTPUT);
      digitalWrite(pin_actor, HIGH);
      pins_used[pin_actor] = true;
    }

    isOn = false;

    if (argument_actor != argument)
    {
      mqtt_unsubscribe();
      argument_actor = argument;
      mqtt_subscribe();
    }

    if (inverted)
    {
      isInverted = inverted;
      ON = HIGH;
      OFF = LOW;
    }
    else
    {
      isInverted = false;
      ON = LOW;
      OFF = HIGH;
    }
  }

  void mqtt_subscribe()
  {
    if (client.connected())
    {
      char subscribemsg[50];
      argument_actor.toCharArray(subscribemsg, 50);
      Serial.print("Subscribing to ");
      Serial.println(subscribemsg);
      client.subscribe(subscribemsg);
    }
  }

  void mqtt_unsubscribe()
  {
    if (client.connected())
    {
      char subscribemsg[50];
      argument_actor.toCharArray(subscribemsg, 50);
      Serial.print("Unsubscribing from ");
      Serial.println(subscribemsg);
      client.unsubscribe(subscribemsg);
    }
  }

  void handlemqtt(char *payload)
  {
    StaticJsonDocument<128> jsonDocument;
    DeserializationError error = deserializeJson(jsonDocument, payload);

    if (error)
    {
      Serial.print("Error reading JSON: ");
      Serial.println(error.c_str());
      return;
    }

    String state = jsonDocument["state"];

    if (state == "off")
    {
      isOn = false;
      power_actor = 0;
      return;
    }
    else if (state == "on")
    {
      int newpower = jsonDocument["power"];
      isOn = true;
      power_actor = min(100, newpower);
      power_actor = max(0, newpower);
      return;
    }
  }
};

/* Initialisierung des Arrays */
Actor actors[NUMBER_OF_ACTORS_MAX] = {
    Actor("", "", false),
    Actor("", "", false),
    Actor("", "", false),
    Actor("", "", false),
    Actor("", "", false),
    Actor("", "", false)};

/* Loop */
void handleActors()
{
  for (int i = 0; i < numberOfActors; i++)
  {
    actors[i].update();
    yield();
  }
}

/* Funktionen für Web */
void handleRequestActors()
{
  StaticJsonDocument<1024> jsonDocument;
  jsonDocument.to<JsonArray>(); // needed to prevent "null" responses
  for (int i = 0; i < numberOfActors; i++)
  {
    JsonObject actorResponse = jsonDocument.createNestedObject();
    actorResponse["status"] = actors[i].isOn;
    actorResponse["power"] = actors[i].power_actor;
    actorResponse["mqtt"] = actors[i].argument_actor;
    actorResponse["pin"] = PinToString(actors[i].pin_actor);
    yield();
  }

  String response;
  serializeJson(jsonDocument, response);

  server.send(200, "application/json", response);
}

void handleRequestActorConfig()
{
  int id = server.arg(0).toInt();
  String request = server.arg(1);
  String message;

  if (id == -1)
  {
    message = "";
    goto SendMessage;
  }
  else
  {
    if (request == "script")
    {
      message = actors[id].argument_actor;
      goto SendMessage;
    }
    if (request == "pin")
    {
      message = PinToString(actors[id].pin_actor);
      goto SendMessage;
    }
    if (request == "inverted")
    {
      message = actors[id].isInverted;
      goto SendMessage;
    }
    message = "not found";
  }
SendMessage:
  server.send(200, "text/plain", message);
}

void handleSetActor()
{
  int id = server.arg(0).toInt();

  if (id == -1)
  {
    id = numberOfActors;
    numberOfActors += 1;
  }

  String ac_pin = PinToString(actors[id].pin_actor);
  String ac_argument = actors[id].argument_actor;
  bool ac_isinverted = actors[id].isInverted;

  for (int i = 0; i < server.args(); i++)
  {
    if (server.argName(i) == "pin")
    {
      ac_pin = server.arg(i);
    }
    if (server.argName(i) == "script")
    {
      ac_argument = server.arg(i);
    }
    if (server.argName(i) == "inverted")
    {
      if (server.arg(i) == "true")
      {
        ac_isinverted = true;
      }
      else
      {
        ac_isinverted = false;
      }
    }
    yield();
  }

  actors[id].change(ac_pin, ac_argument, ac_isinverted);
  saveConfig();
  server.send(201, "text/plain", "created");
}

void handleDelActor()
{
  int id = server.arg(0).toInt();

  for (int i = id; i < numberOfActors; i++)
  {
    if (i == NUMBER_OF_ACTORS_MAX - 1)
    {
      actors[i].change("", "", false);
    }
    else
    {
      actors[i].change(PinToString(actors[i + 1].pin_actor), actors[i + 1].argument_actor, actors[i + 1].isInverted);
    }
  }

  numberOfActors -= 1;
  saveConfig();
  server.send(200, "text/plain", "deleted");
}

void handleRequestActorPins()
{
  int id = server.arg(0).toInt();
  String message;

  if (id != -1)
  {
    message += F("<option>");
    message += PinToString(actors[id].pin_actor);
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
  server.send(200, "text/plain", message);
}
