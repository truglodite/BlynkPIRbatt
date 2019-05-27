# BlynkPIRbatts
This project aims to maximize battery life of an ESP8266 module with PIR motion sensor that communicates with Blynk. It makes use of a simple auxiliary circuit that allows the PIR to enable/disable the ESP using the CH_PD pin. The circuit also allows the ESP to keep itself enabled to finish code execution after the PIR goes low. Inactive draw is reduced to only the PIR idle current when this method is used. With a AM312 PIR sensor, modified with a low draw schmitt buffer (ie NL17SG17) on the output and low quiescent LDO (ie ADP160AU), idle draw of the device was measured at 15uA!

[See Auxilliary circuit here](https://www.esp8266.com/viewtopic.php?f=11&t=4458&start=68)
[See PIR mod here](https://www.esp8266.com/viewtopic.php?p=82466#p82466)

*Project Inspired by: Barnabybear@esp8266.com*

## Code Features:
- Blynk App Ready
- Browser Firmware Update w/ Blynk App Button
- Battery Monitoring
- Blynk notification ARM/DISARM button
- Sends trigger count to track activity
- Minimizes wifi on time
- Avoids false triggering sensitive PIR's by waiting after wifi off before shutdown

## Basic Operation:
*User configurations are in configuration.h.*
- Wakes on PIR rising edge (PIR brings CH_PD high)
- Write io12 HIGH to maintain high CH_PD (prevents PIR from turning us off when low)
- Read battery voltage, if critical, write CH_PD low and loop (wait for PIR to shut it down)
- Connect to Blynk
- Read OTA and arm buttons, and Trigger Count slider from app
- If FW OTA is ON & battery is not low, send OTA notification and wait for FW upload (5min default, then reset)
- If FW OTA is OFF, send occupied signals to the app (LED on, notification if enabled, increment triggers)
- Disconnect from Blynk and turn wifi off
- Listen to the PIR for additional triggers
- Continue after 30sec without PIR triggers
- Turn wifi on & connect to Blynk
- Send LED off and battery voltage to the app
- Send notification if battery is low
- Turn wifi off
- Write io12 LOW... if PIR is low, ESP is disabled
- Wait 30sec for PIR to disable, if not deepsleep

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
