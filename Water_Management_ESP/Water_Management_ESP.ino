#include <WiFi.h>
#include <HTTPClient.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>

// === WiFi Config ===
const char* ssid = "Nasty";
const char* password = "Nasty@@@123";
const char* serverName = "http://192.168.0.108:5000/predict";

// === Pins ===
#define LED_GREEN    19
#define LED_YELLOW   18
#define LED_RED      23
#define BUZZER_PIN   5
#define SERVO_PIN    2

Servo gateServo;
HardwareSerial unoSerial(2);          // GPIO16 RX from UNO TX
U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE, 22, 21);
String serialBuffer = "";

void setup() {
  Serial.begin(115200);
  unoSerial.begin(9600, SERIAL_8N1, 16, 17);  // RX=16, TX=17 (TX not needed)

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  gateServo.attach(SERVO_PIN);
  gateServo.write(0);

  display.begin();
  display.setFont(u8g2_font_ncenR08_tr);
  display.clearBuffer();
  display.drawStr(10, 20, "WiFi Connecting...");
  display.sendBuffer();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  display.clearBuffer();
  display.drawStr(0, 20, "WiFi Connected!");
  display.sendBuffer();

  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

}

void loop() {
  while (unoSerial.available()) {
    char c = unoSerial.read();
    serialBuffer += c;
    if (c == '\n') {
      processSensorData(serialBuffer);
      serialBuffer = "";
    }
  }
}

void processSensorData(String jsonData) {
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, jsonData);
  if (err) { Serial.println("JSON Error"); return; }

  float temp = doc["temp"];
  float hum = doc["humidity"];
  int level = doc["level"];
  int flow = doc["flow"];
  int soil = doc["soil_analog"];
  int rainD = doc["rain_digital"];

  displayData("Sending...", temp, hum, flow, soil, rainD);

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    int httpCode = http.POST(jsonData);
    Serial.printf("HTTP CODE: %d\n", httpCode);

    if (httpCode == 200) {
      String response = http.getString();
      StaticJsonDocument<128> res;
      deserializeJson(res, response);
      String risk = res["risk"] | "UNKNOWN";
      displayData(risk, temp, hum, flow, soil, rainD);
      handleRisk(risk);
    }

    http.end();
  } else {
    Serial.println("WiFi down");
  }
}

void displayData(String risk, float temp, float hum, int flow, int soil, int rain) {
  display.clearBuffer();
  display.setCursor(0, 10);  display.print("Risk: "); display.print(risk);
  display.setCursor(0, 20);  display.print("Temp: "); display.print(temp); display.print(" C");
  display.setCursor(0, 30);  display.print("Hum: "); display.print(hum); display.print(" %");
  display.setCursor(0, 40);  display.print("Flow: "); display.print(flow); display.print(" L/m");
  display.setCursor(0, 50);  display.print("Rain: "); display.print(rain == LOW ? "YES" : "NO");
  display.setCursor(0, 60);  display.print("Soil: "); display.print((soil < 500) ? "WET" : (soil > 1000) ? "DRY" : "MOIST");
  display.sendBuffer();
}

void handleRisk(String risk) {
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  int angle = 0;

  if (risk == "LOW")       { digitalWrite(LED_GREEN, HIGH); angle = 0;     }
  else if (risk == "MODERATE") { digitalWrite(LED_YELLOW, HIGH); digitalWrite(BUZZER_PIN, HIGH); angle = 45;  }
  else if (risk == "HIGH")     { digitalWrite(LED_RED, HIGH); digitalWrite(BUZZER_PIN, HIGH); angle = 90;  }

  gateServo.write(angle);
}