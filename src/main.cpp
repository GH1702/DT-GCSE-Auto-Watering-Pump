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
#include <ArduinoJson.h>
#include "index_html.h"
#include "style_css.h"
#include "script_js.h"
#include "Routines.h"
#include <Adafruit_VL53L0X.h>
#include <Adafruit_BMP280.h>
#include <RTClib.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>


// ---------- Boot mode ----------
enum BootMode { MODE_MQTT, MODE_WIFI, MODE_AP };
BootMode bootMode;

// ---------- Pin definitions ----------
#define SWITCH_PIN_I   13 
#define SWITCH_PIN_II  12 
#define LED_PIN   27
#define NUM_LEDS  32
#define LID_PIN 14

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

// ---------- LED Config----------
Adafruit_NeoPixel ring(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
enum LedMode { LED_WAVE, LED_SOLID, LED_SMART };
LedMode currentLedMode = LED_WAVE;   // 🌊 DEFAULT MODE
float waveOffset = 0.0;
float waveSpeed = 0.08;
float waveLength = 20.0;

// ---------- Objects ----------
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);
WiFiClient espClient;
PubSubClient mqtt(espClient);
WebServer server(80);
Preferences preferences;


// Sensor objects
Adafruit_VL53L0X lox = Adafruit_VL53L0X();
Adafruit_BMP280 bmp;
RTC_DS3231 rtc;

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

StorageManager storage;
CalibrationManager calibration;
RoutineExecutor routineExec;
AutomationExecutor automationExec;
String routinesCache;
String automationsCache;

int waterMinCM = 30;
int waterMaxCM = 5;
int waterLevelPercent = 0;

unsigned long lastWaterRead     = 0;
unsigned long lastMoistureRead  = 0;
unsigned long lastMoistureMQTT  = 0;
unsigned long lastClockUpdate   = 0;
unsigned long lastWebAccessTime = 0;
unsigned long lastRoutineCheck = 0;
unsigned long lastAutomationCheck = 0;
int lastMinute = -1;
long waterLevelCM = -1;

int pumpTimeoutS = 10;
bool manualOverride = false;

// ============================================================
// =================== FORWARD DECLARATIONS ===================
// ============================================================
void sendWhatsAppMessage(String message);
void pumpActionCallback(int pump, int duration);
void whatsappActionCallback(String message);
void activatePump(int idx, int seconds);  // NO DEFAULT HERE
void stopPump(int idx);

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
  VL53L0X_RangingMeasurementData_t measure;
  lox.rangingTest(&measure, false);

  if (measure.RangeStatus != 4 && measure.RangeMilliMeter > 0) {
    long cm = measure.RangeMilliMeter / 10;

    // Apply a simple linear offset correction
    if (cm <= 10)      cm -= 3;
    else if (cm <= 20) cm -= 2;
    else if (cm <= 30) cm -= 1;

    return cm;
  }

  return -1; // invalid reading
}

long getSmoothedWaterDistance() {
  long total = 0;
  int valid = 0;

  for (int i = 0; i < 5; i++) {
    long d = readWaterDistanceCM();
    if (d > 0) {
      total += d;
      valid++;
    }
    delay(10);
  }

  if (valid == 0) return -1;
  return total / valid;
}

long updateWaterLevel() {
    long d = readWaterDistanceCM();
    if (d > 0) {
        waterLevelPercent = map(constrain(d, waterMaxCM, waterMinCM), waterMinCM, waterMaxCM, 0, 100);
    }
    return d;  // ✅ Return the distance
}

float readTemperatureC() {
  return bmp.readTemperature();
}

float readPressureHPa() {
  return bmp.readPressure() / 100.0F;
}

String getCurrentTimeString() {
  DateTime now = rtc.now();
  
  char buffer[9];
  sprintf(buffer, "%02d:%02d:%02d",
          now.hour(),
          now.minute(),
          now.second());
          
  return String(buffer);
}

void syncRTCFromWiFi() {
  timeClient.update();
  unsigned long epochTime = timeClient.getEpochTime();
  
  // Convert epoch to DateTime and save to RTC hardware
  rtc.adjust(DateTime(epochTime)); 
  
  Serial.println("RTC Hardware Synced with NTP");
}

int getTankPercent() {
  long distance = readWaterDistanceCM();
  if (distance < 0) return -1;

  // Prevent divide-by-zero
  if (waterMinCM == waterMaxCM) return -1;

  float percent = 100.0 * (waterMinCM - distance) / (waterMinCM - waterMaxCM);

  percent = constrain(percent, 0, 100);
  return (int)percent;
}



// ============================================================
// ====================== PUMP LOGIC ==========================
// ============================================================

void publishPumpState(int idx) {
  if (bootMode != MODE_MQTT || !mqtt.connected()) return;
  String topic = baseTopic + String(idx + 1) + "/state";
  mqtt.publish(topic.c_str(), pumpActive[idx] ? "ON" : "OFF", true);
}

// DEFAULT VALUE ONLY IN IMPLEMENTATION
void activatePump(int idx, int durationSec) {
  if (idx < 0 || idx >= 4) return;

  pumpStartTime[idx] = millis();
  pumpActive[idx] = true;

  if (durationSec <= 0 || durationSec > pumpTimeoutS) durationSec = pumpTimeoutS;

  pumpOffTime[idx] = pumpStartTime[idx] + (unsigned long)durationSec * 1000UL;

  digitalWrite(pumpPins[idx], LOW); 
  publishPumpState(idx);
  Serial.printf("PUMP %d: Duration set to %d seconds\n", idx+1, durationSec);
}

void stopPump(int idx) {
  digitalWrite(pumpPins[idx], HIGH);
  pumpActive[idx] = false;
  pumpOffTime[idx] = 0;
  pumpStartTime[idx] = 0;
  publishPumpState(idx);
}

// ============================================================
// ================= ROUTINES AND AUTOMATIONS =================
// ============================================================

void handleGetRoutines() {
  String routines = storage.loadRoutines();
  server.send(200, "application/json", routines);
}

void handleSaveRoutines() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    
    if (storage.saveRoutines(body)) {
      server.send(200, "text/plain", "OK");
      Serial.println("Routines saved successfully");
    } else {
      server.send(500, "text/plain", "Save failed");
    }
  } else {
    server.send(400, "text/plain", "No data");
  }
}

void handleGetAutomations() {
  String automations = storage.loadAutomations();
  server.send(200, "application/json", automations);
}

void handleSaveAutomations() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    
    if (storage.saveAutomations(body)) {
      server.send(200, "text/plain", "OK");
      Serial.println("Automations saved successfully");
    } else {
      server.send(500, "text/plain", "Save failed");
    }
  } else {
    server.send(400, "text/plain", "No data");
  }
}

void handleRunRoutine() {
  if (server.hasArg("id")) {
    String routineId = server.arg("id");
    String routines = storage.loadRoutines();
    
    StaticJsonDocument<4096> doc;
    deserializeJson(doc, routines);
    JsonArray arr = doc.as<JsonArray>();
    
    for (JsonObject r : arr) {
      if (String(r["id"].as<long>()) == routineId) {
        int pump = r["p"] | 0;
        int duration = 10;
        
        if (r["mode"] == "static") {
          int amount = r["val"] | 100;
          duration = amount / 10;
        } else {
          duration = 10;
        }
        
        activatePump(pump - 1, duration);
        server.send(200, "text/plain", "Routine executed");
        return;
      }
    }
    
    server.send(404, "text/plain", "Routine not found");
  } else {
    server.send(400, "text/plain", "No ID");
  }
}

void handleRunAutomation() {
  if (server.hasArg("id")) {
    String autoId = server.arg("id");
    String automations = storage.loadAutomations();
    
    StaticJsonDocument<4096> doc;
    deserializeJson(doc, automations);
    JsonArray arr = doc.as<JsonArray>();
    
    for (JsonObject a : arr) {
      if (String(a["id"].as<long>()) == autoId) {
        JsonObject doAction = a["do"];
        String type = doAction["type"] | "";
        
        if (type == "whatsapp") {
          String message = doAction["message"] | "";
          sendWhatsAppMessage(message);
        }
        else if (type == "pump_on") {
          int pump = doAction["pump"] | 1;
          int duration = doAction["duration"] | 10;
          activatePump(pump - 1, duration);
        }
        else if (type == "pump_off") {
          int pump = doAction["pump"] | 1;
          stopPump(pump - 1);
        }
        
        server.send(200, "text/plain", "Automation executed");
        return;
      }
    }
    
    server.send(404, "text/plain", "Automation not found");
  } else {
    server.send(400, "text/plain", "No ID");
  }
}

void handleStorageInfo() {
  String json = "{";
  json += "\"total\":" + String(SPIFFS.totalBytes()) + ",";
  json += "\"used\":" + String(SPIFFS.usedBytes()) + ",";
  json += "\"free\":" + String(SPIFFS.totalBytes() - SPIFFS.usedBytes());
  json += "}";
  server.send(200, "application/json", json);
}

void loadRoutinesCache() {
    routinesCache = storage.loadRoutines();   // returns a String
}

void loadAutomationsCache() {
    automationsCache = storage.loadAutomations(); // returns a String
}

// ============================================================
// ====================== VISUAL FUNCTIONS ====================
// ============================================================

void drawWiFiIcon(int x, int y) {
  // 1. Draw the WiFi "Fan" Arcs
  // x, y is the bottom point (the dot)
  display.fillCircle(x, y, 1, SSD1306_WHITE);           // The bottom dot
  display.drawCircleHelper(x, y, 4, 1, SSD1306_WHITE);  // Small arc
  display.drawCircleHelper(x, y, 7, 1, SSD1306_WHITE);  // Medium arc
  display.drawCircleHelper(x, y, 10, 1, SSD1306_WHITE); // Large arc

  // 2. Draw the "WiFi" Text underneath
  display.setTextSize(1);
  // Center the text "WiFi" (about 24 pixels wide) relative to the icon x
  display.setCursor(x - 12, y + 4); 
  display.print("WiFi");
}

void drawAPIcon(int x, int y) {
  // 1. The Mast (Tower)
  display.drawLine(x, y, x, y - 8, SSD1306_WHITE);           // Vertical pole
  display.drawTriangle(x-2, y, x+2, y, x, y-2, SSD1306_WHITE); // Small base stand
  display.fillCircle(x, y - 9, 1, SSD1306_WHITE);            // Top transmitter dot

  // 2. Broadcast Waves (Left and Right)
  // Left arcs (using helpers for quadrants 2 and 3)
  display.drawCircleHelper(x, y - 6, 4, 2, SSD1306_WHITE); 
  display.drawCircleHelper(x, y - 6, 7, 2, SSD1306_WHITE);
  // Right arcs (using helpers for quadrants 1 and 4)
  display.drawCircleHelper(x, y - 6, 4, 1, SSD1306_WHITE);
  display.drawCircleHelper(x, y - 6, 7, 1, SSD1306_WHITE);

  // 3. "AP" Text underneath
  display.setTextSize(1);
  display.setCursor(x - 5, y + 2); 
  display.print("AP");
}

void drawMQTTIcon(int x, int y) {
  // 1. The Central Broker Node
  display.drawRect(x - 3, y - 7, 7, 7, SSD1306_WHITE); // Central box
  display.drawPixel(x, y - 4, SSD1306_WHITE);          // "Heart" of the broker

  // 2. Publish/Subscribe Arrows (The "Arms")
  // Top Left & Bottom Right (Inbound/Outbound)
  display.drawLine(x - 6, y - 10, x - 3, y - 7, SSD1306_WHITE); 
  display.drawLine(x + 3, y - 1, x + 6, y + 2, SSD1306_WHITE);
  
  // Top Right & Bottom Left
  display.drawLine(x + 3, y - 7, x + 6, y - 10, SSD1306_WHITE);
  display.drawLine(x - 6, y + 2, x - 3, y - 1, SSD1306_WHITE);

  // 3. "MQTT" Text underneath
  display.setTextSize(1);
  // MQTT is about 24 pixels wide, so we offset by 12 to center
  display.setCursor(x - 12, y + 4); 
  display.print("MQTT");
}

// Change sf++ to sf += 2 in your setup calls for even more speed
void bootSwirl(uint8_t frame) {
  ring.clear();
  // We use a multiplier or a larger frame increment to "jump" pixels
  for (int i = 0; i < 4; i++) {
    int pixel = (frame + i) % NUM_LEDS;
    uint8_t brightness = (i + 1) * 60; 
    ring.setPixelColor(pixel, ring.Color(brightness, brightness, 0)); 
  }
  ring.show();
}

void drawWave()
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    float angle = 2.0 * PI * (i + waveOffset) / waveLength;
    float wave = (sin(angle) * 0.5 + 0.5);

    float greenLevel = wave;
    float blueLevel  = 1.0 - wave;

    uint8_t brightness = 40 + 180 * wave;

    uint8_t r = 0;
    uint8_t g = brightness * greenLevel;
    uint8_t b = brightness * blueLevel;

    ring.setPixelColor(i, ring.Color(r, g, b));
  }

  ring.show();

  waveOffset += waveSpeed;
  if (waveOffset >= waveLength) waveOffset -= waveLength;
}

void drawBreathingRed() {
  static float breathValue = 0;
  static bool breathingUp = true;
  
  // Oscillate brightness between 20 and 200
  if (breathingUp) {
    breathValue += 2.5;
    if (breathValue >= 200) breathingUp = false;
  } else {
    breathValue -= 2.5;
    if (breathValue <= 20) breathingUp = true;
  }

  for (int i = 0; i < NUM_LEDS; i++) {
    ring.setPixelColor(i, ring.Color(breathValue, 0, 0)); // Pure Red
  }
  ring.show();
}

void drawProgressBar(int percent) {
  int x = 24;       // Left position
  int y = 44;       // Top position
  int w = 80;       // Total width
  int h = 10;       // Total height

  // Draw the outer box
  display.drawRect(x, y, w, h, SSD1306_WHITE);
  
  // Calculate the width of the progress fill
  int progress = map(percent, 0, 100, 0, w - 4);
  
  // Draw the inner fill (leaving a 2-pixel margin)
  display.fillRect(x + 2, y + 2, progress, h - 4, SSD1306_WHITE);
  display.display();
}

// ============================================================
// ====================== WHATSAPP FUNCTION ===================
// ============================================================

void sendWhatsAppMessage(String message) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected - cannot send WhatsApp");
    return;
  }
  
  message.replace("{tank}", String(waterLevelPercent) + "%");
  for (int i = 0; i < 4; i++) {
    message.replace("{sensor}", String(moisturePercent[i]) + "%");
    message.replace("{value}", String(moisturePercent[i]) + "%");
    message.replace("{pump}", String(i + 1));
  }
  
  String encodedMsg = "";
  for (unsigned int i = 0; i < message.length(); i++) {
    char c = message.charAt(i);
    if (c == ' ') encodedMsg += '+';
    else if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') encodedMsg += c;
    else {
      encodedMsg += '%';
      if (c < 16) encodedMsg += '0';
      encodedMsg += String(c, HEX);
    }
  }
  
  String url = "http://api.callmebot.com/whatsapp.php?";
  url += "phone=+447902664891";
  url += "&text=" + encodedMsg;
  url += "&apikey=4458318";
  
  Serial.println("Sending WhatsApp: " + message);
  
  WiFiClient client;
  if (client.connect("api.callmebot.com", 80)) {
    client.print(String("GET ") + url.substring(30) + " HTTP/1.1\r\n" +
                 "Host: api.callmebot.com\r\n" +
                 "Connection: close\r\n\r\n");
    
    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println("WhatsApp request timeout");
        client.stop();
        return;
      }
    }
    
    while (client.available()) {
      String line = client.readStringUntil('\r');
      Serial.print(line);
    }
    
    client.stop();
    Serial.println("\nWhatsApp message sent successfully");
  } else {
    Serial.println("Failed to connect to callmebot API");
  }
}

void pumpActionCallback(int pump, int duration) {
  if (duration == 0) {
    stopPump(pump);
  } else {
    activatePump(pump, duration);
  }
}

void whatsappActionCallback(String message) {
  sendWhatsAppMessage(message);
}

// ============================================================
// ====================== WEB HANDLERS ========================
// ============================================================

void handleRoot() {
  server.sendHeader("Cache-Control", "no-cache");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", ""); 

  server.sendContent_P(INDEX_HTML_START);
  
  // Wrap CSS
  server.sendContent_P(HTML_STYLE_OPEN);
  server.sendContent_P(STYLE_CSS);
  server.sendContent_P(HTML_STYLE_CLOSE);
  
  // Wrap JS
  server.sendContent_P(HTML_SCRIPT_OPEN);
  server.sendContent_P(SCRIPT_JS);
  server.sendContent_P(HTML_SCRIPT_CLOSE);
  
  server.sendContent_P(INDEX_HTML_BODY);
  server.sendContent_P(INDEX_HTML_FOOTER); // Close the tags properly
  
  server.sendContent("");
}

void handleStatus() {
  lastWebAccessTime = millis();
  
  float tempC = readTemperatureC();   // read temperature
  long waterRaw = readWaterDistanceCM();

  String json = "{";
  json += "\"water\":" + String(waterLevelPercent) + ",";
  json += "\"waterRaw\":" + String(waterRaw) + ",";
  json += "\"temperature\":" + String(tempC, 1) + ",";  // 1 decimal place
  json += "\"m\":[" + String(moisturePercent[0]) + "," + String(moisturePercent[1]) + "," 
                    + String(moisturePercent[2]) + "," + String(moisturePercent[3]) + "],";
  json += "\"raw\":[" + String(rawMoistureValues[0]) + "," + String(rawMoistureValues[1]) + "," 
                      + String(rawMoistureValues[2]) + "," + String(rawMoistureValues[3]) + "]}";
  
  server.send(200, "application/json", json);
}


void handlePump() {
  if (server.hasArg("p") && server.hasArg("t")) {
    int pIdx = server.arg("p").toInt() - 1;
    int duration = server.arg("t").toInt();
    
    if (pIdx >= 0 && pIdx < 4) {
      Serial.printf("WEB: Timed Run P%d for %ds\n", pIdx + 1, duration);
      activatePump(pIdx, duration);
    }
  } 
  else if (server.hasArg("on")) {
    int pIdx = server.arg("on").toInt() - 1;
    if (pIdx >= 0 && pIdx < 4) {
      Serial.printf("WEB: Manual ON P%d\n", pIdx + 1);
      activatePump(pIdx, 0);  // Use 0 to trigger default
    }
  } 
  else if (server.hasArg("off")) {
    int pIdx = server.arg("off").toInt() - 1;
    if (pIdx >= 0 && pIdx < 4) {
      Serial.printf("WEB: Manual OFF P%d\n", pIdx + 1);
      stopPump(pIdx);
    }
  }

  server.sendHeader("Location", "/");
  server.send(303);
}

void handleCalibrate() {
  if (server.hasArg("type")) {
    String type = server.arg("type");
    int sensorIdx = server.hasArg("sensor") ? server.arg("sensor").toInt() : 0;
    int val = server.arg("val").toInt();

    preferences.begin("calib", false);

    if (type == "override") {
      manualOverride = (val == 1);
    } 
    else if (type == "waterFull") {
      waterMaxCM = val;
      preferences.putInt("wMax", waterMaxCM);
    } 
    else if (type == "waterEmpty") {
      waterMinCM = val;
      preferences.putInt("wMin", waterMinCM);
    } 
    else if (type == "wet") {
      if(sensorIdx > 0) {
        wetMin[sensorIdx-1] = val;
        String key = "wet" + String(sensorIdx);
        preferences.putInt(key.c_str(), val);
      }
    } 
    else if (type == "dry") {
      if(sensorIdx > 0) {
        dryMax[sensorIdx-1] = val;
        String key = "dry" + String(sensorIdx);
        preferences.putInt(key.c_str(), val);
      }
    }

    preferences.end();
    server.send(200, "text/plain", "OK");
    Serial.printf("CALIB: %s set to %d\n", type.c_str(), val);
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
  String msg = ""; 
  for (int i=0; i<length; i++) msg += (char)payload[i];
  for (int i=0; i<4; i++) {
    if (String(topic) == (baseTopic + String(i+1) + "/set")) {
      if (msg.equalsIgnoreCase("ON")) activatePump(i, 0);
      else if (msg.equalsIgnoreCase("OFF")) stopPump(i);
    }
  }
}

void drawOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  int16_t x1, y1;
  uint16_t w, h;

  // ---------- TOP ROW (Water | Time | Temp) ----------
  // Water (Left)
  display.setCursor(0, 0);
  display.print("W:");
  display.print(waterLevelPercent);
  display.print("%");

  // Time (Centered)
  DateTime rtcNow = rtc.now();
  char timeBuffer[6];
  sprintf(timeBuffer, "%02d:%02d", rtcNow.hour(), rtcNow.minute());
  display.getTextBounds(timeBuffer, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH / 2) - (w / 2), 0);
  display.print(timeBuffer);

  // Temp (Right)
  float tempC = readTemperatureC();
  char tempBuffer[8];
  sprintf(tempBuffer, "%.1f%c", tempC, 247);
  display.getTextBounds(tempBuffer, 0, 0, &x1, &y1, &w, &h);
  display.setCursor(SCREEN_WIDTH - w, 0);
  display.print(tempBuffer);

  // ---------- PUMPS & MOISTURE (Body) ----------
  for (int i = 0; i < 4; i++) {
    int y = 16 + i * 12;
    display.setCursor(0, y);
    if (rawMoistureValues[i] < 500) {
      display.printf("P%d: %-3s M%d:  X",
                     i + 1,
                     pumpActive[i] ? "ON" : "OFF",
                     i + 1);
    } else {
      display.printf("P%d: %-3s M%d: %d%%",
                     i + 1,
                     pumpActive[i] ? "ON" : "OFF",
                     i + 1,
                     moisturePercent[i]);
    }
  }

  // ---------- NETWORK STATUS ICON (Bottom Right) ----------
  // Logic prioritized by Boot Mode and Connection Status
  if (bootMode == MODE_MQTT && mqtt.connected()) {
    // Show MQTT icon if connected to broker
    drawMQTTIcon(SCREEN_WIDTH - 15, SCREEN_HEIGHT - 12);
  }
  else if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
    // Show Mast icon if Access Point is active
    drawAPIcon(SCREEN_WIDTH - 15, SCREEN_HEIGHT - 10);
  }
  else if (WiFi.status() == WL_CONNECTED) {
    // Show WiFi fan if connected as a station
    drawWiFiIcon(SCREEN_WIDTH - 15, SCREEN_HEIGHT - 14);
  }

  if (digitalRead(LID_PIN) == HIGH) {
    display.setCursor(SCREEN_WIDTH - 23, 17); // Positioned below Temp
    display.print("LID");
    display.setCursor(SCREEN_WIDTH - 23, 27); // Positioned below Temp
    display.print("OFF");
  }

  DateTime now = rtc.now();
  Serial.printf("RTC Time: %02d:%02d:%02d\n", now.hour(), now.minute(), now.second());
  
  display.display();
}

void setup() {
  Serial.begin(115200);
  uint8_t sf = 0;

  // --- 1. IMMEDIATE HARDWARE START ---
  ring.begin();
  ring.setBrightness(150);
  
  Wire.begin(21, 22);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(5, 24); // Adjusted cursor for "Booting..."
  display.print("Booting...");
  display.display();

// --- 2. HIGH-SPEED 2-SECOND ANIMATION ---
  unsigned long bootStart = millis();
  while (millis() - bootStart < 2000) {
    unsigned long elapsed = millis() - bootStart;
    int pct = (elapsed * 100) / 2000;
    
    // INCREASE THIS: sf += 2 makes it twice as fast
    bootSwirl(sf += 2);   
    
    drawProgressBar(pct);
    
    // DECREASE THIS: 10ms is about the limit for stable OLED updates
    delay(10); 
  }

  // 2. PIN MODES & SWITCH DETECTION
  pinMode(LID_PIN, INPUT_PULLDOWN);
  pinMode(SWITCH_PIN_I, INPUT_PULLUP);
  pinMode(SWITCH_PIN_II, INPUT_PULLUP);
  
  // Swirl while waiting for pins to settle (replaces hard delay)
  for(int i=0; i<10; i++) { delay(10); bootSwirl(sf++); }

  int p1 = digitalRead(SWITCH_PIN_I);
  int p3 = digitalRead(SWITCH_PIN_II);

  // Set BootMode based on your truth table
  if (p1 == HIGH && p3 == LOW) {
    bootMode = MODE_AP;
    Serial.println("Mode: AP");
  } else if (p1 == LOW && p3 == HIGH) {
    bootMode = MODE_MQTT;
    Serial.println("Mode: MQTT");
  } else {
    bootMode = MODE_WIFI;
    Serial.println("Mode: WiFi");
  }
  bootSwirl(sf++);
  

  // 3. INITIAL WIFI CONNECT (Animated)
  if (bootMode != MODE_AP) {
    WiFi.begin(ssid, password);
    unsigned long startAttempt = millis();
    // Swirl for up to 10 seconds or until connected
    while (WiFi.status() != WL_CONNECTED && (millis() - startAttempt < 10000)) {
      bootSwirl(sf++);
      delay(50);
    }
  }
  bootSwirl(sf++);

  // 4. STORAGE & SENSORS (Swirl after each block to prevent freezing)
  if (!storage.begin()) Serial.println("Storage Fail");
  bootSwirl(sf++);

  // Inside setup()
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
  } else {
    // This is the "Magic" check:
    if (rtc.lostPower()) {
      Serial.println("RTC power failure! Setting to compile time...");
      // This only runs if the battery was removed or died
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    
    // If we are in a mode with WiFi, update the chip
    if (WiFi.status() == WL_CONNECTED) {
      syncRTCFromWiFi();
    }
  }
  bootSwirl(sf++);

  // Sensors (BMP and Lox)
  bmp.begin(0x76);
  bootSwirl(sf++);
  lox.begin();
  bootSwirl(sf++);

  // 5. CALIBRATION & PUMPS
  calibration.loadAll(wetMin, dryMax, waterMinCM, waterMaxCM, pumpTimeoutS);
  bootSwirl(sf++);

  for (int i=0; i<4; i++) { 
    pinMode(pumpPins[i], OUTPUT); 
    digitalWrite(pumpPins[i], HIGH);
    bootSwirl(sf++);
  }

  // 6. SECONDARY NETWORK CONFIG
  if (bootMode == MODE_AP) {
    WiFi.softAP("ESP32-Watering", "watering123");
  } else if (bootMode == MODE_MQTT) {
    mqtt.setServer(mqtt_server, 1883);
    mqtt.setCallback(mqttCallback);
    timeClient.begin();
  }
  bootSwirl(sf++);

  // 7. WEB SERVER ENDPOINTS
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/pump", handlePump);
  server.on("/calibrate", handleCalibrate);
  server.on("/routines", handleGetRoutines);
  server.on("/automations", handleGetAutomations);
  server.on("/routine/run", handleRunRoutine);
  server.on("/automation/run", handleRunAutomation);
  server.on("/storage/info", handleStorageInfo);
  bootSwirl(sf++);

  // 8. SYSTEM READY
  server.begin(); 
  Serial.println("System Ready");

  // Final visual clear
  ring.clear();
  ring.show();
  
  // Set LED brightness for main loop
  ring.setBrightness(255);
}

void loop()
{
  unsigned long now = millis();
  server.handleClient();

  // Handle MQTT connectivity if in MQTT mode
  if (bootMode == MODE_MQTT)
  {
    if (!mqtt.connected())
      reconnectMQTT();
    mqtt.loop();
  }

  // --- LID STATUS ---
  bool lidOff = (digitalRead(LID_PIN) == HIGH);

  // --- Water level (PAUSED if lid is off) ---
  if (!lidOff && (now - lastWaterRead > 3000))
  {
    long d = updateWaterLevel(); 
    // updateWaterLevel() already updates waterLevelPercent internally
    lastWaterRead = now;
  }

  // --- LED SYSTEM (Modified for Lid/Animation) ---
  static unsigned long lastLedUpdate = 0;
  if (now - lastLedUpdate > 20)
  {
    lastLedUpdate = now;
    if (lidOff) {
      drawBreathingRed(); // Override with Red Breath
    } else {
      switch (currentLedMode) {
        case LED_WAVE:  drawWave();  break;
        case LED_SOLID: /* placeholder */ break;
        case LED_SMART: /* placeholder */ break;
      }
    }
  }

  // --- Pump safety & timers ---
  for (int i = 0; i < 4; i++)
  {
    if (pumpActive[i] && pumpOffTime[i] > 0 && millis() >= pumpOffTime[i])
    {
      stopPump(i);
      Serial.printf("PUMP %d STOPPED\n", i + 1);
    }
  }

  // --- Moisture sensors ---
  if (now - lastMoistureRead > 2000)
  {
    for (int i = 0; i < 4; i++)
    {
      rawMoistureValues[i] = readFastADC(moisturePins[i]);
      moisturePercent[i] = mapMoistToPercent(rawMoistureValues[i], i);
    }
    lastMoistureRead = now;
  }

  // --- Routine checking ---
  if (bootMode == MODE_MQTT)
  {
    // REPLACE timeClient calls with rtc.now()
    DateTime nowRTC = rtc.now();
    int currentHour = nowRTC.hour();
    int currentMinute = nowRTC.minute();
    int currentDay = nowRTC.dayOfTheWeek(); // NTPClient uses 0-6, RTClib also uses 0-6 (Sun-Sat)

    if (currentMinute != lastMinute)
    {
      lastMinute = currentMinute;
      String routines = storage.loadRoutines();
      int duration = 0;
      int pumpToRun = routineExec.checkRoutines(
          routines, currentDay, currentHour, currentMinute,
          moisturePercent, duration);

      if (pumpToRun >= 0 && pumpToRun < 4)
      {
        Serial.printf("Executing routine: Pump %d for %ds\n", pumpToRun + 1, duration);
        activatePump(pumpToRun, duration);
      }
    }
  }

  // --- Automation checking ---
  if (now - lastAutomationCheck > 5000)
  {
    lastAutomationCheck = now;
    String automations = storage.loadAutomations();
    int hour = bootMode == MODE_MQTT ? timeClient.getHours() : 12;
    int minute = bootMode == MODE_MQTT ? timeClient.getMinutes() : 0;

    automationExec.checkAutomations(
        automations, moisturePercent, waterLevelPercent,
        pumpActive, hour, minute,
        pumpActionCallback, whatsappActionCallback);
  }

  // --- OLED refresh ---
  static unsigned long lastDraw = 0;
  if (now - lastDraw > 1500)
  {
    drawOLED();
    lastDraw = now;
  }
  static unsigned long lastSync = 0;
  if (WiFi.status() == WL_CONNECTED && (millis() - lastSync > 86400000))
  { // Once every 24h
    syncRTCFromWiFi();
    lastSync = millis();
  }
}
