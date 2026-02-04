// main.cpp - ESP32 Pump Controller with OLED, MQTT, NTP and 4 moisture sensors
// Active-low relays - LOW = ON, HIGH = OFF

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
enum BootMode {
  MODE_AP,
  MODE_MQTT
};

BootMode bootMode;
#define MODE_PIN 13 // GPIO13 to select mode at boot (HIGH = MQTT, LOW = AP)

// --------- User config ----------
const char* ssid       = "BT-ZMAKN3";
const char* password   = "Q4XfNUMLDQpFrN";

const char* mqtt_server = "192.168.1.222";
const char* mqtt_user   = "esp_mqtt";
const char* mqtt_pass   = "Mosquito";

// MQTT base topic (change if you want)
const String baseTopic = "esp32/pumps/";

// ---------- Display ----------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ---------- Ultrasonic (SR04M) ----------
#define TRIG_PIN 16
#define ECHO_PIN 17


// ---------- Time (NTP) ----------
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // UTC, update every 60s

// ---------- Networking / MQTT ----------
WiFiClient espClient;
PubSubClient mqtt(espClient);

// ---------- WebServer Constants ----------
WebServer server(80);

unsigned long pumpOffTime[4] = {0, 0, 0, 0};


// ---------- Pins ----------
int pumpPins[4]      = {26, 25, 33, 32};   // relay outputs (active-low)
int moisturePins[4] = {35, 34, 39, 36};    // analog inputs (ESP32 ADC pins)
int readAveragedADC(int pin) {
  long total = 0;
  for (int i = 0; i < 10; i++) {
    total += analogRead(pin);
    delay(2);
  }
  return total / 10;
}



bool pumpState[4] = {false, false, false, false};   // false = OFF, true = ON
int moisturePercent[4] = {0,0,0,0};

// ---------- Calibration ----------
int wetMin[4] = {950, 950, 950, 950};   // reading in water -> treated as 100%
const int wetMax  = 1100;  // just a safety high bound near wet
const int dryMin  = 3200;  // dry lower bound
int dryMax[4] = {3300, 3300, 3300, 3300};  // dry upper bound -> treated as 0%

// ---------- Timers ----------
unsigned long lastOledUpdate    = 0;
unsigned long lastMoistureRead  = 0;
unsigned long lastMoistureMQTT  = 0;
unsigned long lastClockUpdate   = 0;

// ---------- Helpers ----------
int mapMoistToPercent(int raw, int idx) {
  if (raw <= wetMin[idx]) return 100;
  if (raw >= dryMax[idx]) return 0;

  float pct = 100.0f * (float)(dryMax[idx] - raw) /
              (float)(dryMax[idx] - wetMin[idx]);

  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return (int)round(pct);
}


void publishPumpState(int idx) {
  String topic = baseTopic + String(idx + 1) + "/state";
  mqtt.publish(topic.c_str(), pumpState[idx] ? "ON" : "OFF", true);
}

void publishAllPumpStates() {
  for (int i=0;i<4;i++) publishPumpState(i);
}

void publishMoistureValuesMQTT() {
  for (int i=0;i<4;i++) {
    String topic = baseTopic + "moisture" + String(i + 1);
    String payload = String(moisturePercent[i]);
    mqtt.publish(topic.c_str(), payload.c_str(), true);
  }
}



// ---------- Pump control (active-low) ----------
void setPumpState(int pump, bool state) {
  pumpState[pump] = state;
  // Active-low relay: LOW = ON, HIGH = OFF
  digitalWrite(pumpPins[pump], state ? LOW : HIGH);
  publishPumpState(pump);
}

// ---------- Read moisture sensors ----------
void readMoistureSensors() {
  for (int i = 0; i < 4; i++) {
    int raw = readAveragedADC(moisturePins[i]);
    moisturePercent[i] = mapMoistToPercent(raw, i);
  }
}


// ---------- Read Distance Sensor ----------
long readWaterDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1;

  return duration / 58;
}
int waterPercentFromDistance(long cm) {
  if (cm <= 5) return 100;
  if (cm >= 30) return 0;

  return map(cm, 30, 5, 0, 100);
}
int waterLevelPercent = 0;
unsigned long lastWaterRead = 0;


// ---------- Web server handlers ----------
void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}
  
void handlePump() {

  // timed run
  if (server.hasArg("p") && server.hasArg("t")) {

    int pump = server.arg("p").toInt() - 1;
    int seconds = server.arg("t").toInt();

    if (pump >= 0 && pump < 4 && seconds > 0) {
      setPumpState(pump, true);
      pumpOffTime[pump] = millis() + (unsigned long)seconds * 1000UL;
    }
  }

  // manual ON
  if (server.hasArg("on")) {
    int pump = server.arg("on").toInt() - 1;
    if (pump >= 0 && pump < 4) {
      setPumpState(pump, true);
      pumpOffTime[pump] = 0;
    }
  }

  // manual OFF
  if (server.hasArg("off")) {
    int pump = server.arg("off").toInt() - 1;
    if (pump >= 0 && pump < 4) {
      setPumpState(pump, false);
      pumpOffTime[pump] = 0;
    }
  }

  server.sendHeader("Location", "/");
  server.send(303);
}


// ---------- OLED drawing ----------
void drawOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // centre time at top
  String t = timeClient.getFormattedTime();
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(t, 0, 0, &x1, &y1, &w, &h);
  int cx = (SCREEN_WIDTH - (int)w) / 2;
  if (cx < 0) cx = 0;
  display.setCursor(cx, 0);
  display.print(t);

  // move pump and moisture rows down by 3 pixels
  for (int i = 0; i < 4; i++) {
    int y = 16 + i * 12;  // shifted from 12 to 16
    display.setCursor(0, y);
    display.print("P");
    display.print(i + 1);
    display.print(": ");
    display.print(pumpState[i] ? "ON " : "OFF");

    display.setCursor(64, y);
    display.print("M");
    display.print(i + 1);
    display.print(": ");
    display.print(moisturePercent[i]);
    display.print("%");
    display.setCursor(0, 58);
    display.print("Water: ");
    display.print(waterLevelPercent);
    display.print("%");

  }

  display.display();
}


// ---------- MQTT callbacks ----------
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // build string
  String msg;
  for (unsigned int i=0;i<length;i++) msg += (char)payload[i];

  // check for set topics
  for (int i=0;i<4;i++) {
    String setTopic = baseTopic + String(i+1) + "/set";
    if (String(topic) == setTopic) {
      if (msg.equalsIgnoreCase("ON"))  setPumpState(i, true);
      if (msg.equalsIgnoreCase("OFF")) setPumpState(i, false);
    }
  }
}

// ---------- MQTT reconnect ----------
void reconnectMQTT() {
  // attempt to connect until success - non blocking-ish
  while (!mqtt.connected()) {
    if (mqtt.connect("ESP32_Pump_Controller", mqtt_user, mqtt_pass)) {
      // subscribe to set topics
      for (int i=0;i<4;i++) {
        String sub = baseTopic + String(i+1) + "/set";
        mqtt.subscribe(sub.c_str());
      }
      // ensure pumps OFF at connect and publish states
      for (int i=0;i<4;i++) {
        setPumpState(i, false);
      }
      // publish initial moisture immediately
      readMoistureSensors();
      publishMoistureValuesMQTT();
    } else {
      delay(2000);
    }
  }
}

// ---------- Start Access Point ----------
void startAccessPoint() {
  WiFi.mode(WIFI_AP);

  const char* ap_ssid = "ESP32-Watering";
  const char* ap_pass = "watering123";

  WiFi.softAP(ap_ssid, ap_pass);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("WIFI MODE");
  display.setCursor(0, 16);
  display.print("SSID: "+String(ap_ssid));
  display.display();
  display.setCursor(0, 31);
  display.print("PASS: "+String(ap_pass));
  display.display();

  MDNS.begin("plant");
  server.on("/", handleRoot);
  server.on("/pump", handlePump);
  server.begin();


}


// ---------- Setup ----------
void setup() {
  Serial.begin(115200);

  // determine boot mode
  pinMode(MODE_PIN, INPUT);

  int modeRead = digitalRead(MODE_PIN);

  if (modeRead == HIGH) {
    bootMode = MODE_MQTT;
  } else {
    bootMode = MODE_AP;
  }

  // ADC attenuation: ensure range covers full values if needed
  // Default is fine for many boards but you can set attenuation if required:
  // analogSetPinAttenuation(moisturePins[i], ADC_11db);

  // Pump pins init - set OFF (HIGH)
  for (int i=0;i<4;i++) {
    pinMode(pumpPins[i], OUTPUT);
    digitalWrite(pumpPins[i], HIGH); // OFF
  }

  // Ultrasonic sensor pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);


  // display init
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 init failed");
    while (1) delay(1000);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // show WiFi connecting
  display.setCursor(0,0);
  display.print("WiFi connecting...");
  display.display();

  if (bootMode == MODE_AP) {

    startAccessPoint();

  // do NOT start MQTT or NTP in AP mode
    return;

  } else {

    WiFi.mode(WIFI_STA);

    WiFi.begin(ssid, password);

    unsigned long wifiStart = millis();
    while (WiFi.status() != WL_CONNECTED) {
      delay(200);
      if (millis() - wifiStart > 20000) break;
    }

    mqtt.setServer(mqtt_server, 1883);
    mqtt.setCallback(mqttCallback);
    reconnectMQTT();

    timeClient.begin();
    timeClient.update();
}
  // show WiFi status

  display.clearDisplay();
  if (WiFi.status() == WL_CONNECTED) {
    display.setCursor(0,0);
    display.print("WiFi connected");
  } else {
    display.setCursor(0,0);
    display.print("WiFi failed");
  }
  display.display();
  delay(600);

  // MQTT connecting
  display.clearDisplay();
  display.setCursor(0,0);
  display.print("MQTT connecting...");
  display.display();

  mqtt.setServer(mqtt_server, 1883);
  mqtt.setCallback(mqttCallback);
  reconnectMQTT();

  display.clearDisplay();
  display.setCursor(0,0);
  display.print("MQTT connected");
  display.display();
  delay(600);

  // start NTP client
  timeClient.begin();
  timeClient.update();

  // initial reads and publishes
  readMoistureSensors();
  publishMoistureValuesMQTT();
  publishAllPumpStates();

  // show main screen
  drawOLED();
}

// ---------- Loop ----------
// ---------- Loop ----------
void loop() {
  unsigned long now = millis();

  // --- Common Logic (Runs in both AP and MQTT modes) ---
  
  // Handle timed pump turn-off
  for (int i = 0; i < 4; i++) {
    if (pumpState[i] && pumpOffTime[i] > 0 && now >= pumpOffTime[i]) {
      setPumpState(i, false);
      pumpOffTime[i] = 0;
    }
  }

  // Read water level sensor every 2 seconds
  if (now - lastWaterRead > 2000) {
    lastWaterRead = now;
    long d = readWaterDistanceCM();
    if (d > 0) {
      waterLevelPercent = waterPercentFromDistance(d);
    }
  }

  // --- Mode Specific Logic ---

  if (bootMode == MODE_AP) {
    // AP MODE: Just handle web requests
    server.handleClient();
  } 
  else {
    // MQTT MODE: Handle MQTT connection and periodic updates
    if (!mqtt.connected()) reconnectMQTT();
    mqtt.loop();

    // Read moisture values for OLED every second
    if (now - lastMoistureRead >= 1000) {
      lastMoistureRead = now;
      readMoistureSensors();
    }

    // Publish moisture to MQTT every 5 seconds
    if (now - lastMoistureMQTT >= 5000) {
      lastMoistureMQTT = now;
      publishMoistureValuesMQTT();
    }

    // Update clock every second
    if (now - lastClockUpdate >= 1000) {
      lastClockUpdate = now;
      timeClient.update();
    }
  }

  // Redraw OLED every second (Always keep the screen updated)
  static unsigned long lastOledDraw = 0;
  if (now - lastOledDraw >= 1000) {
    lastOledDraw = now;
    drawOLED();
  }
} // <--- This was the missing bracket!