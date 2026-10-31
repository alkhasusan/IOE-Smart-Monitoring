
/*
 * IOE Smart Security System
 * ESP32 + DHT22 + HC-SR04 + PIR + LEDs + Buzzer
 * Sends telemetry to the React dashboard through HTTPS.
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <DHTesp.h>
#include <math.h>

// Wi-Fi
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

// Dashboard HTTP bridge
const char* BRIDGE_URL =
  "https://pins-erik-palmer-valium.trycloudflare.com/api/wokwi/telemetry";

// ESP32 pins
const int PIN_PIR = 13;
const int PIN_TRIG = 5;
const int PIN_ECHO = 18;
const int PIN_BUZZER = 19;
const int PIN_LED_RED = 21;
const int PIN_LED_GREEN = 22;
const int PIN_DHT = 15;

// DHT22 sensor
DHTesp dht;

// Security state
bool armed = true;
bool alarmActive = false;

// Timing
unsigned long lastPost = 0;
unsigned long lastDHTRead = 0;
unsigned long lastWiFiAttempt = 0;

const unsigned long POST_INTERVAL = 1000;
const unsigned long DHT_INTERVAL = 2200;
const unsigned long WIFI_RETRY_INTERVAL = 10000;

// Cached DHT22 readings
float temperature = 25.0f;
float humidity = 55.0f;
bool validDHT = false;

// Read ultrasonic distance
float readDistanceCm() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);

  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  unsigned long duration = pulseIn(PIN_ECHO, HIGH, 30000);

  if (duration == 0) {
    return 150.0f;
  }

  return duration * 0.0343f / 2.0f;
}

// Connect to Wi-Fi
bool connectWiFi() {
  Serial.println("[WiFi] Connecting to Wokwi-GUEST...");

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long started = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - started < 15000) {
    delay(250);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[WiFi] Connected. IP: ");
    Serial.println(WiFi.localIP());
    return true;
  }

  Serial.println("[WiFi] Connection failed.");
  return false;
}

// Update alarm outputs
void updateAlarm(bool motion, float distance) {
  alarmActive = armed && (motion || distance < 30.0f);

  digitalWrite(PIN_LED_RED, alarmActive ? HIGH : LOW);
  digitalWrite(PIN_LED_GREEN, alarmActive ? LOW : HIGH);
  digitalWrite(PIN_BUZZER, alarmActive ? HIGH : LOW);
}

// Send telemetry to the dashboard
void sendTelemetry(
  float temp,
  float hum,
  bool motion,
  float distance
) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[HTTP] Wi-Fi disconnected.");
    return;
  }

  String payload = "{";
  payload += "\"temperature\":" + String(temp, 1) + ",";
  payload += "\"humidity\":" + String(hum, 1) + ",";
  payload += "\"motion\":";
  payload += motion ? "true" : "false";
  payload += ",\"distance\":" + String(distance, 1) + ",";
  payload += "\"alarm\":";
  payload += alarmActive ? "true" : "false";
  payload += ",\"armed\":";
  payload += armed ? "true" : "false";
  payload += "}";

  Serial.print("[Telemetry] ");
  Serial.println(payload);

  WiFiClientSecure client;

  // DEMO ONLY: disables TLS certificate verification.
  // Use proper certificate verification in production.
  client.setInsecure();

  HTTPClient http;

  // Keep failed requests from blocking the loop too long.
  http.setConnectTimeout(3000);
  http.setTimeout(3000);

  if (!http.begin(client, BRIDGE_URL)) {
    Serial.println("[HTTP] Request initialization failed.");
    return;
  }

  http.addHeader("Content-Type", "application/json");
  http.addHeader("Cache-Control", "no-cache");

  int status = http.POST(payload);

  Serial.printf("[HTTP] POST status: %d\n", status);

  if (status > 0) {
    Serial.print("[HTTP] Response: ");
    Serial.println(http.getString());
  } else {
    Serial.print("[HTTP] Error: ");
    Serial.println(HTTPClient::errorToString(status));
  }

  http.end();
}

// Initialize hardware
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("==================================");
  Serial.println(" IOE Smart Security System");
  Serial.println(" ESP32 + DHT22 + HC-SR04 + PIR");
  Serial.println("==================================");

  pinMode(PIN_PIR, INPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);

  digitalWrite(PIN_TRIG, LOW);
  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED_RED, LOW);
  digitalWrite(PIN_LED_GREEN, HIGH);

  dht.setup(PIN_DHT, DHTesp::DHT22);

  connectWiFi();
  lastWiFiAttempt = millis();

  Serial.println("[Setup] Sensor initialization complete.");
}

// Main loop
void loop() {
  unsigned long now = millis();

  // Reconnect Wi-Fi when needed
  if (WiFi.status() != WL_CONNECTED &&
      now - lastWiFiAttempt >= WIFI_RETRY_INTERVAL) {
    lastWiFiAttempt = now;
    connectWiFi();
  }

  // Read DHT22 at its scheduled interval
  if (lastDHTRead == 0 ||
      now - lastDHTRead >= DHT_INTERVAL) {
    lastDHTRead = now;

    TempAndHumidity readings = dht.getTempAndHumidity();

    if (!isnan(readings.temperature) &&
        !isnan(readings.humidity) &&
        readings.temperature >= -40.0f &&
        readings.temperature <= 80.0f &&
        readings.humidity >= 0.0f &&
        readings.humidity <= 100.0f) {

      temperature = readings.temperature;
      humidity = readings.humidity;
      validDHT = true;

      Serial.printf(
        "[DHT22] Temperature: %.1f C, Humidity: %.1f %%\n",
        temperature,
        humidity
      );
    } else {
      validDHT = false;
      Serial.println("[DHT22] Invalid reading.");
    }
  }

  // Read PIR and ultrasonic sensors
  bool motion = digitalRead(PIN_PIR) == HIGH;
  float distance = readDistanceCm();

  // Update LEDs, buzzer, and alarm state
  updateAlarm(motion, distance);

  // Send telemetry at the configured interval
  now = millis();

  if (now - lastPost >= POST_INTERVAL) {
    lastPost = now;

    if (validDHT) {
      sendTelemetry(
        temperature,
        humidity,
        motion,
        distance
      );
    } else {
      Serial.println(
        "[Telemetry] Waiting for a valid DHT22 reading."
      );
    }
  }

  delay(20);
}
