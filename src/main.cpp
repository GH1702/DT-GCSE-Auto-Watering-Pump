#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include "index_html.h"
#include "Routines.h"

// ---------- Boot mode ----------
enum BootMode { MODE_MQTT, MODE_WIFI, MODE_AP };
BootMode bootMode;

#define SWITCH_PIN_I   13 
#define SWITCH_PIN_II  12 

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

// ---------- Objects ----------
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);
WiFiClient espClient;
PubSubClient mqtt(espClient);
WebServer server(80);
Preferences preferences;

// ---------- Globals ----------
int pumpPins[4]      = {26, 25, 33, 32};
int moisturePins[4]  = {35, 34, 39, 36};
bool pumpActive[4]   = {false, false, false, false};
unsigned long pumpStartTime[4] = {0, 0, 0, 0};
unsigned long pumpOffTime[4]   = {0, 0, 0, 0};

int moisturePercent[4]   = {0,0,0,0};
int rawMoistureValues[4] = {0,0,0,0};
int wetMin[4] = {950, 950, 950, 950};
int dryMax[4] = {3300, 3300, 3300, 3300};

int waterMinCM = 30;
int waterMaxCM = 5;
int waterLevelPercent = 0;

unsigned long lastWaterRead     = 0;
unsigned long lastMoistureRead  = 0;
unsigned long lastMoistureMQTT  = 0;
unsigned long lastClockUpdate   = 0;
unsigned long lastWebAccessTime = 0;

int pumpTimeoutS = 10;
bool manualOverride = false;

// ============================================================
// ====================== UTILITIES ===========================
// ============================================================

int readFastADC(int pin) {
  long total = 0;
  for (int i = 0; i < 5; i++) { total += analogRead(pin); }
  return total / 5;
}

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

long readWaterDistanceCM() {
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 20000); // Shorter timeout to prevent hang
  return (duration == 0) ? -1 : duration / 58;
}

// ============================================================
// ====================== PUMP LOGIC ==========================
// ============================================================

void publishPumpState(int idx) {
  if (bootMode != MODE_MQTT || !mqtt.connected()) return;
  String topic = baseTopic + String(idx + 1) + "/state";
  mqtt.publish(topic.c_str(), pumpActive[idx] ? "ON" : "OFF", true);
}

void activatePump(int idx, int seconds = 0) {
  if (idx < 0 || idx >= 4) return;
  
  digitalWrite(pumpPins[idx], LOW); 
  pumpActive[idx] = true;
  pumpStartTime[idx] = millis();

  if (!manualOverride) {
    unsigned long maxDuration = (seconds == 0 || seconds > pumpTimeoutS) ? (unsigned long)pumpTimeoutS : (unsigned long)seconds;
    pumpOffTime[idx] = millis() + (maxDuration * 1000UL);
  } else {
    pumpOffTime[idx] = (seconds > 0) ? (millis() + (unsigned long)seconds * 1000UL) : 0;
  }
  publishPumpState(idx);
}

void stopPump(int idx) {
  digitalWrite(pumpPins[idx], HIGH);
  pumpActive[idx] = false;
  pumpOffTime[idx] = 0;
  publishPumpState(idx);
}

// ============================================================
// ====================== WEB HANDLERS ========================
// ============================================================

void handleStatus() {
  lastWebAccessTime = millis();
  String json = "{\"water\":" + String(waterLevelPercent) + ",";
  json += "\"m\":[" + String(moisturePercent[0]) + "," + String(moisturePercent[1]) + "," 
                    + String(moisturePercent[2]) + "," + String(moisturePercent[3]) + "],";
  json += "\"raw\":[" + String(rawMoistureValues[0]) + "," + String(rawMoistureValues[1]) + "," 
                      + String(rawMoistureValues[2]) + "," + String(rawMoistureValues[3]) + "]}";
  server.send(200, "application/json", json);
}

void handlePump() {
  if (server.hasArg("on")) activatePump(server.arg("on").toInt() - 1);
  else if (server.hasArg("off")) stopPump(server.arg("off").toInt() - 1);
  else if (server.hasArg("p") && server.hasArg("t")) activatePump(server.arg("p").toInt() - 1, server.arg("t").toInt());
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleCalibrate() {
  if (server.hasArg("type")) {
    String type = server.arg("type");
    preferences.begin("calib", false);
    if (type == "override") manualOverride = (server.arg("val").toInt() == 1);
    else if (type == "setLimit") { pumpTimeoutS = server.arg("val").toInt(); preferences.putInt("pTimeout", pumpTimeoutS); }
    // Add other calibration types here as needed
    preferences.end();
    server.send(200, "text/plain", "OK");
  }
}

// ============================================================
// ====================== SYSTEM CORE =========================
// ============================================================

void reconnectMQTT() {
  static unsigned long lastAttempt = 0;
  if (millis() - lastAttempt < 5000) return;
  lastAttempt = millis();
  if (mqtt.connect("ESP32_Pump_Controller", mqtt_user, mqtt_pass)) {
    for (int i=0; i<4; i++) {
      mqtt.subscribe((baseTopic + String(i+1) + "/set").c_str());
      publishPumpState(i);
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg = ""; for (int i=0; i<length; i++) msg += (char)payload[i];
  for (int i=0; i<4; i++) {
    if (String(topic) == (baseTopic + String(i+1) + "/set")) {
      if (msg.equalsIgnoreCase("ON")) activatePump(i);
      else if (msg.equalsIgnoreCase("OFF")) stopPump(i);
    }
  }
}

void drawOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Water: "); display.print(waterLevelPercent); display.print("%");
  
  for (int i = 0; i < 4; i++) {
    int y = 16 + i * 12;
    display.setCursor(0, y);
    display.printf("P%d: %s  M%d: %d%%", i+1, pumpActive[i]?"ON":"OFF", i+1, moisturePercent[i]);
  }
  display.display();
}

void setup() {
  Serial.begin(115200);
  
  pinMode(SWITCH_PIN_I, INPUT_PULLUP);
  pinMode(SWITCH_PIN_II, INPUT_PULLUP);
  delay(50);
  if (digitalRead(SWITCH_PIN_I) == LOW) bootMode = MODE_MQTT;
  else if (digitalRead(SWITCH_PIN_II) == LOW) bootMode = MODE_AP;
  else bootMode = MODE_WIFI;

  for (int i=0; i<4; i++) { 
    pinMode(pumpPins[i], OUTPUT); 
    digitalWrite(pumpPins[i], HIGH); 
  }
  pinMode(TRIG_PIN, OUTPUT); pinMode(ECHO_PIN, INPUT);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();

  preferences.begin("calib", true);
  pumpTimeoutS = preferences.getInt("pTimeout", 10);
  // Load wet/dry values here...
  preferences.end();

  if (bootMode == MODE_AP) {
    WiFi.softAP("ESP32-Watering", "watering123");
  } else {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
    if (bootMode == MODE_MQTT) {
      mqtt.setServer(mqtt_server, 1883);
      mqtt.setCallback(mqttCallback);
      timeClient.begin();
    }
  }

  server.on("/", []() { server.send(200, "text/html", INDEX_HTML); });
  server.on("/status", handleStatus);
  server.on("/pump", handlePump);
  server.on("/calibrate", handleCalibrate);
  server.begin(); 
  Serial.println("System Ready");
}

void loop() {
  unsigned long now = millis();
  server.handleClient();

  if (bootMode == MODE_MQTT) {
    if (!mqtt.connected()) reconnectMQTT();
    mqtt.loop();
  }

  // --- Pump Safety & Timers ---
  for (int i = 0; i < 4; i++) {
    if (pumpActive[i]) {
      bool timedOut = (pumpOffTime[i] > 0 && now >= pumpOffTime[i]);
      bool safetyHit = (!manualOverride && (now - pumpStartTime[i] > (unsigned long)pumpTimeoutS * 1000UL));
      if (timedOut || safetyHit) stopPump(i);
    }
  }

  // --- Background Sensors (Staggered) ---
  if (now - lastMoistureRead > 2000) {
    for (int i = 0; i < 4; i++) {
      rawMoistureValues[i] = readFastADC(moisturePins[i]); // NO DELAY VERSION
      moisturePercent[i] = mapMoistToPercent(rawMoistureValues[i], i);
    }
    lastMoistureRead = now;
  }

  if (now - lastWaterRead > 3000) {
    long d = readWaterDistanceCM();
    if (d > 0) waterLevelPercent = map(constrain(d, waterMaxCM, waterMinCM), waterMinCM, waterMaxCM, 0, 100);
    lastWaterRead = now;
  }

  static unsigned long lastDraw = 0;
  if (now - lastDraw > 1500) { drawOLED(); lastDraw = now; }
}