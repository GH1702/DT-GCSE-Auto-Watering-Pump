#ifndef ROUTINES_H
#define ROUTINES_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#include <Preferences.h>

// Maximum items we can store
#define MAX_ROUTINES 20
#define MAX_AUTOMATIONS 20

// ============================================================
// ====================== STORAGE MANAGER =====================
// ============================================================

class StorageManager {
public:
  
  // Initialize SPIFFS
  bool begin() {
    if (!SPIFFS.begin(true)) {
      Serial.println("SPIFFS Mount Failed");
      return false;
    }
    Serial.println("SPIFFS Mounted Successfully");
    
    // Create files if they don't exist
    if (!SPIFFS.exists("/routines.json")) {
      saveJSON("/routines.json", "[]");
    }
    if (!SPIFFS.exists("/automations.json")) {
      saveJSON("/automations.json", "[]");
    }
    
    return true;
  }

  // Save JSON string to file
  bool saveJSON(const char* path, String jsonStr) {
    File file = SPIFFS.open(path, FILE_WRITE);
    if (!file) {
      Serial.printf("Failed to open %s for writing\n", path);
      return false;
    }
    file.print(jsonStr);
    file.close();
    Serial.printf("Saved to %s: %d bytes\n", path, jsonStr.length());
    return true;
  }

  // Load JSON string from file
  String loadJSON(const char* path) {
    if (!SPIFFS.exists(path)) {
      Serial.printf("File %s does not exist\n", path);
      return "[]";
    }
    
    File file = SPIFFS.open(path, FILE_READ);
    if (!file) {
      Serial.printf("Failed to open %s for reading\n", path);
      return "[]";
    }
    
    String content = "";
    while (file.available()) {
      content += (char)file.read();
    }
    file.close();
    
    Serial.printf("Loaded from %s: %d bytes\n", path, content.length());
    return content;
  }

  // Save Routines
  bool saveRoutines(String jsonArray) {
    return saveJSON("/routines.json", jsonArray);
  }

  // Load Routines
  String loadRoutines() {
    return loadJSON("/routines.json");
  }

  // Save Automations
  bool saveAutomations(String jsonArray) {
    return saveJSON("/automations.json", jsonArray);
  }

  // Load Automations
  String loadAutomations() {
    return loadJSON("/automations.json");
  }

  // Get storage info
  void printInfo() {
    Serial.println("=== SPIFFS Info ===");
    Serial.printf("Total: %d bytes\n", SPIFFS.totalBytes());
    Serial.printf("Used: %d bytes\n", SPIFFS.usedBytes());
    Serial.printf("Free: %d bytes\n", SPIFFS.totalBytes() - SPIFFS.usedBytes());
    
    if (SPIFFS.exists("/routines.json")) {
      File f = SPIFFS.open("/routines.json", FILE_READ);
      Serial.printf("Routines file: %d bytes\n", f.size());
      f.close();
    }
    
    if (SPIFFS.exists("/automations.json")) {
      File f = SPIFFS.open("/automations.json", FILE_READ);
      Serial.printf("Automations file: %d bytes\n", f.size());
      f.close();
    }
  }

  // Clear all data (useful for debugging)
  void clearAll() {
    saveRoutines("[]");
    saveAutomations("[]");
    Serial.println("All routines and automations cleared");
  }
};

// ============================================================
// ====================== CALIBRATION MANAGER =================
// ============================================================

class CalibrationManager {
private:
  Preferences prefs;

public:
  
  // Load all calibration values
  void loadAll(int* wetMin, int* dryMax, int& waterMinCM, int& waterMaxCM, int& pumpTimeoutS) {
    prefs.begin("calib", true); // Read-only mode
    
    // Load moisture sensor calibrations
    for (int i = 0; i < 4; i++) {
      String wetKey = "wet" + String(i + 1);
      String dryKey = "dry" + String(i + 1);
      wetMin[i] = prefs.getInt(wetKey.c_str(), 950);
      dryMax[i] = prefs.getInt(dryKey.c_str(), 3300);
    }
    
    // Load water level calibrations
    waterMinCM = prefs.getInt("wMin", 30);
    waterMaxCM = prefs.getInt("wMax", 5);
    
    // Load pump timeout
    pumpTimeoutS = prefs.getInt("pTimeout", 10);
    
    prefs.end();
    
    Serial.println("=== Calibration Loaded ===");
    Serial.printf("Water: Min=%dcm Max=%dcm\n", waterMinCM, waterMaxCM);
    Serial.printf("Pump Timeout: %ds\n", pumpTimeoutS);
    for (int i = 0; i < 4; i++) {
      Serial.printf("M%d: Wet=%d Dry=%d\n", i+1, wetMin[i], dryMax[i]);
    }
  }

  // Save moisture calibration
  bool saveMoisture(int sensor, bool isWet, int value) {
    if (sensor < 1 || sensor > 4) return false;
    
    prefs.begin("calib", false); // Read-write mode
    String key = (isWet ? "wet" : "dry") + String(sensor);
    prefs.putInt(key.c_str(), value);
    prefs.end();
    
    Serial.printf("Saved M%d %s = %d\n", sensor, isWet ? "wet" : "dry", value);
    return true;
  }

  // Save water calibration
  bool saveWater(bool isFull, int value) {
    prefs.begin("calib", false);
    prefs.putInt(isFull ? "wMax" : "wMin", value);
    prefs.end();
    
    Serial.printf("Saved Water %s = %dcm\n", isFull ? "Full" : "Empty", value);
    return true;
  }

  // Save pump timeout
  bool savePumpTimeout(int seconds) {
    prefs.begin("calib", false);
    prefs.putInt("pTimeout", seconds);
    prefs.end();
    
    Serial.printf("Saved Pump Timeout = %ds\n", seconds);
    return true;
  }

  // Reset to defaults
  void resetToDefaults() {
    prefs.begin("calib", false);
    prefs.clear();
    
    // Set defaults
    for (int i = 1; i <= 4; i++) {
      prefs.putInt(("wet" + String(i)).c_str(), 950);
      prefs.putInt(("dry" + String(i)).c_str(), 3300);
    }
    prefs.putInt("wMin", 30);
    prefs.putInt("wMax", 5);
    prefs.putInt("pTimeout", 10);
    
    prefs.end();
    Serial.println("Calibration reset to defaults");
  }
};

// ============================================================
// ====================== ROUTINE EXECUTOR ====================
// ============================================================

class RoutineExecutor {
public:
  RoutineExecutor() {
    for (int i = 0; i < MAX_ROUTINES; i++) {
      smartActive[i] = false;
      smartIds[i] = 0;
    }
  }
  
  // Check if it's time to run any routines
  // Returns: pump index (0-3) to activate, or -1 if none
  // duration: output parameter for how long to run (in seconds)
  int checkRoutines(String routinesJSON, int currentDay, int currentHour, int currentMinute, 
                    int* moistureLevels, int& duration, float* pumpMlPerSec) {
    
    // Parse JSON array
    StaticJsonDocument<4096> doc;
    DeserializationError error = deserializeJson(doc, routinesJSON);
    
    if (error) {
      Serial.println("Failed to parse routines JSON");
      return -1;
    }
    
    JsonArray routines = doc.as<JsonArray>();
    
    for (JsonObject routine : routines) {
      bool enabled = routine["enabled"] | true;
      if (!enabled) continue;
      String mode = routine["mode"] | "static";
      JsonArray days = routine["days"];
      
      // Check if today is active
      if (!days[currentDay]) continue;
      
      if (mode == "static") {
        // Time-based routine
        String timeStr = routine["time"] | "";
        if (timeStr.length() == 5) { // "HH:MM"
          int hour = timeStr.substring(0, 2).toInt();
          int minute = timeStr.substring(3, 5).toInt();
          
          if (hour == currentHour && minute == currentMinute) {
            int pump = routine["p"] | 0;
            int amount = routine["val"] | 100;
            
            // Convert ml to seconds using per-pump calibration.
            float rate = 21.5f;
            if (pumpMlPerSec && pump >= 1 && pump <= 4 && pumpMlPerSec[pump - 1] > 0.01f) {
              rate = pumpMlPerSec[pump - 1];
            }
            duration = (int)ceil((float)amount / rate);
            if (duration < 1) duration = 1;
            if (duration > 10) duration = 10; // Respect watchdog window
            
            Serial.printf("ROUTINE TRIGGERED: %s at %02d:%02d\n", 
                         routine["name"].as<const char*>(), hour, minute);
            return pump - 1; // Return 0-indexed
          }
        }
      }
      else if (mode == "smart") {
        // Moisture-based routine with hysteresis:
        // start when moisture drops below 10% of trigger, then keep running
        // in watchdog-sized pulses until moisture reaches the trigger.
        long routineId = routine["id"] | 0;
        int sensor = routine["s"] | 0;
        int threshold = routine["val"] | 30;
        int pump = routine["p"] | 0;
        
        if (sensor >= 1 && sensor <= 4) {
          int moisture = moistureLevels[sensor - 1];
          bool inWindow = checkTimeWindow(routine, currentHour, currentMinute);
          if (!inWindow) continue;

          int startThreshold = max(1, threshold / 10); // 10% of trigger%
          int slot = getRoutineSlot(routineId);
          bool isActive = (slot >= 0) ? smartActive[slot] : false;

          if (!isActive && moisture <= startThreshold) {
            if (slot >= 0) smartActive[slot] = true;
            isActive = true;
          }

          if (isActive) {
            if (moisture >= threshold) {
              if (slot >= 0) smartActive[slot] = false;
            } else {
              duration = 10; // Keep each run within watchdog window
              Serial.printf("SMART ROUTINE RUN: %s (M%d: %d%%, start<=%d%%, target=%d%%)\n",
                           routine["name"].as<const char*>(), sensor, moisture, startThreshold, threshold);
              return pump - 1;
            }
          }
        }
      }
    }
    
    return -1; // No routine to execute
  }

private:
  bool smartActive[MAX_ROUTINES];
  long smartIds[MAX_ROUTINES];

  int getRoutineSlot(long routineId) {
    for (int i = 0; i < MAX_ROUTINES; i++) {
      if (smartIds[i] == routineId) return i;
    }
    for (int i = 0; i < MAX_ROUTINES; i++) {
      if (smartIds[i] == 0) {
        smartIds[i] = routineId;
        smartActive[i] = false;
        return i;
      }
    }
    return -1;
  }

  bool checkTimeWindow(JsonObject routine, int hour, int minute) {
    bool inverted = routine["inv"] | false;
    int minSlot = routine["min"] | 16;  // 08:00
    int maxSlot = routine["max"] | 36;  // 18:00
    
    // Convert current time to slot (0-47, each = 30 min)
    int currentSlot = (hour * 2) + (minute >= 30 ? 1 : 0);
    
    bool inRange = (currentSlot >= minSlot && currentSlot <= maxSlot);
    
    return inverted ? !inRange : inRange;
  }
};

// ============================================================
// ====================== AUTOMATION EXECUTOR =================
// ============================================================

class AutomationExecutor {
public:
  AutomationExecutor() {
    for (int i = 0; i < MAX_AUTOMATIONS; i++) {
      autoIds[i] = 0;
      lastWhatsAppMs[i] = 0;
    }
  }
  
  // Check all automations and execute actions if triggered
  void checkAutomations(String automationsJSON, int* moistureLevels, int waterLevel,
                       bool* pumpStates, bool lidOff, int lidOffMinutes,
                       int currentHour, int currentMinute,
                       void (*pumpCallback)(int, int), 
                       void (*whatsappCallback)(String),
                       void (*ledModeCallback)(String)) {
    
    StaticJsonDocument<4096> doc;
    DeserializationError error = deserializeJson(doc, automationsJSON);
    
    if (error) {
      Serial.println("Failed to parse automations JSON");
      return;
    }
    
    JsonArray automations = doc.as<JsonArray>();
    
    for (JsonObject auto_obj : automations) {
      bool enabled = auto_obj["enabled"] | true;
      if (!enabled) continue;
      // Check WHEN condition
      bool whenTriggered = checkWhenCondition(auto_obj["when"], 
                                               moistureLevels, waterLevel, pumpStates, lidOff);
      
      if (!whenTriggered) continue;
      
      // Check IF condition (optional)
      if (auto_obj.containsKey("if") && !auto_obj["if"].isNull()) {
        bool ifCondition = checkIfCondition(auto_obj["if"], 
                                            moistureLevels, waterLevel, lidOff, lidOffMinutes,
                                            currentHour, currentMinute);
        if (!ifCondition) continue;
      }
      
      // Execute DO action
      unsigned long long autoId = auto_obj["id"] | 0;
      executeAction(auto_obj["do"], auto_obj["name"], autoId,
                   pumpCallback, whatsappCallback, ledModeCallback);
    }
  }

private:
  unsigned long long autoIds[MAX_AUTOMATIONS];
  unsigned long lastWhatsAppMs[MAX_AUTOMATIONS];

  int getAutoSlot(unsigned long long id) {
    for (int i = 0; i < MAX_AUTOMATIONS; i++) {
      if (autoIds[i] == id) return i;
    }
    for (int i = 0; i < MAX_AUTOMATIONS; i++) {
      if (autoIds[i] == 0) {
        autoIds[i] = id;
        lastWhatsAppMs[i] = 0;
        return i;
      }
    }
    return -1;
  }

  bool checkWhenCondition(JsonObject when, int* moisture, int water, bool* pumps, bool lidOff) {
    String type = when["type"] | "";
    
    if (type == "moisture_below") {
      int sensor = when["sensor"] | 1;
      int threshold = when["threshold"] | 30;
      return moisture[sensor - 1] < threshold;
    }
    else if (type == "moisture_above") {
      int sensor = when["sensor"] | 1;
      int threshold = when["threshold"] | 70;
      return moisture[sensor - 1] > threshold;
    }
    else if (type == "water_below") {
      int threshold = when["threshold"] | 20;
      return water < threshold;
    }
    else if (type == "water_above") {
      int threshold = when["threshold"] | 80;
      return water > threshold;
    }
    else if (type == "pump_starts") {
      int pump = when["pump"] | 1;
      return pumps[pump - 1];
    }
    else if (type == "pump_stops") {
      int pump = when["pump"] | 1;
      return !pumps[pump - 1];
    }
    else if (type == "lid_off") {
      return lidOff;
    }
    
    return false;
  }

  bool checkIfCondition(JsonObject ifCond, int* moisture, int water, bool lidOff, int lidOffMinutes, int hour, int minute) {
    String type = ifCond["type"] | "";
    
    if (type == "moisture_below") {
      int sensor = ifCond["sensor"] | 1;
      int threshold = ifCond["threshold"] | 50;
      return moisture[sensor - 1] < threshold;
    }
    else if (type == "moisture_above") {
      int sensor = ifCond["sensor"] | 1;
      int threshold = ifCond["threshold"] | 50;
      return moisture[sensor - 1] > threshold;
    }
    else if (type == "water_below") {
      int threshold = ifCond["threshold"] | 20;
      return water < threshold;
    }
    else if (type == "water_above") {
      int threshold = ifCond["threshold"] | 80;
      return water > threshold;
    }
    else if (type == "time_between") {
      String from = ifCond["timeFrom"] | "08:00";
      String to = ifCond["timeTo"] | "20:00";
      
      int fromH = from.substring(0, 2).toInt();
      int fromM = from.substring(3, 5).toInt();
      int toH = to.substring(0, 2).toInt();
      int toM = to.substring(3, 5).toInt();
      
      int currentMinutes = hour * 60 + minute;
      int fromMinutes = fromH * 60 + fromM;
      int toMinutes = toH * 60 + toM;
      
      return (currentMinutes >= fromMinutes && currentMinutes <= toMinutes);
    }
    else if (type == "lid_off_for") {
      int mins = ifCond["minutes"] | 10;
      return lidOff && lidOffMinutes >= mins;
    }
    
    return true; // No condition = always pass
  }

  void executeAction(JsonObject doAction, String autoName, unsigned long long autoId,
                    void (*pumpCallback)(int, int),
                    void (*whatsappCallback)(String),
                    void (*ledModeCallback)(String)) {
    String type = doAction["type"] | "";
    
    if (type == "whatsapp") {
      String message = doAction["message"] | "";
      int repeatHours = doAction["repeatHours"] | 0;
      int slot = getAutoSlot(autoId);
      bool sendNow = true;
      if (repeatHours > 0 && slot >= 0) {
        unsigned long cooldownMs = (unsigned long)repeatHours * 3600000UL;
        if (millis() - lastWhatsAppMs[slot] < cooldownMs) sendNow = false;
      }
      if (sendNow) {
        Serial.printf("AUTOMATION: %s -> WhatsApp: %s\n", autoName.c_str(), message.c_str());
        if (whatsappCallback) whatsappCallback(message);
        if (slot >= 0) lastWhatsAppMs[slot] = millis();
      }
    }
    else if (type == "pump_on") {
      int pump = doAction["pump"] | 1;
      int duration = doAction["duration"] | 10;
      Serial.printf("AUTOMATION: %s -> Pump %d ON for %ds\n", autoName.c_str(), pump, duration);
      if (pumpCallback) pumpCallback(pump - 1, duration);
    }
    else if (type == "pump_off") {
      int pump = doAction["pump"] | 1;
      Serial.printf("AUTOMATION: %s -> Pump %d OFF\n", autoName.c_str(), pump);
      if (pumpCallback) pumpCallback(pump - 1, 0); // 0 = stop
    }
    else if (type == "led_mode") {
      String ledMode = doAction["ledMode"] | "normal";
      Serial.printf("AUTOMATION: %s -> LED mode %s\n", autoName.c_str(), ledMode.c_str());
      if (ledModeCallback) ledModeCallback(ledMode);
    }
  }
};

// Global instances (declare these in your main .cpp)
extern StorageManager storage;
extern CalibrationManager calibration;
extern RoutineExecutor routineExec;
extern AutomationExecutor automationExec;

#endif // ROUTINES_H
