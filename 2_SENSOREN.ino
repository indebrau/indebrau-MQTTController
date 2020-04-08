/*
  Sensor classes and related functions.
  Currently supports OneWire and PT100/1000 sensors.
*/
class OneWireSensor{
  private:
    unsigned long lastCalled;

  public:
    char mqtttopic[50];
    byte address[8]; // one wire address
    String name; // just a name that is displayed (in frontend)
    float value; // value to be send
    float offset;

    String getSens_address_string(){
      return OneWireAddressToString(address);
    }

    OneWireSensor(String address, String mqtttopic, String name, float offset){
      change(address, mqtttopic, name, offset);
    }

    void update(){
      if (millis() > (lastCalled + DEFAULT_SENSOR_UPDATE_INTERVAL))
      {

        DS18B20.requestTemperatures();
        value = DS18B20.getTempC(address);

        if (value == -127.0){
          Serial.print("OneWire Sensor ");
          Serial.print(name);
          Serial.println(" not found!");
        }
        else{
          if (value == 85.0){
            Serial.print("One Wire Sensor ");
            Serial.print(name);
            Serial.println(" Error!");
          }
          else{
            value = value + offset; // after error checking, add offset
            publishmqtt(); // and send
          }
        }

        lastCalled = millis();
      }
    }

    void change(String new_address, String new_mqtttopic, String new_name, float new_offset){
      new_mqtttopic.toCharArray(mqtttopic, new_mqtttopic.length() + 1);
      name = new_name;
      offset = new_offset;
      // if this check fails, this could also mean a call from array init
      // (no actual sensor defined for this array entry, so skip init here)
      if (new_address.length() == 16){
        char address_char[16];

        new_address.toCharArray(address_char, 17);

        char hexbyte[2];
        int octets[8];

        for (int d = 0; d < 16; d += 2){
          // Assemble a digit pair into the hexbyte string
          hexbyte[0] = address_char[d];
          hexbyte[1] = address_char[d + 1];

          // Convert the hex pair to an integer
          sscanf(hexbyte, "%x", &octets[d / 2]);
          yield();
        }
        Serial.print("Starting OneWire sensor: ");
        for (int i = 0; i < 8; i++){
          address[i] = octets[i];
          Serial.print(address[i], HEX);
        }
        Serial.println("");
        DS18B20.setResolution(address, ONE_WIRE_RESOLUTION);
      }
    }

    void publishmqtt(){
      if (client.connected()){
        client.publish(mqtttopic, getValueString());
      }
    }

    char *getValueString(){
      char buf[8];
      dtostrf(value, 3, 2, buf);
      return buf;
    }
};

class PTSensor{
  private:
    unsigned long lastCalled; // timestamp
    // reference to amplifier board, with initial values (will be overwritten in "constructor")
    Adafruit_MAX31865 maxChip = Adafruit_MAX31865(DEFAULT_CS_PIN, PT_PINS[0], PT_PINS[1], PT_PINS[2]);

  public:
    byte csPin = DEFAULT_CS_PIN; // set to default, otherwise would be "D3" (-> Arduino 0)
    byte numberOfWires;
    char mqttTopic[50];
    String name; // just a name that is displayed (in frontend)
    float value; // value to be send
    float offset;

    PTSensor(String csPin, byte numberOfWires, String mqtttopic, String name, float offset){
      change(csPin, numberOfWires, mqtttopic, name, offset);
    }

    void update(){
      if (millis() > (lastCalled + DEFAULT_SENSOR_UPDATE_INTERVAL)){
        value = maxChip.temperature(RNOMINAL, RREF);

        // sensor reads very low temps if disconnected
        if (maxChip.readFault() || value < -100)
        {
          value = -127.0;
          maxChip.clearFault();
        }
        else
        {
          value = value + offset; // after error checking, add offset
          publishmqtt(); // and send
        }
        lastCalled = millis();
      }
    }

    void change(String newCSPin, byte newNumberOfWires, String newMqttTopic, String newName, float newOffset){
      // check for initial empty array entry initialization
      // (no actual sensor defined in this call)
      if (newCSPin != ""){
        byte byteNewCSPin = StringToPin(newCSPin);
        // if this pin fits the bill, go for it..
        if (isPin(byteNewCSPin)){
          pins_used[csPin] = false;
          csPin = byteNewCSPin;
          pins_used[csPin] = true;
          newMqttTopic.toCharArray(mqttTopic, newMqttTopic.length() + 1);
          numberOfWires = newNumberOfWires;
          name = newName;
          offset = newOffset;
          maxChip = Adafruit_MAX31865(csPin, PT_PINS[0], PT_PINS[1], PT_PINS[2]);
          Serial.print("Starting PT sensor with ");
          Serial.print(newNumberOfWires);
          Serial.print(" wires. CS Pin is ");
          Serial.println(PinToString(csPin));
          if (newNumberOfWires == 4){
            maxChip.begin(MAX31865_4WIRE);
          } else if (newNumberOfWires == 3){
            maxChip.begin(MAX31865_3WIRE);
          } else{
            maxChip.begin(MAX31865_2WIRE); // set to 2 wires as default
          }
        }
      }
    }

    void publishmqtt(){
      if (client.connected()) {
        client.publish(mqttTopic, getValueString());
      }
    }

    char *getValueString(){
      char buf[8];
      dtostrf(value, 3, 2, buf);
      return buf;
    }
};

class DistanceSensor{
  private:
    unsigned long lastCalled; // timestamp

  public:
    byte triggerPin; // set to some default
    byte echoPin; // set to default
    char mqttTopic[50];
    String name; // just a name that is displayed (in frontend)
    float value; // value to be send in cm

    DistanceSensor(String triggerPin, String echoPin, String mqtttopic, String name){
      Serial.print("Init sensor " + name);
      change(triggerPin, echoPin, mqtttopic, name);
    }

    void update(){
      if (millis() > (lastCalled + DEFAULT_SENSOR_UPDATE_INTERVAL)){
        digitalWrite(triggerPin, HIGH);
        delayMicroseconds(200); // wait between switching (more than enough)
        digitalWrite(triggerPin, LOW);
        value = pulseIn(echoPin, HIGH);
        value = ((value * 0.034) / 2);
        
        // sensor reads less than 20cm if disconnected or distance too close
        if (value < 20){
          value = -127.0;
        }
        else{
          publishmqtt(); // and send
        }
        lastCalled = millis();
      }
    }

    void change(String newTriggerPin, String newEchoPin, String newMqttTopic, String newName){
      // check for initial empty array entry initialization
      // (no actual sensor defined in this call)
      if (newTriggerPin != ""){
        byte byteNewTriggerPin = StringToPin(newTriggerPin);
        byte byteNewEchoPin = StringToPin(newEchoPin);
        // if these pins fit the bill, go for it..
        if (isPin(byteNewTriggerPin) && isPin(byteNewEchoPin)){
          pins_used[triggerPin] = false;
          pins_used[echoPin] = false;
          triggerPin = byteNewTriggerPin;
          echoPin = byteNewEchoPin;
          pins_used[triggerPin] = true;
          pins_used[echoPin] = true;
          
          newMqttTopic.toCharArray(mqttTopic, newMqttTopic.length() + 1);
          name = newName;
          pinMode(echoPin, INPUT);
          pinMode(triggerPin, OUTPUT);
          digitalWrite(echoPin, HIGH);
          Serial.print("Starting distance sensor with trigger pin ");
          Serial.print(PinToString(triggerPin));
          Serial.print(" and echo pin ");
          Serial.println(PinToString(echoPin));
        }
      }
    }

    void publishmqtt(){
      if (client.connected()){
        client.publish(mqttTopic, getValueString());
      }
    }

    char *getValueString(){
      char buf[8];
      dtostrf(value, 3, 2, buf);
      return buf;
    }
};

/*
  Initializing Sensor Arrays
  please mind: max sensor capacity is interpreted as max for each type
  TODO: Initialized hardcoded to (max) 6 sensors
*/
OneWireSensor oneWireSensors[NUMBER_OF_SENSORS_MAX] = {
  OneWireSensor("","","",0),
  OneWireSensor("","","",0),
  OneWireSensor("","","",0),
  OneWireSensor("","","",0),
  OneWireSensor("","","",0),
  OneWireSensor("","","",0)
};

PTSensor ptSensors[NUMBER_OF_SENSORS_MAX] = {
  PTSensor("",0,"","",0),
  PTSensor("",0,"","",0),
  PTSensor("",0,"", "",0),
  PTSensor("",0,"","",0),
  PTSensor("",0,"","",0),
  PTSensor("",0,"","",0)
};

DistanceSensor distanceSensors[NUMBER_OF_SENSORS_MAX] = {
  DistanceSensor("D1","D6","testTopic","test"),
  DistanceSensor("","","",""),
  DistanceSensor("","","",""),
  DistanceSensor("","","",""),
  DistanceSensor("","","",""),
  DistanceSensor("","","","")
};

/* Called in loop() */
void handleSensors()
{
  for (int i = 0; i < numberOfOneWireSensors; i++){
    oneWireSensors[i].update();
    yield();
  }
  for (int i = 0; i < numberOfPTSensors; i++){
    ptSensors[i].update();
    yield();
  }
  for (int i = 0; i < numberOfDistanceSensors; i++){
    distanceSensors[i].update();
    yield();
  }
}

/* Search the OneWire bus for available sensor addresses */
byte searchOneWireSensors(){
  byte n = 0;
  byte addr[8];
  while (oneWire.search(addr))
  {
    if (OneWire::crc8(addr, 7) == addr[7])
    {
      Serial.print("Sensor found: ");
      for (int i = 0; i < 8; i++)
      {
        oneWireAddressesFound[n][i] = addr[i];
        Serial.print(addr[i], HEX);
      }
      Serial.println("");
      n += 1;
    }
    yield();
  }
  oneWire.reset_search();
  return n;
}

/* Convert the OneWire sensor address to a (printable) string */
String OneWireAddressToString(byte addr[8]){
  char charbuffer[50];
  sprintf(charbuffer, "%02x%02x%02x%02x%02x%02x%02x%02x", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);
  return charbuffer;
}

/* All following fuctions called from frontend (mapping in 0_Setup.ino) */

/* Update sensor attributes or create new sensor.*/
void handleSetSensor(){
  int id = server.arg(0).toInt();
  String type = server.arg(1);

  if (type == SENSOR_TYPE_ONE_WIRE){
    // means: create new sensor request
    if (id == -1){
      id = numberOfOneWireSensors;
      numberOfOneWireSensors += 1;
      // first sensor, block OneWire sensor pins
      if(numberOfOneWireSensors == 1){
        pins_used[ONE_WIRE_BUS] = true;
      }
    }
    String newName = server.arg(2);
    String newTopic = server.arg(3);
    String newAddress = server.arg(4);
    float newOffset = server.arg(5).toFloat();
    oneWireSensors[id].change(newAddress, newTopic, newName, newOffset);
  }
  else if (type == SENSOR_TYPE_PT){
    // means: create new sensor request
    if (id == -1){
      id = numberOfPTSensors;
      numberOfPTSensors += 1;
      // first sensor, block PT sensor pins
      if(numberOfPTSensors == 1){
        pins_used[PT_PINS[0]] = true;
        pins_used[PT_PINS[1]] = true;
        pins_used[PT_PINS[2]] = true;
      }
    }
    String newName = server.arg(2);
    String newTopic = server.arg(3);
    String newCsPin = server.arg(4);
    byte newNumberOfWires = server.arg(5).toInt();
    float newOffset = server.arg(6).toFloat();
    ptSensors[id].change(newCsPin, newNumberOfWires, newTopic, newName, newOffset);
  }
   else if (type == SENSOR_TYPE_ULTRASONIC){
    // means: create new sensor request
    if (id == -1){
      id = numberOfDistanceSensors;
      numberOfDistanceSensors += 1;
    }
    String newName = server.arg(2);
    String newTopic = server.arg(3);
    String newTriggerPin = server.arg(4);
    String newEchoPin = server.arg(5);
    distanceSensors[id].change(newTriggerPin, newEchoPin, newTopic, newName);
  }
  // unknown type
  else{
    server.send(400, "text/plain", "Unknown Sensor Type");
    return;
  }
  // all done, save and exit
  saveConfig();
  server.send(201, "text/plain", "created");
}

/* Delete a sensor.*/
void handleDelSensor(){
  int id = server.arg(0).toInt();
  String type = server.arg(1);
  // OneWire
  if (type == SENSOR_TYPE_ONE_WIRE){
    // move all sensors following the given id one to the front of array,
    // effectively overwriting the sensor to be deleted..
    for (int i = id; i < numberOfOneWireSensors; i++)
    {
      oneWireSensors[i].change(oneWireSensors[i + 1].getSens_address_string(), oneWireSensors[i + 1].mqtttopic, oneWireSensors[i + 1].name, oneWireSensors[i + 1].offset);
      yield();
    }
    // ..and declare the array's content to one sensor less
    numberOfOneWireSensors -= 1;
    // last sensor removed, free pin usage
    if(numberOfOneWireSensors == 0){
      pins_used[ONE_WIRE_BUS] = false;
    }
  } else if (type == SENSOR_TYPE_PT) {
    // first declare the pin unused
    pins_used[ptSensors[id].csPin] = false;
    // move all sensors following the given id one to the front of array,
    // effectively overwriting the sensor to be deleted..
    for (int i = id; i < numberOfPTSensors; i++)
    {
      String csPinString = String(ptSensors[i + 1].csPin); // yeah, not very nice or efficient..
      ptSensors[i].change(csPinString, ptSensors[i + 1].numberOfWires, ptSensors[i + 1].mqttTopic, ptSensors[i + 1].name, ptSensors[i + 1].offset);
      yield();
    }
    // ..and declare the array's content to one sensor less
    numberOfPTSensors -= 1;
    // last sensor removed, free pin usages
    if(numberOfPTSensors == 0){
      pins_used[PT_PINS[0]] = false;
      pins_used[PT_PINS[1]] = false;
      pins_used[PT_PINS[2]] = false;
    }
  } else if (type == SENSOR_TYPE_ULTRASONIC) {
    // first declare the pin unused
    pins_used[distanceSensors[id].triggerPin] = false;
      pins_used[distanceSensors[id].echoPin] = false;
    // move all sensors following the given id one to the front of array,
    // effectively overwriting the sensor to be deleted..
    for (int i = id; i < numberOfDistanceSensors; i++)
    {
      String triggerPin = String(distanceSensors[i + 1].triggerPin); // yeah, not very nice or efficient..
      String echoPin = String(distanceSensors[i + 1].echoPin); // yeah, not very nice or efficient..
      distanceSensors[i].change(triggerPin, echoPin, distanceSensors[i + 1].mqttTopic, distanceSensors[i + 1].name);
      yield();
    }
    // ..and declare the array's content to one sensor less
    numberOfDistanceSensors -= 1;
  } else {
    server.send(400, "text/plain", "Unknown sensor type");
    return;
  }
  // all done, save and exit
  saveConfig();
  server.send(200, "text/plain", "deleted");
}

/* Provides search results of OneWire bus search */
void handleRequestOneWireSensorAddresses(){
  numberOfOneWireSensorsFound = searchOneWireSensors();
  int id = server.arg(0).toInt();
  String message = "";
  // if id given, render this sensor's address first
  // and check if id is valid (client could send nonsense for id..)
  if (id != -1 && id < numberOfOneWireSensors){
    message += F("<option>");
    message += oneWireSensors[id].getSens_address_string();
    message += F("</option><option disabled>──────────</option>");
  }
  // Now render all found addresses, except the one already assigned to the sensor
  for (int i = 0; i < numberOfOneWireSensorsFound; i++)
  {
    String foundAddress = OneWireAddressToString(oneWireAddressesFound[i]);
    if (id == -1 || !(oneWireSensors[id].getSens_address_string() == foundAddress)){
      message += F("<option>");
      message += foundAddress;
      message += F("</option>");
    }
    yield();
  }
  server.send(200, "text/html", message);
}

/* Similar as the actor pin request function, but this time for PT sensor CS pin */
void handleRequestPtSensorPins(){
  int id = server.arg(0).toInt();
  String message;

  if (id != -1){
    message += F("<option>");
    message += PinToString(ptSensors[id].csPin);
    message += F("</option><option disabled>──────────</option>");
  }
  for (int i = 0; i < NUMBER_OF_PINS; i++){
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

/*
  Returns a JSON array with the current sensor data of all sensors.
*/
void handleRequestSensors()
{
  StaticJsonBuffer<1024> jsonBuffer;
  JsonArray &sensorsResponse = jsonBuffer.createArray();

  for (int i = 0; i < numberOfOneWireSensors; i++)
  {
    JsonObject &sensorResponse = jsonBuffer.createObject();
    ;
    sensorResponse["name"] = oneWireSensors[i].name;
    if ((oneWireSensors[i].value != -127.0) && (oneWireSensors[i].value != 85.0))
    {
      sensorResponse["value"] = oneWireSensors[i].getValueString();
    }
    else
    {
      sensorResponse["value"] = "ERR";
    }
    sensorResponse["mqtt"] = oneWireSensors[i].mqtttopic;
    sensorResponse["type"] = SENSOR_TYPE_ONE_WIRE;
    sensorResponse["id"] = i;
    sensorResponse["offset"] = oneWireSensors[i].offset;
    sensorsResponse.add(sensorResponse);
    yield();
  }
  for (int i = 0; i < numberOfPTSensors; i++)
  {
    JsonObject &sensorResponse = jsonBuffer.createObject();
    sensorResponse["name"] = ptSensors[i].name;
    // While it is technically possible to read this low value with a PT sensor
    // We reuse "OneWire Error Codes" here for simplicity
    if (ptSensors[i].value != -127.0)
    {
      sensorResponse["value"] = ptSensors[i].getValueString();
    }
    else
    {
      sensorResponse["value"] = "ERR";
    }
    sensorResponse["mqtt"] = ptSensors[i].mqttTopic;
    sensorResponse["type"] = SENSOR_TYPE_PT;
    sensorResponse["id"] = i;
    sensorResponse["offset"] = ptSensors[i].offset;
    sensorsResponse.add(sensorResponse);
    yield();
  }
  String response;
  sensorsResponse.printTo(response);
  server.send(200, "application/json", response);
}

/* Returns information for one specific sensor (for update / delete menu in frontend) */
void handleRequestSensorConfig()
{
  int id = server.arg(0).toInt();
  String type = server.arg(1);
  String request = server.arg(2);
  String response;

  if (type == SENSOR_TYPE_ONE_WIRE)
  {
    if (id == -1 || id > numberOfOneWireSensors)
    {
      response = "not found";
      server.send(404, "text/plain", response);
      return;
    }
    else
    {
      StaticJsonBuffer<1024> jsonBuffer;
      JsonObject &sensorJson = jsonBuffer.createObject();
      sensorJson["name"] = oneWireSensors[id].name;
      sensorJson["topic"] = oneWireSensors[id].mqtttopic;
      sensorJson["offset"] = oneWireSensors[id].offset;
      sensorJson.printTo(response);
      server.send(200, "application/json", response);
      return;
    }
  }
  else if (type == SENSOR_TYPE_PT)
  {
    if (id == -1 || id > numberOfPTSensors)
    {
      response = "not found";
      server.send(404, "text/plain", response);
      return;
    }
    else
    {
      StaticJsonBuffer<1024> jsonBuffer;
      JsonObject &sensorJson = jsonBuffer.createObject();
      sensorJson["name"] = ptSensors[id].name;
      sensorJson["topic"] = ptSensors[id].mqttTopic;
      sensorJson["csPin"] = PinToString(ptSensors[id].csPin);
      sensorJson["numberOfWires"] = ptSensors[id].numberOfWires;
      sensorJson["offset"] = ptSensors[id].offset;
      sensorJson.printTo(response);
      server.send(200, "application/json", response);
      return;
    }
  }
  response = "unknown type: ";
  response += type;
  server.send(406, "text/plain", response);
  return;
}
