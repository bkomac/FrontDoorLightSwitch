#include <Espiot.h>

Espiot espiot;

// Vcc measurement
//ADC_MODE(ADC_VCC);

String sensorData = "";
int sensorValue;

// CONF
String ver = "1.0.2";
long lastTime = millis();

// PIR
#define PIRPIN 14

#define BUILTINLED 2
#define RELEY 13

String MODE = "AUTO";
boolean sentMsg = false;

void setup() { //------------------------------------------------
  Serial.begin(115200);

  pinMode(PIRPIN, INPUT);
  pinMode(RELEY, OUTPUT);
  pinMode(BUILTINLED, OUTPUT);

  stateOff("AUTO");

  Serial.println("FrontDoorSwitch ...v" + String(ver));
  delay(300);

  espiot.init(ver);
  espiot.SENSOR = "PIR";

  espiot.server.on("/info", HTTP_GET, []() {
    espiot.blink();
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    root["RELEY"] = RELEY;
    root["BUILTINLED"] = BUILTINLED;
    root["PIRPIN"] = PIRPIN;

    String content;
    root.printTo(content);
    espiot.server.send(200, "application/json", content);
  });

  espiot.server.on("/state", HTTP_GET, []() {
    espiot.blink();
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();

    root["id"] = espiot.getDeviceId();
    root["name"] = espiot.deviceName;
    root["mode"] = MODE;
    root["state"] = getState();

    root["light"] = sensorValue;

    root.printTo(Serial);

    String content;
    root.printTo(content);
    espiot.server.send(200, "application/json", content);
  });

  espiot.server.on("/switch/on", HTTP_GET, []() {
    espiot.blink();
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();

    stateOn("MANUAL");

    root["id"] = espiot.getDeviceId();
    root["name"] = espiot.deviceName;
    root["mode"] = MODE;
    root["state"] = getState();
    root["light"] = sensorValue;

    root.printTo(Serial);

    String content;
    root.printTo(content);
    espiot.server.send(200, "application/json", content);
  });

  espiot.server.on("/switch/off", HTTP_GET, []() {
    espiot.blink();
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();

    stateOff("MANUAL");

    root["id"] = espiot.getDeviceId();
    root["name"] = espiot.deviceName;
    root["mode"] = MODE;
    root["state"] = getState();
    root["light"] = sensorValue;

    root.printTo(Serial);

    String content;
    root.printTo(content);
    espiot.server.send(200, "application/json", content);
  });

  espiot.server.on("/switch/auto", HTTP_GET, []() {
    espiot.blink();
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();

    stateAuto();

    root["id"] = espiot.getDeviceId();
    root["name"] = espiot.deviceName;
    root["mode"] = MODE;
    root["state"] = getState();
    root["light"] = sensorValue;

    root.printTo(Serial);

    String content;
    root.printTo(content);
    espiot.server.send(200, "application/json", content);
  });

} //--

// -----------------------------------------------------------------------------
// loop ------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void loop() {
  yield();
  sensorValue = analogRead(A0); // read analog input pin 0
  int inputState = digitalRead(PIRPIN);

  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  root["id"] = espiot.getDeviceId();
  root["name"] = espiot.deviceName;
  root["state"] = getState();
  root["light"] = sensorValue;
  root["motion"] = false;

  if (MODE == "AUTO") {
    if (inputState == HIGH) {
      Serial.println(F("Sensor high..."));

      if(sensorValue < espiot.lightThreshold){
          stateOn("AUTO");
      }else{
          Serial.println("Trashold preventing on state..." + String(espiot.lightThreshold));
      }

      lastTime = millis();
    } else{
        root["motion"] = false;
    }
  }

  yield();

  if (inputState == HIGH && !sentMsg) {
      String content;
      root["motion"] = true;
      root.printTo(content);
      espiot.mqPublishSubTopic(content, "motion");
      sentMsg = true;
  }

  if (millis() > lastTime + espiot.timeOut && inputState != HIGH) {
    stateOff("AUTO");

    Serial.println("timeout... " + String(lastTime));
    Serial.print("Light: ");
    Serial.println(sensorValue, DEC);

    String content;
    root.printTo(content);
    if (!sentMsg){
        espiot.mqPublish(content);
    }

    sentMsg = false;
    lastTime = millis();
  }
  yield();
  espiot.loop();
  delay(100);

} //---------------------------------------------------------------

void stateOn(String mode){
    Serial.println(F("\nSwitch on ..."));
    digitalWrite(BUILTINLED, LOW);
    digitalWrite(RELEY, HIGH);
    MODE = mode;
}
void stateOff(String mode){
    Serial.println(F("\nSwitch off ..."));
    digitalWrite(BUILTINLED, HIGH);
    digitalWrite(RELEY, LOW);
    MODE = mode;
}
void stateAuto(){
    Serial.println(F("\nSwitch to auto ..."));
    MODE = "AUTO";
}

String getState(){
    if(digitalRead(RELEY) == HIGH)
        return "on";
    else
        return "off";
}
