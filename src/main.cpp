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
#include <Preferences.h>
Preferences preferences;

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

void publishpumpActive(int idx);
unsigned long pumpOffTime[4] = {0, 0, 0, 0};
int pumpPins[4]      = {26, 25, 33, 32};
int moisturePins[4]  = {35, 34, 39, 36};
bool pumpActive[4] = {false, false, false, false}; // Renamed from pumpActive
unsigned long pumpStartTime[4] = {0, 0, 0, 0};     // Added this
int moisturePercent[4] = {0,0,0,0};
int waterLevelPercent = 0; 
unsigned long lastWaterRead = 0;
unsigned long lastMoistureRead = 0;
unsigned long lastMoistureMQTT = 0;
unsigned long lastClockUpdate = 0;
unsigned long lastWebAccessTime = 0;
int pumpTimeoutS = 10; // 10 seconds
bool manualOverride = false; // System starts in safe mode


// ---------- Calibration ----------
int wetMin[4] = {950, 950, 950, 950};
int dryMax[4] = {3300, 3300, 3300, 3300};
// Add these to your calibration globals
int waterMinCM = 30; // Default: 30cm is empty
int waterMaxCM = 5;  // Default: 5cm is full

// ---------- Functions ----------
void activatePump(int idx, int seconds = 0) {
  if (idx < 0 || idx >= 4) return;

  digitalWrite(pumpPins[idx], LOW); 
  pumpActive[idx] = true;
  pumpStartTime[idx] = millis();

  // If NOT in manual override, force a 10s limit
  if (!manualOverride) {
    // If the requested time 's' is 0 (Manual ON) or > 10, cap it at 10
    if (seconds == 0 || seconds > 10) {
      pumpOffTime[idx] = millis() + 10000UL; 
    } else {
      pumpOffTime[idx] = millis() + (unsigned long)seconds * 1000UL;
    }
  } else {
    // In Manual Override, use the requested time or 0 for infinite
    if (seconds > 0) {
      pumpOffTime[idx] = millis() + (unsigned long)seconds * 1000UL;
    } else {
      pumpOffTime[idx] = 0; 
    }
  }
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

void publishpumpActive(int idx) {
  String topic = baseTopic + String(idx + 1) + "/state";
  mqtt.publish(topic.c_str(), pumpActive[idx] ? "ON" : "OFF", true);
}

void publishAllpumpActives() { for (int i=0;i<4;i++) publishpumpActive(i); }

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

long readWaterDistanceCM() {
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  return (duration == 0) ? -1 : duration / 58;
}

void handleStatus() {
  lastWebAccessTime = millis();
  String json = "{\"water\":" + String(waterLevelPercent) + ",";
  json += "\"waterRaw\":" + String(readWaterDistanceCM()) + ","; // Add this
  json += "\"m\":[" + String(moisturePercent[0]) + "," + String(moisturePercent[1]) + "," 
                    + String(moisturePercent[2]) + "," + String(moisturePercent[3]) + "],";
  // Add raw moisture readings
  json += "\"raw\":[" + String(readAveragedADC(moisturePins[0])) + "," 
                      + String(readAveragedADC(moisturePins[1])) + "," 
                      + String(readAveragedADC(moisturePins[2])) + "," 
                      + String(readAveragedADC(moisturePins[3])) + "]}";
  server.send(200, "application/json", json);
}

void handleRoot() { server.send(200, "text/html", INDEX_HTML); }

void handlePump() {
  if (server.hasArg("on")) {
    activatePump(server.arg("on").toInt() - 1);
  }
  else if (server.hasArg("p") && server.hasArg("t")) {
    activatePump(server.arg("p").toInt() - 1, server.arg("t").toInt());
  }
  else if (server.hasArg("off")) {
    int p = server.arg("off").toInt() - 1;
    if (p >= 0 && p < 4) { 
      digitalWrite(pumpPins[p], HIGH); 
      pumpActive[p] = false; 
      pumpOffTime[p] = 0;
      if (bootMode == MODE_MQTT) publishpumpActive(p);
    }
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
    display.drawCircle(66, 3, 2, SSD1306_WHITE); 
    // Right link (overlapping slightly)
    display.drawCircle(69, 3, 2, SSD1306_WHITE); 
  }

  // 4. Pump and Moisture Rows
  for (int i = 0; i < 4; i++) {
    int y = 16 + i * 12;
    display.setCursor(0, y);
    display.print("P"); display.print(i + 1); display.print(": "); display.print(pumpActive[i] ? "ON" : "OFF");
    display.setCursor(64, y);
    display.print("M"); display.print(i + 1); display.print(": "); display.print(moisturePercent[i]); display.print("%");
  }
  display.display();
}


void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg; for (int i=0; i<length; i++) msg += (char)payload[i];
  for (int i=0; i<4; i++) {
    if (String(topic) == (baseTopic + String(i+1) + "/set")) {
      if (msg.equalsIgnoreCase("ON")) { 
        digitalWrite(pumpPins[i], LOW); 
        pumpActive[i] = true; 
        pumpStartTime[i] = millis(); // <--- ADD THIS LINE
        publishpumpActive(i); 
      }
      else if (msg.equalsIgnoreCase("OFF")) { 
        digitalWrite(pumpPins[i], HIGH); 
        pumpActive[i] = false; 
        publishpumpActive(i); 
      }
    }
  }
}

unsigned long lastMqttAttempt = 0;

void reconnectMQTT() {
  // Only attempt to connect if it's been 5 seconds since the last try
  if (millis() - lastMqttAttempt > 5000) {
    lastMqttAttempt = millis();
    Serial.println("Attempting MQTT connection...");
    
    if (mqtt.connect("ESP32_Pump_Controller", mqtt_user, mqtt_pass)) {
      Serial.println("MQTT Connected!");
      for (int i=0; i<4; i++) { 
        mqtt.subscribe((baseTopic + String(i+1) + "/set").c_str()); 
      }
      publishAllpumpActives();
    }
  }
}

void handleCalibrate() {
  if (server.hasArg("type")) {
    String type = server.arg("type");
    preferences.begin("calib", false);

    if (type == "resetMoist") {
      int idx = server.arg("sensor").toInt() - 1;
      wetMin[idx] = 950; dryMax[idx] = 3300;
      preferences.remove(("w" + String(idx)).c_str());
      preferences.remove(("d" + String(idx)).c_str());
    } 
    else if (type == "resetWater") {
      waterMinCM = 30; waterMaxCM = 5;
      preferences.remove("watMin"); preferences.remove("watMax");
    }
    else if (type == "setLimit") { // Added this for Safety Limit
      pumpTimeoutS = server.arg("val").toInt();
      preferences.putInt("pTimeout", pumpTimeoutS);
    }
    else if (server.hasArg("sensor") && server.hasArg("val")) {
      int val = server.arg("val").toInt();
      int idx = server.arg("sensor").toInt() - 1;
      if (type == "wet") { wetMin[idx] = val; preferences.putInt(("w" + String(idx)).c_str(), val); } 
      else if (type == "dry") { dryMax[idx] = val; preferences.putInt(("d" + String(idx)).c_str(), val); }
      else if (type == "waterFull") { waterMaxCM = val; preferences.putInt("watMax", val); }
      else if (type == "waterEmpty") { waterMinCM = val; preferences.putInt("watMin", val); }
    }
    else if (type == "override")
    {
      manualOverride = (server.arg("val").toInt() == 1);
      Serial.print("Manual Override: ");
      Serial.println(manualOverride ? "ENABLED" : "DISABLED");
    }

    preferences.end();
    server.send(200, "text/plain", "OK");
  }
}

void setup() {
  Serial.begin(115200);
  
  // 1. Initialize Pins
  pinMode(MODE_PIN, INPUT);
  bootMode = (digitalRead(MODE_PIN) == HIGH) ? MODE_MQTT : MODE_AP;

  for (int i=0; i<4; i++) { 
    pinMode(pumpPins[i], OUTPUT); 
    digitalWrite(pumpPins[i], HIGH); // Keep pumps OFF on boot (Active High relay)
  }
  pinMode(TRIG_PIN, OUTPUT); 
  pinMode(ECHO_PIN, INPUT);

  // 2. Start OLED Display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.print("Booting...");
  display.display();

  // 3. Load All Saved Settings from Permanent Memory
  preferences.begin("calib", true); // Open in Read-Only mode
  
  // Load Moisture Calibration
  for(int i=0; i<4; i++) {
    wetMin[i] = preferences.getInt(("w" + String(i)).c_str(), 950);
    dryMax[i] = preferences.getInt(("d" + String(i)).c_str(), 3300);
  }
  
  // Load Water Tank Calibration
  waterMinCM = preferences.getInt("watMin", 30);
  waterMaxCM = preferences.getInt("watMax", 5);

  // NEW: Load the Pump Safety Timeout (Default to 60 seconds if not set)
  pumpTimeoutS = preferences.getInt("pTimeout", 10);
  if (pumpTimeoutS < 1) pumpTimeoutS = 10;
  
  preferences.end(); // Close memory safely

  // 4. Network Setup
  if (bootMode == MODE_AP) {
    WiFi.softAP("ESP32-Watering", "watering123");
    MDNS.begin("plant");
    Serial.println("Started in AP Mode");
  } else {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { 
      delay(500); 
      Serial.print(".");
    }
    mqtt.setServer(mqtt_server, 1883);
    mqtt.setCallback(mqttCallback);
    reconnectMQTT();
    timeClient.begin();
    Serial.println("Connected to WiFi & MQTT");
  }

  // 5. Web Server Routes
  server.on("/", handleRoot);
  server.on("/pump", handlePump);
  server.on("/status", handleStatus);
  server.on("/sync", handleSyncTime);
  server.on("/calibrate", handleCalibrate); 
  
  server.begin();
  
  // 6. Initial OLED Draw
  drawOLED();
}

void loop()
{
  unsigned long now = millis();
  server.handleClient();

  // 1. Pump Management (Timers & Safety Watchdog)
  // Inside void loop() - Pump Management Section
  for (int i = 0; i < 4; i++)
  {
    if (pumpActive[i])
    {
      unsigned long now = millis();
      unsigned long elapsed = now - pumpStartTime[i];

      // 1. Check Timed Run (if pumpOffTime was set)
      bool timedRunFinished = (pumpOffTime[i] > 0 && now >= pumpOffTime[i]);

      // 2. Check Safety Watchdog (ONLY if NOT in manual override)
      // We use 10500ms to give it a tiny bit of breathing room
      bool safetyTimeoutHit = (!manualOverride && elapsed > 10500UL);

      if (timedRunFinished || safetyTimeoutHit)
      {
        digitalWrite(pumpPins[i], HIGH); // Physically Turn OFF
        pumpActive[i] = false;
        pumpOffTime[i] = 0;

        if (safetyTimeoutHit)
        {
          Serial.printf("SAFETY: Pump %d reached 10s limit and was killed.\n", i + 1);
        }
        if (bootMode == MODE_MQTT)
          publishpumpActive(i);
      }
    }
  }

  // 2. Water Distance Sensor
  if (now - lastWaterRead > 2000) {
    lastWaterRead = now;
    long d = readWaterDistanceCM();
    if (d > 0) {
      waterLevelPercent = map(constrain(d, waterMaxCM, waterMinCM), waterMinCM, waterMaxCM, 0, 100);
    }
  }

  // 3. Moisture Sensors
  if (now - lastMoistureRead >= 1000) {
    for (int i = 0; i < 4; i++) {
      moisturePercent[i] = mapMoistToPercent(readAveragedADC(moisturePins[i]), i);
    }
    lastMoistureRead = now;
  }

  // 4. Mode Specific Logic (MQTT/Clock)
  if (bootMode == MODE_MQTT) {
    if (!mqtt.connected()) reconnectMQTT();
    mqtt.loop();
    if (now - lastMoistureMQTT >= 5000) {
      publishMoistureValuesMQTT();
      lastMoistureMQTT = now;
    }
    if (now - lastClockUpdate >= 1000) {
      timeClient.update();
      lastClockUpdate = now;
    }
  }

  // 5. OLED Refresh
  static unsigned long lastOledDraw = 0;
  if (now - lastOledDraw >= 1000) {
    lastOledDraw = now;
    drawOLED();
  }
}