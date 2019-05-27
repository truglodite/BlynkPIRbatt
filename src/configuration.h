//////////////////////////////////////////////////////////
// configuration.h
// by: Truglodite
// updated: 5/25/2019
//
// General configuration for BlynkPIRbatts.
// Enter your private information in privacy.h
//////////////////////////////////////////////////////////

//#define testBoard                              // Uncomment for developing on devices without the aux circuit connected

// General configs below ///////////////////////////
#define pirPin              13                    // Physical pin: PIR output
#define holdEnablePin       12                    // Physical pin: "stay on" output
#define firmwareVpin        V0                    // Firmware OTA Button
#define armButtonVpin       V1                    // Button to arm/disarm
#define statusVpin          V2                    // LED to indicate status
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
