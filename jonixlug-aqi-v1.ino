/*
JonixLUG-AQI V1 Air Quality Indicator
Advanced dust sensor (PM2.5 and PM10) with fine tuning temperature and humidity algorithm.
July 2019
License: GPLv3
Contacts: https://gitlab.com/JonixLUG jonixlug@gmail.com
  Dario P. & Vincenzo Q. (Team JonixLUG)
Partners: Piersoft https://www.piersoft.it/ Peacelink https://www.peacelink.it/
~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
This code is intended to be used on a Wemos mini D1 https://www.wemos.cc/
in combination with:
- DHT22 (temperature and humidity) https://learn.adafruit.com/dht
- SDS011 (PM2.5 and PM10 dust sensor) by Nova Fitness Co.,Ltd http://www.inovafitness.com
and some libraries included in the project's files:
- Adafruit DHT Humidity & Temperature Sensor https://travis-ci.org/adafruit/DHT-sensor-library.svg?branch=master
- Adafruit Unified Sensor Driver https://github.com/adafruit/Adafruit_Sensor
- SDS011 Arduino library By R. Zschiegner reviewed (sds011.cpp) by Piersoft https://gitlab.com/JonixLUG/jonixlug-aqi/SDS011-ricki-z-Piersoft
- RunningAverage https://github.com/RobTillaart/Arduino/tree/master/libraries/RunningAverage
- base64_arduino Base64 encoder/decoder https://travis-ci.org/Densaugeo/base64_arduino.svg?branch=master
Compile it with Arduino IDE 1.8.9 or later https://www.arduino.cc/en/Main/Software#download
Data will be sent to ThingSpeak's server, so it need a free or paid account. https://thingspeak.com
~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
~ Warning: This sketch will not work "as is". It need to be modified adding your personal TeamSpeak's data. ~
~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
# First of all, setup the software environment, installing Arduino IDE 1.8.9 or later and adding this URL:
  https://arduino.esp8266.com/stable/package_esp8266com_index.json
  in "File -> Preferences -> Additional Boards Manager URLs" field.
# Move "libraries" folder to Arduino IDE's main libraries folder location (eg. ~/Arduino/libraries for GNU/Linux)
# Change in this sketch (ThingSpeak Settings) lines: "TeamSpeak channel" and "upload API key", with your personal account data.
# Make a WiFi access point with SSID "jonixlug-aqi" and (the same) "jonixlug-aqi" as WPA-PSK password, or better,
change (in Wi-Fi Settings) "ssid" and "pass" variables with yours.
# Next steps are wiring sensors to Wemos mini D1's digital inputs.
DHT22 need to be connected to 3,3V Vcc, Ground and VDD serial data pin
SDS011 need 5Vcc, Ground, TX and RX pins
Wemos mini D1 can be programmed and powered by onboard mini USB port.
Warning: we need 2 Ground pins for both DHT22 and SDS011 sensors, but Wemos mini D1 has only 1 Ground Pin.
Don't panic, it can be used simultaneously for all sensors. Simply wire it together.
Please visit https://gitlab.com/JonixLUG/jonixlug-aqi for schemes and infos about wiring and power your Advanced dust sensor.
# Finally verify and if there're not errors, upload sketch on Wemos mini D1 Board.
Tips: Open a "monitor" on Arduino IDE and read debug infos to make sure things are going well. 
Data will be sent to ThingSpeak server and can be visualized as graph by Matlab and reused as json, XML and CSV format.
Power source can be anything with miniUSB 5Vdc @ at least 500mA output (better 1A). 
*/
// DHT sensor
#include <DHT.h>
#include <DHT_U.h>
const int pinDHT = 4; // serial (DAT) on DHT22 -> PIN D2 (Wemos-D1-R1 is GPIO4 and 4 on Arduino IDE)
#define DHTTYPE DHT22 // Define DHT** sensor (default DHT22)
DHT dht(pinDHT, DHTTYPE);
static char tempCelsius[7];
static char tempHumidity[7];

// SDS011 sensor
#include <SDS011.h>
SDS011 sds;
const int sdsRX = 14; // Pin D5 = GPIO14 = SDS011 RX
const int sdsTX = 12; // Pin D6 = GPIO12 = SDS011 TX
float p10, p25;
int error;
#include "base64.hpp"
#include "RunningAverage.h"
RunningAverage pm25Stats(10);
RunningAverage pm10Stats(10);
RunningAverage temperatureStats(10);
unsigned int Pm25 = 0;
unsigned int Pm10 = 0;

// Wemos-D1-R1 WiFi
#include <WiFiClientSecure.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>

// Wi-Fi Settings
const char* ssid = "jonixlug-aqi"; // your wireless network name (SSID)
const char* pass = "jonixlug-aqi"; // your Wi-Fi WPA network password
const int httpPort = 80;

// ThingSpeak Settings
const int channelID = 000000; // your ThingSpeak Channel
String writeAPIKey = "LLNNLNLLNNNLLNLN"; // write API key for your ThingSpeak Channel
const char* server = "184.106.153.149"; // ThingSpeak server's IP

// Sleep Time
const int sleepTime = 60 * 15; // Default sleep time (ms) between each sample

// Initilize Wifi client
WiFiClient client;

// Begin Setup
void setup() {
  Serial.begin(9600);
  // Initialize sensors
  sds.begin(sdsTX, sdsRX); // Begin SDS011 sensor - Pin D6 (SDS011 TX) & Pin D5 (SDS011 RX)
  delay(10);
  dht.begin(); // Begin DHT sensor
}
// Begin loop
void loop() {
  pm25Stats.clear();
  pm10Stats.clear();
  temperatureStats.clear();
  // Connect & check WiFi
  connectWiFi();
  checkWiFi();
  // Verify server connection
  verifyConnection();
  // Read DHT22 data
  float h = dht.readHumidity(); // Read Humidity
  float t = dht.readTemperature(); // Read temperature (Celsius)
  // Verify data. If wrongs restart sampling loop
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read DHT sensor data!");
    strcpy(tempCelsius, "Temp Fail");
    strcpy(tempHumidity, "Hum Fail");
    Serial.println("Sleep 1 minute and retry");
    disconnectAndSleep();
    delay(60000);
    loop(); // Restart loop
  }
  else {
    // Set temperature values in Celsius & Humidity in %
    float hic = dht.computeHeatIndex(t, h, false);
    dtostrf(hic, 6, 2, tempCelsius);
    dtostrf(h, 6, 2, tempHumidity);
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.print(" *C ");
    Serial.print("Humidity': ");
    Serial.print(h);
    Serial.println(" %\t");
    Serial.flush();  // Wait for complete serial string
  }
  // Wake up and read SDS011
  sds.wakeup(); // wakeup SDS011 sensor
  Serial.println("Calibrate SDS011 - Start fan (15 sec)");
  delay(15000);
  // Read 10 samples of pm10 & pm2.5
  for (int i = 0; i < 10; i++) {
    error = sds.read(&p25, &p10);
    if (error || p25 <= 0 || p10 <= 0) {
      Serial.println("Error SDS011 not working! Sleep 1 minute and retry");
      disconnectAndSleep();
      Serial.println("Put SDS011 sensor in stand-by mode");
      sds.sleep();
      delay(60000);
      loop();
    }
    if (!error && p25 > 999 && p10 > 1999) {
      Serial.println("Error SDS011 wrong reads! Sleep 1 minute and retry");
      disconnectAndSleep();
      Serial.println("Put SDS011 sensor in stand-by mode");
      sds.sleep();
      delay(60000);
      loop();
    }
    if (!error && p25 < 999 && p10 < 1999) {
      pm25Stats.addValue(p25);
      pm10Stats.addValue(p10);
    }
    Serial.println("Average PM10: " + String(pm10Stats.getAverage()) + ", PM2.5: " + String(pm25Stats.getAverage()));
    delay(1500);
  }
  Serial.println("Put SDS011 sensor in stand-by mode");
  sds.sleep();
  // Get pm10 and pm2.5 average data
  float pm25n = normalizePM25(float(pm25Stats.getAverage()), h); // normalize PM10 related to humidity
  float pm10n = normalizePM10(float(pm10Stats.getAverage()), h); // normalize PM2.5  related to humidity
  // Format reads
  String body = "field1=";
  body += String(pm10n);
  body += "&field2=";
  body += String(pm25n);
  body += "&field3=";
  body += String(t);
  body += "&field4=";
  body += String(h);
  // Generate and send URL to the server
  String url = "POST /update?api_key=" + writeAPIKey + "&field1=" + String(pm10n) + "&field2=" + String(pm25n) + "&field3=" + String(t) + "&field4=" + String(h) + " HTTP/1.1\r\nHost: 184.106.153.149\r\nConnection: close\r\n\r\n";
  Serial.print("Requesting URL: ");
  Serial.println(url);
  client.print(url); // Print data to the server
  delay(1000);
  String line = client.readStringUntil('OK'); // Use '</html>' for verbose and complete output
  Serial.println(line);
  // Put WiFi in sleep mode
  Serial.println("Sleep " + String(sleepTime / 60) + " minute(s) until next readings");
  disconnectAndSleep(); // Turn OFF WiFi
  delay(sleepTime * 1000);
  Serial.println("Restart Sampling");
}
// loop end

// Normalize data function
float normalizePM25(float pm25, float humidity) {
  return pm25 / (1.0 + 0.48756 * pow((humidity / 100.0), 8.60068));
}
float normalizePM10(float pm10, float humidity) {
  return pm10 / (1.0 + 0.81559 * pow((humidity / 100.0), 5.83411));
}

// Connect WiFi
void connectWiFi() {
  Serial.println("\n------------- Begin connection ------------");
  Serial.print("Try to connect ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  wifi_station_connect();
  WiFi.begin(ssid, pass);
}

// Check WiFi association function
void checkWiFi() {
  for (int i = 0; i < 60; i++) {
    if (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    else break;
  }
  if (WiFi.status() != WL_CONNECTED) {  // if it take too much time, reconnect
    sds.sleep();
    Serial.println();
    Serial.println("No wiFi connection. Sleep 1 minute and retry");
    delay(60000);
    loop();
  }
  // If WiFi connection is up, write SSID & IP on serial monitor
  Serial.print("Connected to: ");
  Serial.println(ssid);
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// Verify connection to ThingSpeak's server
void verifyConnection() {
  if (!client.connect(server, httpPort)) {
    Serial.println("Connection to the server failed");
    sds.sleep();
    Serial.println("Sleep 1 minute and retry connection");
    delay(60000);
    loop();
  }
  Serial.println("\nConnected! server is online");
}

void disconnectAndSleep() {
  WiFi.disconnect();
  delay(10); // warning
  WiFi.mode(WIFI_OFF);
  delay(10); // warning
  WiFi.forceSleepBegin();
  delay(10);
  Serial.println("Disconnect And Sleep");
}

// Process SDS011 serial data function
void ProcessSerialData() {
  uint8_t mData = 0;
  uint8_t i = 0;
  uint8_t mPkt[10] = {0};
  uint8_t mCheck = 0;
  while (Serial1.available() > 0) {
    // from www.inovafitness.com packet format:
    // AA C0 PM25_Low PM25_High PM10_Low PM10_High 0 0 CRC AB
    mData = Serial1.read();
    delay(2); //wait until packet is received
    //Serial.println(mData);
    //Serial.println("*");
    if (mData == 0xAA) { //head1 ok
      mPkt[0] =  mData;
      mData = Serial1.read();
      if (mData == 0xc0) { //head2 ok
        mPkt[1] =  mData;
        mCheck = 0;
        for (i = 0; i < 6; i++) { //data recv and crc calc
          mPkt[i + 2] = Serial1.read();
          delay(2);
          mCheck += mPkt[i + 2];
        }
        mPkt[8] = Serial1.read();
        delay(1);
        mPkt[9] = Serial1.read();
        if (mCheck == mPkt[8]) { //crc ok
          //Serial.flush();
          //Serial.write(mPkt,10);
          Pm25 = (uint16_t)mPkt[2] | (uint16_t)(mPkt[3] << 8);
          Pm10 = (uint16_t)mPkt[4] | (uint16_t)(mPkt[5] << 8);
          if (Pm25 > 9999)
            Pm25 = 9999;
          if (Pm10 > 9999)
            Pm10 = 9999;
          //Serial.println('Pm25'+Pm25);
          //Serial.println('Pm10'+Pm10);
          // get one good packet
          return;
        }
      }
    }
  }
}
