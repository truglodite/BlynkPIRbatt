/*
  blynkPIRbattsHttpOTA3.ino
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
#include "configuration.h"
#ifdef privacy
  #include "privacy.h"
#endif
#ifdef testBoard
  #define debug
  #define BLYNK_PRINT Serial
#endif

// Globals ////////////////////////////////////////////////
unsigned long lastPirHigh = 0;      // Holder for PIR timeout timer
unsigned long otaStartTime = 0;     // Holder for OTA timeout timer
unsigned long startConnectTime = 0; // Holder for time when connection attemps are started
int armButton = 0;                  // Arm button state
int nTriggersServer = 0;            // Blynk server trigger count
int nTriggers = 0;                  // Local trigger count
bool isArmButtonSet = 0;            // Arm V button sync flag
bool isTriggersSliderSet = 0;       // ...
bool isFWbuttonSet = 0;             // ...
bool isLedSet = 0;                  // ...
bool isVbattSet = 0;                // ...
bool fwButton = 0;                  // Firmware OTA button
bool OTAnotificationSent = 0;       // OTA notification flag
bool batteryLow = 0;                // Low battery flag
int state = 0;                      // State machine variable
int ledServerValue = 0;             // Blynk server LED value
IPAddress myIP;
#ifdef batteryMonitor
  float vbatt = 0.0;                // Holder for battery vbatt
  float vbattServerValue = 0.0;
#endif
#define ledHigh 1023
#define ledLow 0

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

// These functions run whenever Blynk.syncVirtualALL() is called,
// or individually with Blynk.syncVirtual(vPin1, vPin2, ...)
BLYNK_WRITE(firmwareVpin) {  // Sync firmware button from app, and set the flag
  #ifdef debug
    Serial.print("V");
    Serial.print(firmwareVpin);
    Serial.print("-OTA = ");
  #endif
  fwButton = param.asInt();
  #ifdef debug
    Serial.println(fwButton);
  #endif
  isFWbuttonSet = 1;
}
BLYNK_WRITE(armButtonVpin) {     //read arm/disarm value from app Vpin
  #ifdef debug
    Serial.print("V");
    Serial.print(armButtonVpin);
    Serial.print("-Arm = ");
  #endif
  armButton = param.asInt();
  #ifdef debug
    Serial.println(armButton);
  #endif
  isArmButtonSet = 1;
}
BLYNK_WRITE(triggersVpin) {      //Read # of triggers from app Vpin
  #ifdef debug
    Serial.print("V");
    Serial.print(triggersVpin);
    Serial.print("-Triggers = ");
  #endif
  nTriggersServer = param.asInt();  //!Won't sync with value display unless it contains value :(
  #ifdef debug
    Serial.println(nTriggersServer);
  #endif
  isTriggersSliderSet = 1;
}
BLYNK_WRITE(ledVpin) {      //Read # of triggers from app Vpin
  #ifdef debug
    Serial.print("V");
    Serial.print(ledVpin);
    Serial.print("-LED = ");
  #endif
  ledServerValue = param.asInt();  //!Won't sync with value display unless it contains value :(
  #ifdef debug
    Serial.println(ledServerValue);
  #endif
  isLedSet = 1;
}
BLYNK_WRITE(batteryVpin) {      //Read # of triggers from app Vpin
  #ifdef debug
    Serial.print("V");
    Serial.print(batteryVpin);
    Serial.print("-Vbatt = ");
  #endif
  vbattServerValue = param.asInt();  //!Won't sync with value display unless it contains value :(
  #ifdef debug
    Serial.println(vbattServerValue);
  #endif
  isVbattSet = 1;
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
          Serial.println("Error: Failed to connect... shutting down.");
        #endif
        digitalWrite(holdEnablePin,LOW);
        ESP.restart();
      }
      yield();  // feed the dog
    }
    myIP = WiFi.localIP();  // save IP address
    #ifdef debug
      Serial.println(myIP);
    #endif
  }
  //Already Connected
  else {
    myIP = WiFi.localIP();  // save IP address
    #ifdef debug
      Serial.print("Already Connected: ");
      Serial.println(myIP);
    #endif
  }
  return;
}
//////////////////////////////////////////////////////////////////////////
// the main routine that runs after primary Vpins are synced
void doStuff(void)  {
  switch(state) {
    //  state:
    //  0 = Wake, send message(s) and turn app LED on, resync pins
    //  1 = Verify data is sent correctly (otherwise -> 4)
    //  2 = wifi off
    //  3 = Wait if PIR off <=30sec
    //  4 = PIR off >30sec, turn off app LED & send voltage, resync pins
    //  5 = Verify data is sent correctly (otherwise -> 6)
    //  6 = turn off, set ch_pd low
    //  7 = wait 30sec, deepsleep
    //
    case 0:{ //Send alarm "on" stuff
      OTAnotificationSent = 0;  //reset flag so we can get another notification
      nTriggers = nTriggersServer + 1;
      if(nTriggers > triggersMax) nTriggers = 0; //Reset triggers if it's too big
      #ifdef debug
        BLYNK_LOG("Updating Vpins");
        Serial.println("Updating V-Pins");
      #endif
      Blynk.virtualWrite(ledVpin,ledHigh);  //send value
      isLedSet = 0; // ready to get new value
      Blynk.virtualWrite(triggersVpin,nTriggers);
      isTriggersSliderSet = 0;
      #ifdef batteryMonitor           // Update battery status/notify
        #ifdef debug
          Serial.print("Sending Volts: ");
          Serial.println(vbatt);
        #endif
        Blynk.virtualWrite(batteryVpin,vbatt);
        isVbattSet = 0;
        if(batteryLow) {
          #ifdef debug
            Serial.println("Sending low batt notification");
          #endif
          Blynk.notify(notifyLowBatt);
        }
      #endif
      if(armButton) {                //only send email/notify if armed
        #ifdef debug
          //Serial.println("Sending alarm email");
          Serial.println("Sending alarm notification");
        #endif
        //Blynk.email("test@msn.net", "From Blynk", stringMail);
        Blynk.notify(notifyPIR);
      }
      yield();
      #ifdef debug
        Serial.println("Syncing LED, Triggers, & Vbatt");
      #endif
      Blynk.syncVirtual(ledVpin, triggersVpin, batteryVpin);
      #ifdef debug
        Serial.println("state = 1");
      #endif
      state = 1;
      break;
    }

    case 1: { // verify data was uploaded correctly
      if(!isLedSet || !isTriggersSliderSet || !isVbattSet)  yield();
      // values all match, go ahead and disconnect
      else if(vbatt == vbattServerValue && ledServerValue == ledHigh && nTriggers == nTriggersServer) {
        #ifdef debug
          Serial.println("Sync successful");
          Serial.println("state = 2");
        #endif
        state = 2;
      }
      // synced values don't match... must have been an error
      else {
        #ifdef debug
          Serial.println("Error: state 1 sync mismatch");
        #endif
        char temp[] = {0};
        sprintf(temp, "%s: State1 Mismatch!", deviceName);
        Blynk.notify(temp);
        #ifdef debug
          Serial.println("state = 4");
        #endif
        state = 4;
      }
      // catch misc. failed syncing
      if(millis() > maxOnTime*1000) {
        #ifdef debug
          Serial.println("Error: state 1 timeout");
        #endif
        char temp[] = {0};
        sprintf(temp, "%s: State1 Timeout!", deviceName);
        Blynk.notify(temp);
        #ifdef debug
          Serial.println("state = 4");
        #endif
        state = 4;
      }
      break;
    }

    case 2: { // Disconnect wifi
      #ifdef debug
        Serial.print("Disconnecting Wifi: ");
      #endif
      Blynk.disconnect();
      WiFi.disconnect();
      wifi_station_disconnect();
      WiFi.mode(WIFI_OFF);
      WiFi.forceSleepBegin();
      yield(); //this delay is required for wifi to actually shut down
      #ifdef debug
        Serial.println("wifi off");
      #endif
      #ifdef debug
        Serial.println("state = 3");
      #endif
      state = 3;
    }

    case 3:{ // Wait for PIR to timeout
      if(millis() < maxOnTime*1000) {  //Not a false positive PIR yet...
        #ifdef debug
          Serial.println("!Testboard timeout!");
          Serial.println("state = 4");
          state = 4;
          return;
        #endif
        if(digitalRead(pirPin)) {  //reset timer if PIR triggers while waiting
          lastPirHigh = millis();
          return;
        }
        else if(millis()-lastPirHigh > pirTimeout*1000)  {  // PIR timeout, move on
          #ifdef debug
            Serial.println("Normal time out");
            Serial.println("state = 4");
          #endif
          state = 4;
          return;
        }
        yield(); //delay to prevent watchdog timer reset
      }
      else {  // We must have a PIR that is false triggering... move on
        #ifdef debug
          Serial.println("Error: state 2 false trigger");
          Serial.println("state = 4");
        #endif
        state = 4;
      }
      break;
    }

    case 4:{  // Connect wifi & send led off
      #ifdef debug
        Serial.println("Reconnecting WiFi");
      #endif
      WiFi.forceSleepWake();                // Turn wifi on
      WiFi.mode(WIFI_STA);
      wifi_station_set_hostname(hostName);
      wifi_station_connect();
      WiFi.begin(ssid,pass);                // Default Blynk server

      #ifdef debug
        Serial.println("Config Blynk...");
      #endif
      Blynk.config(auth0);        // Default Blynk server
      checkConnection();

      #ifdef debug
        Serial.println("Sending LED OFF");
      #endif
      Blynk.virtualWrite(ledVpin,ledLow);        // Turn off LED
      isLedSet = 0;
      yield();
      #ifdef debug
        Serial.println("Syncing LED");
      #endif
      Blynk.syncVirtual(ledVpin);
      #ifdef debug
        Serial.println("state = 5");
      #endif
      state = 5;
      break;
    }

    case 5: { // verify data was uploaded correctly
      if(!isLedSet) {
        yield();
      }
      // synced value matches, go ahead...
      else if(isLedSet && ledServerValue == ledLow) {
        #ifdef debug
          Serial.println("state = 6");
        #endif
        state = 6;
      }
      // synced values don't match... must have been an error
      else if(isLedSet && ledServerValue == !ledLow){
        #ifdef debug
          Serial.println("Error: state 4 sync mismatch");
        #endif
        char temp[] = {0};
        sprintf(temp, "%s: State4 Mismatch!", deviceName);
        Blynk.notify(temp);
        #ifdef debug
          Serial.println("state = 6");
        #endif
        state = 6;
      }
      // catch failed syncing
      if(millis() > maxOnTime*1000) {
        #ifdef debug
          Serial.println("Error: state 4 timeout");
        #endif
        char temp[] = {0};
        sprintf(temp, "%s: State4 Timeout!", deviceName);
        Blynk.notify(temp);
        #ifdef debug
          Serial.println("state = 6");
        #endif
        state = 6;
        return;
      }
      break;
    }

    case 6: {  // disconnect wifi & set 'enable low'
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
        Serial.print("Setting CH_PD: ");
      #endif
      digitalWrite(holdEnablePin,LOW);
      // If PIR is already low we should be off by now...
      #ifdef debug
        Serial.println("low");
        Serial.println("-=|  Goodbye! |=-");
      #endif
      #ifdef debug
        Serial.println("state = 7");
      #endif
      state = 7;
    }

    case 7:{ //Do nothing until PIR disables us
      #ifdef testBoard
        ESP.reset();
      #endif
      yield();  //delay to prevent watchdog timer reset
      if(millis() > maxOnTime*1000) {
        digitalWrite(holdEnablePin,LOW);
        ESP.deepSleep(0);  //stop PIR errors
      }
      break;
    }

    default:{ //This should never happen
      #ifdef debug
        Serial.println("Error: INVALID STATE!!!");
      #endif
      digitalWrite(holdEnablePin,LOW);
      ESP.deepSleep(0);
      yield();
      break;
    }
  }
  return;
}
//////////////////////////////////////////////////////////
void setup()  {
  //Make sure we stay on until we're ready to sleep
  pinMode(holdEnablePin, OUTPUT);
  digitalWrite(holdEnablePin,HIGH);
  #ifdef debug
    Serial.begin(115200);
    Serial.println();
    Serial.println("Debug...");
    Serial.println("Holding enable pin high");
  #endif

  #ifdef batteryMonitor    // Read battery voltage
    #ifdef debug
      Serial.println("Reading battery volts");
    #endif
    vbatt = analogRead(A0);
    vbatt = vbatt * (vbattRatio / 1024.0);
    #ifdef testBoard
      vbatt = 4;
    #endif
    if(vbatt < vbattCrit) { // vbatt critical, shutdown immediately
      #ifdef debug
        Serial.println("Critical Battery Shut Down...");
      #endif
      digitalWrite(holdEnablePin,LOW);
      while(1<2) {          // when PIR goes low we will be off
        yield();
      }
    }
    if(vbatt <= vbattLow) batteryLow = 1;
  #endif

  pinMode(pirPin, INPUT);
  lastPirHigh = millis();   //We've already seen PIR high, mark it now

  // Combine , except ones that need an IP
  sprintf(hostName, "%s%s", hostNameX, deviceName);
  sprintf(notifyLowBatt, "%s%s", deviceName, notifyLowBattX);
  sprintf(notifyCritBatt, "%s%s", deviceName, notifyCritBattX);
  sprintf(notifyOTAtimeout, "%s%s", deviceName, notifyOTAtimeoutX);
  sprintf(notifyPIR, "%s%s", deviceName, notifyPIRX);

  #ifdef debug
    Serial.println("Connect Wifi...");
  #endif
  WiFi.mode(WIFI_STA);       // Configure & connect to wifi
  //WiFi.setOutputPower(10); // Use this if you want to play
  wifi_station_set_hostname(hostName);
  wifi_station_connect();
  WiFi.begin(ssid,pass);
  #ifdef debug
    Serial.println("Config Blynk...");
  #endif
  Blynk.config(auth0);        // Default Blynk server
  checkConnection();          // Connect to Blynk

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

  // Sync all blynk vpins
  #ifdef debug
    Serial.println("Syncing OTA, Arm, & Triggers");
  #endif
  //Blynk.syncVirtualAll();
  Blynk.syncVirtual(firmwareVpin, armButtonVpin, triggersVpin);
  BLYNK_LOG("Sync OTA, Arm, & Triggers");
}

//////////////////////////////////////////////////////////////////////////
void loop() {
  if(state==2 || state==3 || state==6 || state==7) {}  // Don't need this while wifi is off
  else Blynk.run();

  // Catch any misc. errors that might kill the battery
  if(millis() > maxOnTime*1000) {
    #ifdef debug
      Serial.println("Error: loop() timeout");
    #endif
    digitalWrite(holdEnablePin,LOW);
    ESP.deepSleep(0);  //stop PIR errors
  }

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

  // FW button off (or batt too low to OTA), do PIR routine
  if(!fwButton && isFWbuttonSet && isArmButtonSet && isTriggersSliderSet) {
    doStuff();
  }
}
