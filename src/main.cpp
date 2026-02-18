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
  configTime(0, 0, "pool.ntp.org");

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    rtc.adjust(DateTime(
      timeinfo.tm_year + 1900,
      timeinfo.tm_mon + 1,
      timeinfo.tm_mday,
      timeinfo.tm_hour,
      timeinfo.tm_min,
      timeinfo.tm_sec
    ));
  }
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
void activatePump(int idx, int seconds = 0) {
  if (idx < 0 || idx >= 4) return;
  
  unsigned long startTime = millis();
  pumpStartTime[idx] = startTime;
  pumpActive[idx] = true;

  unsigned long duration = (unsigned long)seconds;

  if (!manualOverride) {
    if (duration == 0 || duration > (unsigned long)pumpTimeoutS) {
      duration = (unsigned long)pumpTimeoutS;
    }
    pumpOffTime[idx] = startTime + (duration * 1000UL);
  } else {
    if (duration > 0) {
      pumpOffTime[idx] = startTime + (duration * 1000UL);
    } else {
      pumpOffTime[idx] = 0;
    }
  }

  digitalWrite(pumpPins[idx], LOW); 
  publishPumpState(idx);
  Serial.printf("PUMP %d: Duration set to %lu seconds\n", idx + 1, duration);
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
  
  String json = "{\"water\":" + String(waterLevelPercent) + ",";
  json += "\"waterRaw\":" + String(readWaterDistanceCM()) + ",";
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

  // Initialize storage
  if (!storage.begin()) {
    Serial.println("FATAL: Storage initialization failed");
  }
  
  // Initialize VL53L0X
  Wire.begin(21, 22); // SDA, SCL
  Wire.setClock(100000); // 100 kHz
  if (!lox.begin()) {
    Serial.println("VL53L0X not found!");
  }


  // Initialize BMP280
  if (!bmp.begin(0x76)) {
    Serial.println("BMP280 not found!");
  }

  // Initialize RTC
  rtc.begin();

  // Sensor Serial Output
  if (!bmp.begin(0x76)) Serial.println("BMP280 not found!");
  if (!lox.begin()) Serial.println("VL53L0X not found!");
  if (!rtc.begin()) Serial.println("RTC not found!");

  // Load calibration values
  calibration.loadAll(wetMin, dryMax, waterMinCM, waterMaxCM, pumpTimeoutS);
  storage.printInfo();

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


  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();

  if (bootMode == MODE_AP) {
    WiFi.softAP("ESP32-Watering", "watering123");
  } else {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { 
      delay(500); 
      Serial.print("."); 
      syncRTCFromWiFi();
    }
    if (bootMode == MODE_MQTT) {
      mqtt.setServer(mqtt_server, 1883);
      mqtt.setCallback(mqttCallback);
      timeClient.begin();
    }
  }

  // Register web endpoints
  server.on("/", handleRoot);  // Changed from lambda to function
  server.on("/status", handleStatus);
  server.on("/pump", handlePump);
  server.on("/calibrate", handleCalibrate);
  server.on("/routines", HTTP_GET, handleGetRoutines);
  server.on("/routines", HTTP_POST, handleSaveRoutines);
  server.on("/automations", HTTP_GET, handleGetAutomations);
  server.on("/automations", HTTP_POST, handleSaveAutomations);
  server.on("/routine/run", HTTP_POST, handleRunRoutine);
  server.on("/automation/run", HTTP_POST, handleRunAutomation);
  server.on("/storage/info", HTTP_GET, handleStorageInfo);
  
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

  // Pump safety & timers
  for (int i = 0; i < 4; i++) {
    if (pumpActive[i]) {
      bool timedOut = (pumpOffTime[i] > 0 && now >= pumpOffTime[i]);
      bool safetyHit = false;
      
      if (!manualOverride && pumpStartTime[i] > 0) {
        unsigned long maxAllowed = (unsigned long)pumpTimeoutS * 1000UL;
        if (now - pumpStartTime[i] > maxAllowed) safetyHit = true;
      }

      if (timedOut || safetyHit) {
        stopPump(i);
        Serial.printf("PUMP %d STOPPED\n", i + 1);
      }
    }
  }

  // Moisture sensors
  if (now - lastMoistureRead > 2000) {
    for (int i = 0; i < 4; i++) {
      rawMoistureValues[i] = readFastADC(moisturePins[i]);
      moisturePercent[i] = mapMoistToPercent(rawMoistureValues[i], i);
    }
    lastMoistureRead = now;
  }

  // Water level
  if (now - lastWaterRead > 3000) {
    long d = getSmoothedWaterDistance();
    if (d > 0) {
      waterLevelPercent = map(constrain(d, waterMaxCM, waterMinCM), waterMinCM, waterMaxCM, 0, 100);
    }
    lastWaterRead = now;
  }

  // Routine checking
  if (bootMode == MODE_MQTT) {
    timeClient.update();
    int currentHour = timeClient.getHours();
    int currentMinute = timeClient.getMinutes();
    int currentDay = timeClient.getDay();
    
    if (currentMinute != lastMinute) {
      lastMinute = currentMinute;
      
      String routines = storage.loadRoutines();
      int duration = 0;
      int pumpToRun = routineExec.checkRoutines(
        routines, currentDay, currentHour, currentMinute,
        moisturePercent, duration
      );
      
      if (pumpToRun >= 0 && pumpToRun < 4) {
        Serial.printf("Executing routine: Pump %d for %ds\n", pumpToRun + 1, duration);
        activatePump(pumpToRun, duration);
      }
    }
  }
  
  // Automation checking
  if (now - lastAutomationCheck > 5000) {
    lastAutomationCheck = now;
    
    String automations = storage.loadAutomations();
    int hour = bootMode == MODE_MQTT ? timeClient.getHours() : 12;
    int minute = bootMode == MODE_MQTT ? timeClient.getMinutes() : 0;
    
    automationExec.checkAutomations(
      automations, moisturePercent, waterLevelPercent,
      pumpActive, hour, minute,
      pumpActionCallback, whatsappActionCallback
    );
  }



  // OLED refresh
  static unsigned long lastDraw = 0;
  if (now - lastDraw > 1500) {
    drawOLED();
    lastDraw = now;
  }
  
  //I2C Delay
  if (millis() - lastWaterRead > 3000) { getSmoothedWaterDistance(); }
  if (millis() - lastDraw > 1500) { drawOLED(); }
}