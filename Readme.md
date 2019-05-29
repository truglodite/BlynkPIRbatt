# BlynkPIRbatts
This project aims to maximize battery life of an ESP8266 module with PIR motion sensor that communicates with Blynk. It makes use of a simple auxiliary circuit that allows the PIR to enable/disable the ESP using the CH_PD pin. The circuit also allows the ESP to keep itself enabled to finish code execution after the PIR goes low. Inactive draw is reduced to only the PIR idle current when this method is used. With a AM312 PIR sensor, modified with a low draw schmitt buffer (ie NL17SG17) on the output and low quiescent LDO (ie ADP160AU), idle draw of the device was measured at 15uA!

[See Auxilliary circuit here](https://www.esp8266.com/viewtopic.php?f=11&t=4458&start=68)

[See PIR mod here](https://www.esp8266.com/viewtopic.php?p=82466#p82466)

*Project Inspired by: Barnabybear@esp8266.com*

## Code Features:
- Blynk app ready
- Browser OTA firmware update controlled w/ Blynk app button
- Blynk notification ARM/DISARM button
- Displays motion status in app LED
- Battery monitoring
- Sends trigger count to allow tracking motion history (chart in app)
- Minimizes wifi on time
- Avoids false triggering sensitive PIR's by waiting after wifi off before shutdown
- Verifies data sent to Blynk server before shutting off wifi
- Sends notification when verification finds mismatched data

## Basic Operation:
*User configurations are in configuration.h.*
- Wakes on PIR rising edge (PIR brings CH_PD high)
- Write io12 HIGH to maintain high CH_PD (prevents PIR from turning us off when low)
- Read battery voltage, if critical, write CH_PD low and loop (wait for PIR to shut it down)
- Wifi on, connect to Blynk
- Sync OTA and arm buttons, and Trigger Count slider from Blynk server
- If FW OTA is ON & battery is not low, send OTA notification and wait for FW upload (5min default, then reset)
- If FW OTA is OFF, send occupied signals to the app, resync Vpins (LED on, notification if enabled, increment triggers)
- Verify data sent correctly
- Disconnect from Blynk and turn wifi off
- Listen to the PIR for additional triggers
- Continue after 30sec without PIR triggers
- Wifi on & reconnect to Blynk
- Send LED off and battery voltage to the app, send notification if battery is low, resync vpins
- Verify data sent correctly
- Turn wifi off & write io12 LOW... if PIR is low, ESP is disabled now
- Wait 30sec for PIR to disable us, if not deepsleep

## Default Pinout
ESPpin | Description
------ | -------------------
io1  | 1k to Enable diode/resistors [set high first thing after bootup, set low to sleep]
io13 | 1k to PIR output
io2  | 10k High
io15 | 10k Low
A0   | Battery voltage divider (1.0V max)
EN   | Enable diode/resistors
RST  | 10k High

## Default Blynk App Virtual Pins:
Vpin | Interfaces with
------  |  -------------------
V0 | FW OTA Button
V1 | Arm/disarm Button
V2 | PIR status LED
V3 | Triggers Slider
V4 | Battery Voltage

## Privacy.h
This code includes a "privacy" macro that is disabled by default. Most users can ignore this and just edit their info in to configuration.h; the code will work as intended with the same level of privacy/security. Privacy.h and the privacy macro are there purely for the convenience of Github users. If you use github, you can create a "privacy.h" file in the "src/"" directory, copy the privacy macro section from "configuration.h" to it (just the part between the #define lines), edit in your personal info, and uncomment "#define privacy" in "BlynkPIRbatts.ino". The code will then compile properly with your personal info from privacy.h, however when committing to github, privacy.h will not be shared. An example privacy.h file is not included in the repository to avoid the potential of inadvertently sending the private info to github.
