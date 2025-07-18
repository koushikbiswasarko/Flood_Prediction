#include <DHT.h>
#include <ArduinoJson.h>

// === Sensor Pins ===
#define DHTPIN 4
#define DHTTYPE DHT22

#define SOIL_AO_PIN A0
#define SOIL_DO_PIN 7
#define RAIN_AO_PIN A1
#define RAIN_DO_PIN 6
#define LEVEL_PIN A2
#define FLOW_PIN 2
#define SENSOR_POWER 8

volatile int flowPulses = 0;
DHT dht(DHTPIN, DHTTYPE);

void flowISR() {
  flowPulses++;
}

void setup() {
  Serial.begin(9600);
  dht.begin();

  pinMode(SOIL_DO_PIN, INPUT);
  pinMode(RAIN_DO_PIN, INPUT);
  pinMode(FLOW_PIN, INPUT_PULLUP);
  pinMode(SENSOR_POWER, OUTPUT);

  digitalWrite(SENSOR_POWER, HIGH);
  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), flowISR, RISING);
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int soilAnalog = analogRead(SOIL_AO_PIN);
  int soilDigital = digitalRead(SOIL_DO_PIN);
  int rainAnalog = analogRead(RAIN_AO_PIN);
  int rainDigital = digitalRead(RAIN_DO_PIN);
  int level = analogRead(LEVEL_PIN);

  noInterrupts();
  int flowRate = flowPulses * 60 / 7.5;
  flowPulses = 0;
  interrupts();

  StaticJsonDocument<256> doc;
  doc["temp"] = isnan(temp) ? 0 : temp;
  doc["humidity"] = isnan(hum) ? 0 : hum;
  doc["soil_analog"] = soilAnalog;
  doc["soil_digital"] = soilDigital;
  doc["rain_analog"] = rainAnalog;
  doc["rain_digital"] = rainDigital;
  doc["level"] = level;
  doc["flow"] = flowRate;

  serializeJson(doc, Serial);
  Serial.println();

  delay(3000);
}