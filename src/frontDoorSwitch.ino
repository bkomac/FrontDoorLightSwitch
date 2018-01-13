#include <Espiot.h>

Espiot espiot;

// Vcc measurement
//ADC_MODE(ADC_VCC);
int lightTreshold = 0; // 0 - dark, >100 - light

String sensorData = "";
int sensorValue;

// CONF
String ver = "1.0.0";
long lastTime = millis();

// PIR
#define PIRPIN 12

#define BUILTINLED 2
#define RELEY 13

String MODE = "AUTO";
boolean sentMsg = false;

void setup() { //------------------------------------------------
  Serial.begin(115200);

  pinMode(PIRPIN, INPUT);
  pinMode(RELEY, OUTPUT);
  pinMode(BUILTINLED, OUTPUT);


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
    if(digitalRead(PIRPIN) == HIGH)
        root["state"] = "on";
    else
        root["state"] = "off";

    root["light"] = sensorValue;

    JsonObject &rgb = root.createNestedObject("rgb");

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

  espiot.loop();

  sensorValue = analogRead(A0); // read analog input pin 0
  int inputState = digitalRead(PIRPIN);
  yield();

  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  root["id"] = espiot.getDeviceId();
  root["name"] = espiot.deviceName;
  root["light"] = sensorValue;
  root["motion"] = NULL;

  yield();

  if (MODE == "AUTO") {
    if (inputState == HIGH) {
      Serial.println(F("Sensor high..."));
      digitalWrite(BUILTINLED, LOW);
      digitalWrite(RELEY, HIGH);

      lastTime = millis();
      if(!sentMsg){
        String content;
        root["motion"] = true;
        root.printTo(content);
        espiot.mqPublishSubTopic(content, "motion");
        sentMsg = true;
      }
    } else{
        root["motion"] = false;
    }
  }

  if (millis() > lastTime + espiot.timeOut && inputState != HIGH) {
    digitalWrite(RELEY, LOW);
    digitalWrite(BUILTINLED, HIGH);

    sentMsg = false;
    Serial.println("timeout... " + String(lastTime));
    Serial.print("Light: ");
    Serial.println(sensorValue, DEC);

    String content;
    root.printTo(content);
    espiot.mqPublish(content);

    lastTime = millis();
  }
  delay(200);
} //---------------------------------------------------------------
