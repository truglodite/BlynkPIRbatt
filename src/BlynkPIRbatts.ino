/* blynkPIRbattsHttpOTA3.ino
   by: Truglodite
   Updated 5/22/2019

   This code provides for a long lasting battery powered PIR alarm using the Blynk app,
   ESP8266-12,a PIR motion sensor, and a few resistors & a diode. This version:
   removes Blynk.begin(), and adds Blynk.config() & WiFi.begin(). Handling
   of false triggering PIR's forces resuming regardless of trigger state.
   Shutdown uses deepSleep to prevent false triggers from esp8266 rfi.

   All user configs are in configuration.h.
 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <BlynkSimpleEsp8266_SSL.h>
extern "C" {
  #include "user_interface.h"
}

#ifdef testBoard
  #define debug
  #define BLYNK_PRINT Serial
#endif
#ifdef privacy
  #include "privacy.h"
#endif
#include "configuration.h"

// Globals ////////////////////////////////////////////////
unsigned long lastPirHigh = 0;                           // Holder for PIR timeout timer
unsigned long otaStartTime = 0;                          // Holder for OTA timeout timer
unsigned long startConnectTime = 0;  // Holder for time when connection attemps are started
int armButton = 0;                                       // Holder for arm button state
int nTriggers = 0;                                       // Holder for # of triggers (also on app)
bool areAllSynced = 0;                                   // Blynk.syncAll() only after wake up
bool isArmButtonSet = 0;                                 // Blynk.run() until variables are downloaded
bool isTriggersSliderSet = 0;                            // ...
bool fwButton = 0;                                       // Holder for firmware button position
bool isFWbuttonSet = 0;                                  // Holder for sync finished/unfinished status
bool OTAnotificationSent = 0;                            // Holder for OTA ready notification
bool batteryLow = 0;                                     // Low battery flag
int state = 0;                                           // Holder for state machine variable
IPAddress myIP;
#ifdef batteryMonitor
  float vbatt = 0.0;                                     // Holder for battery vbatt
#endif

// Declare and set the null arrays
char hostName[sizeof(hostNameX) + sizeof(deviceName) + 1] = {0};
char notifyLowBatt[sizeof(notifyLowBattX) + sizeof(deviceName) + 1] = {0};
char notifyCritBatt[sizeof(notifyCritBattX) + sizeof(deviceName) + 1] = {0};
char notifyOTAready[sizeof(notifyOTAreadyX) + 24 + sizeof(update_path) + 1] = {0};
char notifyOTAtimeout[sizeof(notifyOTAtimeoutX) + sizeof(deviceName) + 1] = {0};
char notifyPIR[sizeof(notifyPIRX) + sizeof(deviceName) + 1] = {0};

// Init web update server
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

void setup()  {
  pinMode(holdEnablePin, OUTPUT);
  digitalWrite(holdEnablePin,HIGH); //Make sure we stay on until we're ready to sleep
  #ifdef debug
    Serial.begin(115200);
    Serial.println();
    Serial.println("Debug...");
    Serial.println("Holding enable pin high");
  #endif

  #ifdef batteryMonitor                 // Read battery voltage
    #ifdef debug
      Serial.println("Reading battery volts");
    #endif
    vbatt = analogRead(A0);
    vbatt = vbatt * (vbattRatio / 1024.0); // 4.2 is the nominal voltage of the 18560 battery
    #ifdef testBoard
      vbatt = 4;
    #endif
    if(vbatt < vbattCrit) { // vbatt critical, shutdown immediately
      #ifdef debug
        Serial.println("Critical Battery Shut Down...");
      #endif
      digitalWrite(holdEnablePin,LOW);
      while(1<2) { // when PIR goes low we will reset
        yield();
      }
    }
    if(vbatt <= vbattLow) batteryLow = 1;
  #endif

  pinMode(pirPin, INPUT);
  lastPirHigh = millis();          //We've already seen PIR high, mark it now

  // Combine , except ones that need an IP
  sprintf(hostName, "%s%s", hostNameX, deviceName);
  sprintf(notifyLowBatt, "%s%s", deviceName, notifyLowBattX);
  sprintf(notifyCritBatt, "%s%s", deviceName, notifyCritBattX);
  sprintf(notifyOTAtimeout, "%s%s", deviceName, notifyOTAtimeoutX);
  sprintf(notifyPIR, "%s%s", deviceName, notifyPIRX);

  #ifdef debug
    Serial.println("Connect Wifi...");
  #endif
  WiFi.mode(WIFI_STA);             // Configure & connect to wifi
  //WiFi.setOutputPower(10);
  wifi_station_set_hostname(hostName);
  wifi_station_connect();
  WiFi.begin(ssid,pass);
  #ifdef debug
    Serial.println("Config Blynk...");
  #endif
  Blynk.config(auth0);             // Default Blynk server

  checkConnection();               // Connect to Blynk

  // combine this string now that we have an IP
  sprintf(notifyOTAready, "%s%s%s", notifyOTAreadyX, myIP.toString().c_str(), update_path);

  //Initialize OTA server
  #ifdef debug
    Serial.print("Initializing OTA: ");
  #endif
  MDNS.begin(hostName);
  httpUpdater.setup(&httpServer, update_path, update_username, otaPassword);
  httpServer.begin();
  MDNS.addService("http", "tcp", 80);
  #ifdef debug
    Serial.println("Ready");
  #endif
}
//////////////////////////////////////////////////////////////////////////
// This function will run every time Blynk connection is established
BLYNK_CONNECTED() {
  if (!areAllSynced) {   //Only on first connect, sync all the Vpins
    #ifdef debug
      Serial.print("Syncing with Blynk:");
    #endif
    Blynk.syncAll();
    #ifdef debug
      Serial.println("Synced");
    #endif
    BLYNK_LOG("Synced all");
    areAllSynced = 1;
  }
}

BLYNK_WRITE(firmwareVpin) {  // Sync firmware button from app, and set the flag
  #ifdef debug
    Serial.print(firmwareVpin);
    Serial.print(" button: ");
  #endif
  fwButton = param.asInt();
  #ifdef debug;
    Serial.println(fwButton);
  #endif
  isFWbuttonSet = 1;
}
BLYNK_WRITE(armButtonVpin) {     //read arm/disarm value from app Vpin
  #ifdef debug
    Serial.print(armButton);
    Serial.print(" button: ");
  #endif
  armButton = param.asInt();
  #ifdef debug
    Serial.println(armButton);
  #endif
  isArmButtonSet = 1;
}
BLYNK_WRITE(triggersVpin) {      //Read # of triggers from app Vpin
  #ifdef debug
    Serial.print(triggersVpin);
    Serial.print(" reads: ");
  #endif
  nTriggers = param.asInt();  //!Won't sync with value display unless it contains value :(
  #ifdef debug
    Serial.println(nTriggers);
  #endif
  isTriggersSliderSet = 1;
}
//////////////////////////////////////////////////////////////////////////
void loop() {
  if(state==1 || state==3) {}   //We don't need Blynk.run() while wifi is off
  else Blynk.run();             //loop while wifi is on

  // FW button on & battery OK: send a notification, set flag, start timer
  if(isFWbuttonSet && fwButton && !OTAnotificationSent && !batteryLow) {
    #ifdef debug
      Serial.println("Sending OTA notification");
    #endif
    Blynk.notify(notifyOTAready);
    OTAnotificationSent = 1;
    #ifdef debug
      Serial.println("OTA waiting... ");
    #endif
    otaStartTime = millis();
  }
  // Battery too low to OTA, send a different notification
  else if(isFWbuttonSet && fwButton && !OTAnotificationSent)  {
    #ifdef batteryMonitor
      #ifdef debug
        Serial.println("Sending low batt OTA notification");
      #endif
      Blynk.notify("Batt too low for OTA...");
      OTAnotificationSent = 1;
    #endif
  }

  // FW button on, battery OK: handle OTA calls
  if(isFWbuttonSet && fwButton && !batteryLow) {
    httpServer.handleClient();
    MDNS.update();
    //httpServer.setLedPin(LED_BUILTIN, LOW);//attempt to flash led during firmware upload
    if(millis() - otaStartTime > 1000*otaTimeout)  { // OTA timeout... reset so we send another notification
      #ifdef debug
        Serial.println("OTA timeout, resetting... ");
      #endif
      digitalWrite(holdEnablePin,LOW);
      //ESP.deepSleep(0);
      //delay(500);
    }
  }

  // FW button off, or battery too low to OTA: do PIR routine
  if(!fwButton && isFWbuttonSet && isArmButtonSet && isTriggersSliderSet) {
    doStuff();
  }
}
//////////////////////////////////////////////////////////////////////////
void doStuff(void)  {         //the main routine that runs after the Vpins are downloaded
    switch(state) {
      //  state:
      //  0 = Wake, send message(s) and turn app LED on
      //  1 = PIR off <=30sec, wait
      //  2 = PIR off >30sec, turn off app LED, turn off wifi, set ch_pd low
      //  3 = wait 30sec, deepsleep
      //
      case 0:{ //Send alarm "on" stuff
        OTAnotificationSent = 0;  //reset flag so we can get another notification
        nTriggers++;
        if(nTriggers > triggersMax) nTriggers = 0; //Reset triggers if it's too big
        #ifdef debug
          BLYNK_LOG("Updating Vpins");
          Serial.println("Updating V-Pins");
        #endif
        Blynk.virtualWrite(statusVpin,1023);
        Blynk.virtualWrite(triggersVpin,nTriggers);

        if(armButton) {                //only send email/notify if armed
          #ifdef debug
            //Serial.println("Sending alarm email");
            Serial.println("Sending alarm notification");
          #endif
          //Blynk.email("test@msn.net", "From Blynk", stringMail);
          Blynk.notify(notifyPIR);
        }
        yield();  //ensure data is sent before disconnecting
        #ifdef debug
          Serial.print("Disconnecting Wifi: ");
        #endif
        Blynk.disconnect();
        WiFi.disconnect();           // Shut down wifi to save battery and get a clean adc read
        wifi_station_disconnect();
        WiFi.mode(WIFI_OFF);
        WiFi.forceSleepBegin();
        yield(); //this delay is required for wifi to actually shut down
        #ifdef debug
          Serial.println("wifi off");
        #endif

        state = 1;
        #ifdef debug
          Serial.print("Waiting for PIR timeout: ");
        #endif
        break;
      }

      case 1:{ //Wait for PIR to timeout
        if(millis() < maxOnTime*1000) {  //Not a false positive PIR yet...
          #ifdef testBoard
            state = 2;  //no sense in waiting for a PIR if we don't have one connected
            return;
          #endif
          if(digitalRead(pirPin)) {  //reset timer if PIR triggers while waiting
            lastPirHigh = millis();
            return;
          }
          else if(millis()-lastPirHigh > pirTimeout*1000)  {  //PIR timeout... move on
            state = 2;
            #ifdef debug
              Serial.println("normal time out");
            #endif
            return;
          }
          yield(); //delay to prevent watchdog timer reset
        }
        else {  // We must have a PIR that is false triggering... move on
          state = 2;
          #ifdef debug
            Serial.println("False Triggering!");
          #endif
        }
        break;
      }

      case 2:{  // Read battery, send voltage & alarm "off" stuff, turn wifi off, wait 30sec, then disable
        #ifdef debug
          Serial.println("Reconnecting WiFi: ");
        #endif
        WiFi.forceSleepWake();                           // Turn wifi on
        WiFi.mode(WIFI_STA);
        wifi_station_set_hostname(hostName);
        wifi_station_connect();
        WiFi.begin(ssid,pass);                     // Default Blynk server
        #ifdef debug
          Serial.print("Connect Blynk: ");
        #endif
        checkConnection();

        #ifdef batteryMonitor                            // Update battery status/notify
          #ifdef debug
            Serial.print("Sending Volts: ");
            Serial.println(vbatt);
          #endif
          Blynk.virtualWrite(batteryVpin,vbatt);
          if(batteryLow) {
            #ifdef debug
              Serial.println("Sending low batt notification");
            #endif
            Blynk.notify(notifyLowBatt);
          }
        #endif

        #ifdef debug
          Serial.println("Sending LED OFF");
        #endif
        Blynk.virtualWrite(statusVpin,0);        // Turn off LED
        Blynk.run();
        yield(); // ensure data gets sent before disconnecting

        #ifdef debug
          Serial.print("Disconnecting wifi: ");
          BLYNK_LOG("Disconnecting");
        #endif
        Blynk.disconnect();
        WiFi.disconnect();           // Shut down wifi now to prevent false restarts
        wifi_station_disconnect();
        WiFi.mode(WIFI_OFF);
        WiFi.forceSleepBegin();
        yield();

        #ifdef debug
          Serial.println("wifi off");
          Serial.print("Setting CH_PD:");
        #endif
        digitalWrite(holdEnablePin,LOW);
        // If PIR is already low we should be off by now...
        #ifdef debug
          Serial.println("low");
        #endif
        state = 3;
        break;
      }

      case 3:{ //Do nothing until PIR disables us
        #ifdef testBoard
          ESP.reset();
        #endif
        yield();  //delay to prevent watchdog timer reset
        if(millis()-lastPirHigh > maxOnTime)  ESP.deepSleep(0);  //stop PIR errors
        break;
      }

      default:{ //This should never happen
        #ifdef debug
          Serial.println("INVALID STATE!!!");
        #endif
        digitalWrite(holdEnablePin,LOW);
        ESP.deepSleep(0);
        yield();
        break;
      }
    }
    return;
}

// Check connection to Blynk Server ////////////////////////////////
void checkConnection(void) {
  if(!Blynk.connect()) {           // We aren't connected
    startConnectTime = millis();     // Reset connection timer
    #ifdef debug
      Serial.print("Connecting to Blynk: ");
    #endif
    while (!Blynk.connect()) {       // Loop until connected
      // Restart after timeout... or sleep until PIR wakes us up
      if(millis() - startConnectTime > 1000*blynkTimeout) {
        #ifdef debug
          Serial.println("Failed to connect... shutting down.");
        #endif
        digitalWrite(holdEnablePin,LOW);
        ESP.restart();
      }
      yield();  // feed the dog
    }
    myIP = WiFi.localIP();  // save IP address
  }
  //Already Connected
  #ifdef debug
    Serial.println(myIP);
  #endif
  return;
}
