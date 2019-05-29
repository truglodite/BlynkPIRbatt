//////////////////////////////////////////////////////////
// configuration.h
// by: Truglodite
// updated: 5/28/2019
//
// General configuration for BlynkPIRbatts.
//
//////////////////////////////////////////////////////////
#pragma once
// Github users can add a /src/privacy.h with #define privacy, etc.
#ifndef privacy
  const char deviceName[] =   "myDevice";        // This is added to all hostnames & messages
  const char authToken[] =    "myBlynkAppAuthToken";// Blynk app auth token
  //The default battery voltage calibration factor is for a 330k/100k divider & 1 cell Lithium Ion, but can be adjusted for your needs below.
  const float vbattRatio =    4.129;
  const char ssid[] =         "myWifiSSID";      // Wifi SSID
  const char pass[]=          "myWifiPassword";  // Wifi WPA2 password
  const char* update_path =   "/firmware";       // OTA webserver update directory
  const char* update_username = "myOtaUsername"; // OTA username
  const char* otaPassword =   "myOtaPassword";   // OTA password
  //IPAddress staticIP          (192,168,1,0);   // Static local IP (setup your router accordingly)
  //byte mac[] =                {0xDE,0xAD,0xBE,0xEF,0xFE,0xED};// Wifi MAC
#endif

// General configs below ///////////////////////////
#define pirPin              13    // physical pin: PIR output
#define holdEnablePin       12    // physical pin: "stay on" output
#define firmwareVpin        V0    // OTA Button
#define armButtonVpin       V1    // button to arm/disarm
#define ledVpin             V2    // LED to indicate status
#define triggersVpin        V3    // slider to indicate # of triggers
#define batteryVpin         V4    // battery Voltage display
#define pirTimeout          30    // seconds after PIR low before "turning off"
#define triggersMax         50    // set triggers to 0 when > this value
#define otaTimeout          300   // seconds to wait before OTA cancels
#define blynkTimeout        5     // seconds to wait for connect before restart
#define maxOnTime           60    // seconds before restart globally
#define batteryMonitor            // comment out if not using a voltage divider
#ifdef batteryMonitor
  #define vbattLow          3.3   // voltage for low voltage notify
  #define vbattCrit         3.0   // voltage when "all deepsleep" mode starts
#endif
const char hostNameX[] =           "ESP-";               // hostname prefix, default "ESP-[deviceName]"
const char notifyLowBattX[] =      ": Battery Low";      // low battery notification text
const char notifyCritBattX[] =     ": Battery Critical!";// critical battery notification text
const char notifyOTAreadyX[] =     "OTA Waiting\nhttp://"; // IPAddress.Local() and update_path are appended to this
const char notifyOTAtimeoutX[] =   ": OTA Timeout!";     // OTA timeout notification text
const char notifyPIRX[] =          ": Movement!";        // Armed PIR notification text
