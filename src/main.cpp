// main.cpp - ESP32 Pump Controller with OLED, MQTT, NTP and 4 moisture sensors
// Active-low relays - LOW = ON, HIGH = OFF

#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

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

// ---------- Time (NTP) ----------
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // UTC, update every 60s

// ---------- Networking / MQTT ----------
WiFiClient espClient;
PubSubClient mqtt(espClient);

// ---------- Pins ----------
int pumpPins[4]      = {25, 26, 27, 14};   // relay outputs (active-low)
int moisturePins[4]  = {34, 35, 32, 33};   // analog inputs (ESP32 ADC pins)

bool pumpState[4] = {false, false, false, false};   // false = OFF, true = ON
int moisturePercent[4] = {0,0,0,0};

// ---------- Calibration ----------
const int wetMin  = 950;   // reading in water -> treated as 100%
const int wetMax  = 1100;  // just a safety high bound near wet
const int dryMin  = 3200;  // dry lower bound
const int dryMax  = 3300;  // dry upper bound -> treated as 0%

// ---------- Timers ----------
unsigned long lastOledUpdate    = 0;
unsigned long lastMoistureRead  = 0;
unsigned long lastMoistureMQTT  = 0;
unsigned long lastClockUpdate   = 0;

// ---------- Helpers ----------
int mapMoistToPercent(int raw) {
  if (raw <= wetMin) return 100;
  if (raw >= dryMax) return 0;
  // linear map between wetMin (100%) and dryMax (0%)
  float pct = 100.0f * (float)(dryMax - raw) / (float)(dryMax - wetMin);
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
  for (int i=0;i<4;i++) {
    int raw = analogRead(moisturePins[i]);
    moisturePercent[i] = mapMoistToPercent(raw);
  }
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

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);

  // ADC attenuation: ensure range covers full values if needed
  // Default is fine for many boards but you can set attenuation if required:
  // analogSetPinAttenuation(moisturePins[i], ADC_11db);

  // pump pins init - set OFF (HIGH)
  for (int i=0;i<4;i++) {
    pinMode(pumpPins[i], OUTPUT);
    digitalWrite(pumpPins[i], HIGH); // OFF
  }

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

  WiFi.begin(ssid, password);
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    // keep the device responsive - break after long timeout if you want
    if (millis() - wifiStart > 20000) { // 20s timeout - optional
      break;
    }
  }

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
void loop() {
  if (!mqtt.connected()) reconnectMQTT();
  mqtt.loop();

  unsigned long now = millis();

  // read moisture values for OLED every second
  if (now - lastMoistureRead >= 1000) {
    lastMoistureRead = now;
    readMoistureSensors();
  }

  // publish moisture to MQTT every 5 seconds
  if (now - lastMoistureMQTT >= 5000) {
    lastMoistureMQTT = now;
    publishMoistureValuesMQTT();
  }

  // update clock every second and redraw display
  if (now - lastClockUpdate >= 1000) {
    lastClockUpdate = now;
    timeClient.update();   // update NTP display time - not heavy
    drawOLED();
  }
}
