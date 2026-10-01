/*
  JonixLUG-AQI V2 — Centralina qualità dell'aria
  Sensore particolato (PM2.5 / PM10) con compensazione temperatura e umidità.

  Hardware:
    - Wemos D1 (ESP8266)
    - BME280 su I2C (D3/D4) — temperatura, umidità, pressione
    - SDS011 su pin D5/D6 (GPIO14/GPIO12) — particolato

  Invio dati a:
    - Sensor.Community (rete globale citizen science)
    - openSenseMap (rete accademica open data)
    - InfluxDB su VPS (dashboard Grafana personalizzata)

  Basato sul progetto originale JonixLUG ABC (GPLv3, 2019)
  https://gitlab.com/JonixLUG/jonixlug-aqi
  Autori originali: Dario P. & Vincenzo Q. (Team JonixLUG)
  Partner: Piersoft (https://www.piersoft.it/), Peacelink (https://www.peacelink.it/)

  V2 by William Donzelli — APS FareZero Makers Fab Lab — https://farezero.org
  License: GPLv3
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SDS011.h>
#include <RunningAverage.h>

#include "config.h"

#define FW_VERSION "farezero-aqi-2.1.0"

// --- Sensori ---
#define SDS_TX  12   // D6 = GPIO12
#define SDS_RX  14   // D5 = GPIO14

Adafruit_BME280 bme;
SDS011 sds;
RunningAverage pm25Stats(10);
RunningAverage pm10Stats(10);

bool bmeOk = false;

// --- Normalizzazione PM in base all'umidità ---
float normalizePM25(float pm25, float humidity) {
  return pm25 / (1.0 + 0.48756 * pow(humidity / 100.0, 8.60068));
}

float normalizePM10(float pm10, float humidity) {
  return pm10 / (1.0 + 0.81559 * pow(humidity / 100.0, 5.83411));
}

// --- WiFi ---
bool connectWiFi() {
  Serial.print("\nConnessione a ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  for (int i = 0; i < 60; i++) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("Connesso — IP: ");
      Serial.println(WiFi.localIP());
      return true;
    }
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi non disponibile.");
  return false;
}

void disconnectWiFi() {
  WiFi.disconnect();
  delay(10);
  WiFi.mode(WIFI_OFF);
  delay(10);
  WiFi.forceSleepBegin();
  delay(10);
}

// --- Sensor.Community ---
void sendToSensorCommunity(float pm10, float pm25, float temp, float hum, float pres) {
  if (!ENABLE_SENSOR_COMMUNITY) return;

  String sensorId = "esp8266-" + String(ESP.getChipId());
  Serial.print("[SC] Sensor ID: ");
  Serial.println(sensorId);

  WiFiClient wc;
  HTTPClient http;

  // POST 1: dati particolato (SDS011, pin 1)
  String pmBody = "{\"software_version\":\"" + String(FW_VERSION) + "\","
    "\"sensordatavalues\":["
    "{\"value_type\":\"P1\",\"value\":\"" + String(pm10) + "\"},"
    "{\"value_type\":\"P2\",\"value\":\"" + String(pm25) + "\"}"
    "]}";

  http.begin(wc, "http://api.sensor.community/v1/push-sensor-data/");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Pin", "1");
  http.addHeader("X-Sensor", sensorId);

  int code = http.POST(pmBody);
  Serial.print("[SC] PM → ");
  Serial.println(code > 0 ? String(code) : "errore " + String(code));
  http.end();

  delay(500);

  // POST 2: dati climatici (BME280, pin 11)
  String bmeBody = "{\"software_version\":\"" + String(FW_VERSION) + "\","
    "\"sensordatavalues\":["
    "{\"value_type\":\"temperature\",\"value\":\"" + String(temp) + "\"},"
    "{\"value_type\":\"humidity\",\"value\":\"" + String(hum) + "\"},"
    "{\"value_type\":\"BME280_pressure\",\"value\":\"" + String(pres * 100.0) + "\"}"
    "]}";

  http.begin(wc, "http://api.sensor.community/v1/push-sensor-data/");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Pin", "11");
  http.addHeader("X-Sensor", sensorId);

  code = http.POST(bmeBody);
  Serial.print("[SC] BME → ");
  Serial.println(code > 0 ? String(code) : "errore " + String(code));
  http.end();
}

// --- openSenseMap ---
void sendToOpenSenseMap(float pm10, float pm25, float temp, float hum, float pres) {
  if (!ENABLE_OPENSENSEMAP) return;

  struct { const char* id; float value; const char* label; } sensors[] = {
    { OSM_SENSOR_PM10, pm10, "PM10" },
    { OSM_SENSOR_PM25, pm25, "PM2.5" },
    { OSM_SENSOR_TEMP, temp, "Temp" },
    { OSM_SENSOR_HUM,  hum,  "Hum" },
    { OSM_SENSOR_PRES, pres, "Press" },
  };

  WiFiClient wc;
  HTTPClient http;

  for (int i = 0; i < 5; i++) {
    String url = "http://api.opensensemap.org/boxes/" + String(OSM_BOX_ID) + "/" + String(sensors[i].id);
    String body = "{\"value\":" + String(sensors[i].value) + "}";

    http.begin(wc, url);
    http.addHeader("Content-Type", "application/json");

    int code = http.POST(body);
    Serial.print("[OSM] ");
    Serial.print(sensors[i].label);
    Serial.print(" → ");
    Serial.println(code > 0 ? String(code) : "errore " + String(code));
    http.end();
    delay(200);
  }
}

// --- InfluxDB ---
void sendToInfluxDB(float pm10, float pm25, float temp, float hum, float pres) {
  if (!ENABLE_INFLUXDB) return;

  WiFiClient wc;
  HTTPClient http;

  String url = "http://" + String(INFLUX_HOST) + ":" + String(INFLUX_PORT) + "/write?db=" + String(INFLUX_DB);
  String line = "air,sensor=farezero"
    " pm25=" + String(pm25)
    + ",pm10=" + String(pm10)
    + ",temperature=" + String(temp)
    + ",humidity=" + String(hum)
    + ",pressure=" + String(pres);

  http.begin(wc, url);
  http.addHeader("Content-Type", "text/plain");

  int code = http.POST(line);
  Serial.print("[InfluxDB] → ");
  Serial.println(code > 0 ? String(code) : "errore " + String(code));
  http.end();
}

// --- Setup ---
void setup() {
  Serial.begin(9600);
  Serial.println("\n=== JonixLUG-AQI V2 — FareZero ===");
  Serial.print("Chip ID: ");
  Serial.println(ESP.getChipId());

  // BME280 su I2C (indirizzo 0x76 o 0x77)
  bmeOk = bme.begin(0x76);
  if (!bmeOk) bmeOk = bme.begin(0x77);
  if (bmeOk) {
    Serial.println("BME280 trovato.");
  } else {
    Serial.println("BME280 non trovato! Controlla i collegamenti.");
  }

  sds.begin(SDS_TX, SDS_RX);
  delay(10);
}

// --- Loop ---
void loop() {
  pm25Stats.clear();
  pm10Stats.clear();

  if (!connectWiFi()) {
    disconnectWiFi();
    delay(60000);
    return;
  }

  // BME280
  if (!bmeOk) {
    Serial.println("BME280 non disponibile, riprovo tra 1 minuto.");
    disconnectWiFi();
    delay(60000);
    return;
  }

  float t = bme.readTemperature();
  float h = bme.readHumidity();
  float p = bme.readPressure() / 100.0F; // Pa → hPa

  if (isnan(t) || isnan(h) || isnan(p)) {
    Serial.println("Errore lettura BME280, riprovo tra 1 minuto.");
    disconnectWiFi();
    delay(60000);
    return;
  }

  Serial.print("Temp: ");
  Serial.print(t);
  Serial.print(" C  |  Umidita: ");
  Serial.print(h);
  Serial.print(" %  |  Pressione: ");
  Serial.print(p);
  Serial.println(" hPa");

  // SDS011
  sds.wakeup();
  Serial.println("SDS011: calibrazione ventola (15s)...");
  delay(15000);

  float p25, p10;
  int validSamples = 0;

  for (int i = 0; i < 10; i++) {
    int err = sds.read(&p25, &p10);
    if (err || p25 <= 0 || p10 <= 0 || p25 > 999 || p10 > 1999) {
      delay(1500);
      continue;
    }
    pm25Stats.addValue(p25);
    pm10Stats.addValue(p10);
    validSamples++;
    Serial.print("  #");
    Serial.print(validSamples);
    Serial.print(" PM10=");
    Serial.print(pm10Stats.getAverage());
    Serial.print(" PM2.5=");
    Serial.println(pm25Stats.getAverage());
    delay(1500);
  }

  sds.sleep();

  if (validSamples == 0) {
    Serial.println("SDS011: nessun campione valido, riprovo tra 1 minuto.");
    disconnectWiFi();
    delay(60000);
    return;
  }

  float pm25n = normalizePM25(pm25Stats.getAverage(), h);
  float pm10n = normalizePM10(pm10Stats.getAverage(), h);

  Serial.println("--- Valori normalizzati ---");
  Serial.print("PM10: ");
  Serial.print(pm10n);
  Serial.print("  PM2.5: ");
  Serial.println(pm25n);

  // Invio a tutte le piattaforme
  sendToSensorCommunity(pm10n, pm25n, t, h, p);
  sendToOpenSenseMap(pm10n, pm25n, t, h, p);
  sendToInfluxDB(pm10n, pm25n, t, h, p);

  Serial.print("\nSleep ");
  Serial.print(SLEEP_SECONDS / 60);
  Serial.println(" minuti...\n");
  disconnectWiFi();
  delay(SLEEP_SECONDS * 1000);
}
