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
