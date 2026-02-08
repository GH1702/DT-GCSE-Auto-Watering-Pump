// main.cpp - ESP32 Pump Controller
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "index_html.h"

// ---------- Boot mode ----------
enum BootMode { MODE_AP, MODE_MQTT };
BootMode bootMode;
#define MODE_PIN 13 

// --------- Config ----------
const char* ssid       = "BT-ZMAKN3";
const char* password   = "Q4XfNUMLDQpFrN";
const char* mqtt_server = "192.168.1.222";
const char* mqtt_user   = "esp_mqtt";
const char* mqtt_pass   = "Mosquito";
const String baseTopic = "esp32/pumps/";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define TRIG_PIN 16
#define ECHO_PIN 17

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);
WiFiClient espClient;
PubSubClient mqtt(espClient);
WebServer server(80);

// ---------- Globals ----------
unsigned long pumpOffTime[4] = {0, 0, 0, 0};
int pumpPins[4]      = {26, 25, 33, 32};
int moisturePins[4]  = {35, 34, 39, 36};
bool pumpState[4]    = {false, false, false, false};
int moisturePercent[4] = {0,0,0,0};
int waterLevelPercent = 0; 
unsigned long lastWaterRead = 0;
unsigned long lastMoistureRead = 0;
unsigned long lastMoistureMQTT = 0;
unsigned long lastClockUpdate = 0;
unsigned long lastWebAccessTime = 0;
// ---------- Calibration ----------
int wetMin[4] = {950, 950, 950, 950};
int dryMax[4] = {3300, 3300, 3300, 3300};

// ---------- Functions ----------

int readAveragedADC(int pin) {
  long total = 0;
  for (int i = 0; i < 10; i++) { total += analogRead(pin); delay(2); }
  return total / 10;
}

int mapMoistToPercent(int raw, int idx) {
  if (raw <= wetMin[idx]) return 100;
  if (raw >= dryMax[idx]) return 0;
  float pct = 100.0f * (float)(dryMax[idx] - raw) / (float)(dryMax[idx] - wetMin[idx]);
  return (int)constrain(pct, 0, 100);
}

void publishPumpState(int idx) {
  String topic = baseTopic + String(idx + 1) + "/state";
  mqtt.publish(topic.c_str(), pumpState[idx] ? "ON" : "OFF", true);
}

void publishAllPumpStates() { for (int i=0;i<4;i++) publishPumpState(i); }

void publishMoistureValuesMQTT() {
  for (int i=0;i<4;i++) {
    String topic = baseTopic + "moisture" + String(i + 1);
    mqtt.publish(topic.c_str(), String(moisturePercent[i]).c_str(), true);
  }
}

void handleSyncTime() {
  if (server.hasArg("epoch")) {
    long epoch = server.arg("epoch").toInt();
    // Note: NTPClient doesn't have a 'set' function easily, 
    // but we can use the built-in ESP32 internal clock:
    struct timeval tv;
    tv.tv_sec = epoch;
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);
    server.send(200, "text/plain", "OK");
  }
}

void handleStatus() {
  lastWebAccessTime = millis(); // Update the timer whenever the phone/laptop asks for data
  String json = "{\"water\":" + String(waterLevelPercent) + ",";
  json += "\"m\":[" + String(moisturePercent[0]) + "," + String(moisturePercent[1]) + "," 
                    + String(moisturePercent[2]) + "," + String(moisturePercent[3]) + "]}";
  server.send(200, "application/json", json);
}

void handleRoot() { server.send(200, "text/html", INDEX_HTML); }

void handlePump() {
  if (server.hasArg("p") && server.hasArg("t")) {
    int p = server.arg("p").toInt() - 1;
    int s = server.arg("t").toInt();
    if (p >= 0 && p < 4 && s > 0) {
      digitalWrite(pumpPins[p], LOW); pumpState[p] = true;
      pumpOffTime[p] = millis() + (unsigned long)s * 1000UL;
      publishPumpState(p);
    }
  }
  if (server.hasArg("on")) {
    int p = server.arg("on").toInt() - 1;
    if (p >= 0 && p < 4) { digitalWrite(pumpPins[p], LOW); pumpState[p] = true; pumpOffTime[p] = 0; publishPumpState(p); }
  }
  if (server.hasArg("off")) {
    int p = server.arg("off").toInt() - 1;
    if (p >= 0 && p < 4) { digitalWrite(pumpPins[p], HIGH); pumpState[p] = false; pumpOffTime[p] = 0; publishPumpState(p); }
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void drawOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // 1. Water Level
  display.setCursor(0, 0);
  display.print("Water: "); display.print(waterLevelPercent); display.print("%");

  // 2. INTERNAL System Time (Fixed to show laptop sync)
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char timeStringBuff[9]; 
    strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M:%S", &timeinfo);
    int16_t x1, y1; uint16_t w, h;
    display.getTextBounds(timeStringBuff, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(SCREEN_WIDTH - w, 0);
    display.print(timeStringBuff);
  }

  // 3. Linked Chains Icon (Moved 2px Right to x=62)
  if (millis() - lastWebAccessTime < 5000) {
    // Left link
    display.drawCircle(62, 3, 2, SSD1306_WHITE); 
    // Right link (overlapping slightly)
    display.drawCircle(65, 3, 2, SSD1306_WHITE); 
  }

  // 4. Pump and Moisture Rows
  for (int i = 0; i < 4; i++) {
    int y = 16 + i * 12;
    display.setCursor(0, y);
    display.print("P"); display.print(i + 1); display.print(": "); display.print(pumpState[i] ? "ON" : "OFF");
    display.setCursor(64, y);
    display.print("M"); display.print(i + 1); display.print(": "); display.print(moisturePercent[i]); display.print("%");
  }
  display.display();
}


void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg; for (int i=0; i<length; i++) msg += (char)payload[i];
  for (int i=0; i<4; i++) {
    if (String(topic) == (baseTopic + String(i+1) + "/set")) {
      if (msg.equalsIgnoreCase("ON")) { digitalWrite(pumpPins[i], LOW); pumpState[i] = true; publishPumpState(i); }
      else if (msg.equalsIgnoreCase("OFF")) { digitalWrite(pumpPins[i], HIGH); pumpState[i] = false; publishPumpState(i); }
    }
  }
}

void reconnectMQTT() {
  while (!mqtt.connected()) {
    if (mqtt.connect("ESP32_Pump_Controller", mqtt_user, mqtt_pass)) {
      for (int i=0; i<4; i++) { mqtt.subscribe((baseTopic + String(i+1) + "/set").c_str()); }
      publishAllPumpStates();
    } else { delay(2000); }
  }
}

long readWaterDistanceCM() {
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  return (duration == 0) ? -1 : duration / 58;
}

void setup() {
  Serial.begin(115200);
  pinMode(MODE_PIN, INPUT);
  bootMode = (digitalRead(MODE_PIN) == HIGH) ? MODE_MQTT : MODE_AP;

  for (int i=0; i<4; i++) { pinMode(pumpPins[i], OUTPUT); digitalWrite(pumpPins[i], HIGH); }
  pinMode(TRIG_PIN, OUTPUT); pinMode(ECHO_PIN, INPUT);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.print("Booting...");
  display.display();

  if (bootMode == MODE_AP) {
    WiFi.softAP("ESP32-Watering", "watering123");
    MDNS.begin("plant");
  } else {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); }
    mqtt.setServer(mqtt_server, 1883);
    mqtt.setCallback(mqttCallback);
    reconnectMQTT();
    timeClient.begin();
  }

  server.on("/", handleRoot);
  server.on("/pump", handlePump);
  server.on("/status", handleStatus);
  server.on("/sync", handleSyncTime);
  server.begin();
  drawOLED();
}

void loop() {
  unsigned long now = millis();
  server.handleClient();

  // 1. Pump Off Timer
  for (int i = 0; i < 4; i++) {
    if (pumpState[i] && pumpOffTime[i] > 0 && now >= pumpOffTime[i]) {
      digitalWrite(pumpPins[i], HIGH); pumpState[i] = false; pumpOffTime[i] = 0; publishPumpState(i);
    }
  }

  // 2. Water Distance Sensor
  if (now - lastWaterRead > 2000) {
    lastWaterRead = now;
    long d = readWaterDistanceCM();
    if (d > 0) waterLevelPercent = map(constrain(d, 5, 30), 30, 5, 0, 100);
  }

  // 3. Moisture Sensors (Global - runs in both modes)
  if (now - lastMoistureRead >= 1000) {
    for (int i=0; i<4; i++) {
      moisturePercent[i] = mapMoistToPercent(readAveragedADC(moisturePins[i]), i);
    }
    lastMoistureRead = now;
  }

  // 4. MQTT Mode specific logic
  if (bootMode == MODE_MQTT) {
    if (!mqtt.connected()) reconnectMQTT();
    mqtt.loop();
    if (now - lastMoistureMQTT >= 5000) { publishMoistureValuesMQTT(); lastMoistureMQTT = now; }
    if (now - lastClockUpdate >= 1000) { timeClient.update(); lastClockUpdate = now; }
  }

  // 5. OLED Refresh
  static unsigned long lastOledDraw = 0;
  if (now - lastOledDraw >= 1000) { lastOledDraw = now; drawOLED(); }
}