//////////////////////////////////////////////////////////
// configuration.h
// by: Truglodite
// updated: 5/25/2019
//
// General configuration for BlynkPIRbatts.
//
//////////////////////////////////////////////////////////
#pragma once
// Uncomment for developing on devices without the aux circuit connected
//#define testBoard
// Uncomment and add this section to /src/privacy.h if using github
#define privacy

#ifndef privacy
  const char deviceName[] =   "myDevice";        // This is added to all hostnames & messages
  const char auth0[] =        "myBlynkAppAuthToken";// Blynk app auth token
  //The default battery voltage calibration factor is for a 330k/100k divider & 1 cell Lithium Ion, but can be adjusted for your needs below.
  const float vbattRatio =    4.129;
  const char ssid[] =         "myWifiSSID";      // Wifi SSID
  const char pass[]=          "myWifiPassword";  // Wifi WPA2 password
  const char* update_path =   "/firmware";       // OTA webserver update directory
  const char* update_username = "myOtaUsername"; // OTA username
  const char* otaPassword =   "myOtaPassword";   // OTA password (old pass: kwaker5)
  //IPAddress staticIP          (192,168,1,0);   // Static local IP (setup your router accordingly)
  //byte mac[] =                {0xDE,0xAD,0xBE,0xEF,0xFE,0xED};// Wifi MAC
#endif

// General configs below ///////////////////////////
#define pirPin              13                    // Physical pin: PIR output
#define holdEnablePin       12                    // Physical pin: "stay on" output
#define firmwareVpin        V0                    // Firmware OTA Button
#define armButtonVpin       V1                    // Button to arm/disarm
#define ledVpin             V2                    // LED to indicate status
#define triggersVpin        V3                    // Slider to indicate # of triggers
#define batteryVpin         V4                    // Battery Voltage display
#define pirTimeout          10                    // Seconds after PIR low to wait before "turning off"
#define triggersMax         50                    // Set trigger count to 0 when it is > this value
#define otaTimeout          300                   // Seconds to wait for FW before OTA cancels and sleeps
#define blynkTimeout        5                     // Seconds to wait for connection before restart
#define maxOnTime           60                    // Seconds to wait for PIR timeout before restart
#define batteryMonitor                            // Comment out if not using a voltage divider
#ifdef batteryMonitor
  #define vbattLow          3.3                   // battery voltage for low voltage mode
  #define vbattCrit         3.0                   // battery voltage for "all deepsleep" mode
#endif
const char hostNameX[] =           "ESP-";               // hostname prefix, default "ESP-[deviceName]"
const char notifyLowBattX[] =      ": Battery Low";      // Low battery notification text
const char notifyCritBattX[] =     ": Battery Critical!";// Critical battery notification text
const char notifyOTAreadyX[] =     "OTA Waiting\nhttp://"; // IPAddress.Local() and update_path are appended to this
const char notifyOTAtimeoutX[] =   ": OTA Timeout!";     // OTA Timeout notification text
const char notifyPIRX[] =          ": Movement!";        // Armed PIR notification text
