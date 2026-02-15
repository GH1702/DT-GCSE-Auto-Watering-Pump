// main.cpp - ESP32 Pump Controller (Rewritten Clean Version)

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

// -------------------- Boot Mode --------------------
enum BootMode { MODE_MQTT, MODE_WIFI, MODE_AP };
BootMode bootMode;

#define SWITCH_PIN_I   13
#define SWITCH_PIN_II  12

// -------------------- Network --------------------
const char* ssid       = "BT-ZMAKN3";
const char* password   = "Q4XfNUMLDQpFrN";
const char* mqtt_server = "192.168.1.222";
const char* mqtt_user   = "esp_mqtt";
const char* mqtt_pass   = "Mosquito";
const String baseTopic  = "esp32/pumps/";

// -------------------- Display --------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// -------------------- Pins --------------------
#define TRIG_PIN 16
#define ECHO_PIN 17

int pumpPins[4]     = {26, 25, 33, 32};
int moisturePins[4] = {35, 34, 39, 36};

// -------------------- Objects --------------------
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);
WiFiClient espClient;
PubSubClient mqtt(espClient);
WebServer server(80);
Preferences preferences;

// -------------------- Globals --------------------
bool pumpActive[4] = {false, false, false, false};
unsigned long pumpStartTime[4] = {0,0,0,0};
unsigned long pumpOffTime[4] = {0,0,0,0};

int moisturePercent[4] = {0,0,0,0};
int wetMin[4] = {950,950,950,950};
int dryMax[4] = {3300,3300,3300,3300};

int waterMinCM = 30;
int waterMaxCM = 5;
int waterLevelPercent = 0;

unsigned long lastWaterRead = 0;
unsigned long lastMoistureRead = 0;
unsigned long lastMoistureMQTT = 0;
unsigned long lastClockUpdate = 0;
unsigned long lastWebAccessTime = 0;

int pumpTimeoutS = 10;
bool manualOverride = false;

// ============================================================
// ====================== UTILITIES ===========================
// ============================================================

int readAveragedADC(int pin)
{
  long total = 0;
  for (int i = 0; i < 10; i++)
  {
    total += analogRead(pin);
    delay(2);
  }
  return total / 10;
}

int mapMoistToPercent(int raw, int idx)
{
  if (raw <= wetMin[idx]) return 100;
  if (raw >= dryMax[idx]) return 0;

  float pct = 100.0f * (float)(dryMax[idx] - raw) /
              (float)(dryMax[idx] - wetMin[idx]);

  return constrain((int)pct, 0, 100);
}

long readWaterDistanceCM()
{
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1;

  return duration / 58;
}

// ============================================================
// ====================== PUMPS ===============================
// ============================================================

void publishPumpState(int idx)
{
  if (bootMode != MODE_MQTT) return;

  String topic = baseTopic + String(idx + 1) + "/state";
  mqtt.publish(topic.c_str(), pumpActive[idx] ? "ON" : "OFF", true);
}

void activatePump(int idx, int seconds = 0)
{
  if (idx < 0 || idx >= 4) return;

  digitalWrite(pumpPins[idx], LOW);
  pumpActive[idx] = true;
  pumpStartTime[idx] = millis();

  if (!manualOverride)
  {
    unsigned long maxRun = (unsigned long)pumpTimeoutS * 1000UL;

    if (seconds == 0 || (unsigned long)seconds * 1000UL > maxRun)
      pumpOffTime[idx] = millis() + maxRun;
    else
      pumpOffTime[idx] = millis() + (unsigned long)seconds * 1000UL;
  }
  else
  {
    if (seconds > 0)
      pumpOffTime[idx] = millis() + (unsigned long)seconds * 1000UL;
    else
      pumpOffTime[idx] = 0;
  }

  publishPumpState(idx);
}

void stopPump(int idx)
{
  digitalWrite(pumpPins[idx], HIGH);
  pumpActive[idx] = false;
  pumpOffTime[idx] = 0;
  publishPumpState(idx);
}

// ============================================================
// ====================== MQTT ================================
// ============================================================

void mqttCallback(char* topic, byte* payload, unsigned int length)
{
  String msg;
  for (unsigned int i = 0; i < length; i++)
    msg += (char)payload[i];

  for (int i = 0; i < 4; i++)
  {
    String cmdTopic = baseTopic + String(i + 1) + "/set";

    if (String(topic) == cmdTopic)
    {
      if (msg.equalsIgnoreCase("ON"))
        activatePump(i);

      if (msg.equalsIgnoreCase("OFF"))
        stopPump(i);
    }
  }
}

void reconnectMQTT()
{
  static unsigned long lastAttempt = 0;
  if (millis() - lastAttempt < 5000) return;

  lastAttempt = millis();

  if (mqtt.connect("ESP32_Pump_Controller", mqtt_user, mqtt_pass))
  {
    for (int i = 0; i < 4; i++)
    {
      mqtt.subscribe((baseTopic + String(i + 1) + "/set").c_str());
      publishPumpState(i);
    }
  }
}

// ============================================================
// ====================== WEB ================================
// ============================================================

void handleStatus()
{
  lastWebAccessTime = millis();

  String json = "{";
  json += "\"water\":" + String(waterLevelPercent) + ",";
  json += "\"m\":[" +
          String(moisturePercent[0]) + "," +
          String(moisturePercent[1]) + "," +
          String(moisturePercent[2]) + "," +
          String(moisturePercent[3]) + "]}";

  server.send(200, "application/json", json);
}

void handleRoot()
{
  server.send(200, "text/html", INDEX_HTML);
}

void handleCalibrate()
{
  if (server.hasArg("type") && server.arg("type") == "override")
  {
    manualOverride = (server.arg("val").toInt() == 1);
    Serial.print("Manual Override: ");
    Serial.println(manualOverride ? "ON" : "OFF");
  }

  server.send(200, "text/plain", "OK");
}


void handlePump()
{
  if (server.hasArg("on"))
  {
    activatePump(server.arg("on").toInt() - 1);
  }
  else if (server.hasArg("off"))
  {
    stopPump(server.arg("off").toInt() - 1);
  }
  else if (server.hasArg("p") && server.hasArg("t"))
  {
    int p = server.arg("p").toInt() - 1;
    int t = server.arg("t").toInt();
    activatePump(p, t);
  }

  server.sendHeader("Location", "/");
  server.send(303);
}


// ============================================================
// ====================== OLED ===============================
// ============================================================

void drawOLED()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print("Water: ");
  display.print(waterLevelPercent);
  display.print("%");

  for (int i = 0; i < 4; i++)
  {
    int y = 16 + i * 12;
    display.setCursor(0, y);
    display.print("P");
    display.print(i + 1);
    display.print(": ");
    display.print(pumpActive[i] ? "ON " : "OFF");

    display.setCursor(64, y);
    display.print("M");
    display.print(i + 1);
    display.print(": ");
    display.print(moisturePercent[i]);
    display.print("%");
  }

  display.display();
}

// ============================================================
// ====================== SETUP ===============================
// ============================================================

void setup()
{
  Serial.begin(115200);

  pinMode(SWITCH_PIN_I, INPUT_PULLUP);
  pinMode(SWITCH_PIN_II, INPUT_PULLUP);

  delay(50);

  if (digitalRead(SWITCH_PIN_I) == LOW)
    bootMode = MODE_MQTT;
  else if (digitalRead(SWITCH_PIN_II) == LOW)
    bootMode = MODE_AP;
  else
    bootMode = MODE_WIFI;

  // Pump pins
  for (int i = 0; i < 4; i++)
  {
    pinMode(pumpPins[i], OUTPUT);
    digitalWrite(pumpPins[i], HIGH);  // OFF (active LOW relays)
  }

  // Ultrasonic
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // OLED
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();

  // Load preferences
  preferences.begin("calib", true);
  pumpTimeoutS = preferences.getInt("pTimeout", 10);
  preferences.end();

  // WiFi
  if (bootMode == MODE_AP)
  {
    WiFi.softAP("ESP32-Watering", "watering123");
    Serial.println("AP Mode Started");
  }
  else
  {
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");

    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - startAttempt < 15000)
    {
      delay(500);
      Serial.print(".");
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("Connected!");
      Serial.println(WiFi.localIP());
    }
    else
    {
      Serial.println("WiFi Failed");
    }

    if (bootMode == MODE_MQTT)
    {
      mqtt.setServer(mqtt_server, 1883);
      mqtt.setCallback(mqttCallback);
      timeClient.begin();
    }
  }

  // Web routes
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/pump", handlePump);
  server.on("/calibrate", handleCalibrate);

  server.onNotFound([](){
    server.send(404, "text/plain", "Not found");
  });

  // ⭐ THIS WAS MISSING
  server.begin();
  Serial.println("Web server started");
}



// ============================================================
// ====================== LOOP ================================
// ============================================================

void loop()
{
  unsigned long now = millis();

  server.handleClient();

  if (bootMode == MODE_MQTT)
  {
    if (!mqtt.connected())
      reconnectMQTT();

    mqtt.loop();

    if (now - lastMoistureMQTT > 5000)
    {
      for (int i = 0; i < 4; i++)
      {
        String topic = baseTopic + "moisture" + String(i + 1);
        mqtt.publish(topic.c_str(),
                     String(moisturePercent[i]).c_str(),
                     true);
      }
      lastMoistureMQTT = now;
    }
  }

  for (int i = 0; i < 4; i++)
  {
    if (pumpActive[i])
    {
      bool timedOut =
        (pumpOffTime[i] > 0 && now >= pumpOffTime[i]);

      bool safetyHit =
        (!manualOverride &&
         (now - pumpStartTime[i]) >
         (unsigned long)pumpTimeoutS * 1000UL);

      if (timedOut || safetyHit)
        stopPump(i);
    }
  }

  if (now - lastWaterRead > 2000)
  {
    long d = readWaterDistanceCM();
    if (d > 0)
    {
      waterLevelPercent = map(
        constrain(d, waterMaxCM, waterMinCM),
        waterMaxCM, waterMinCM,
        100, 0
      );
    }
    lastWaterRead = now;
  }

  if (now - lastMoistureRead > 1000)
  {
    for (int i = 0; i < 4; i++)
      moisturePercent[i] =
        mapMoistToPercent(
          readAveragedADC(moisturePins[i]), i);

    lastMoistureRead = now;
  }

  static unsigned long lastDraw = 0;
  if (now - lastDraw > 1000)
  {
    drawOLED();
    lastDraw = now;
  }
}
