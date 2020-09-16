// ESP32 based nixie clock.
// REQUIRES: 8-Bit SIPO shift registers and BCD to DEC HV drivers.
// 
// (c) V. Klopfenstein, September 2020

// Libraries
#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <EEPROM.h>
#include <time.h>
<<<<<<< Updated upstream
#include "web.h"
=======
>>>>>>> Stashed changes

// Definitions
#define srLatch = 2;
#define srClock = 2;
#define srData = 2;

// RTC / time
#define RTCaddr = 0x76;
const char* ntpServer = "ch.pool.ntp.org";
const long  gmtOffset_sec = 3600;       // Timezone
const int   daylightOffset_sec = 3600;  // Daylight savings

<<<<<<< Updated upstream
=======

>>>>>>> Stashed changes
// WiFi
const char* wifiSSID = "TBD";
const char* wifiPSK  = "TBD";

byte BCDtable[10] = {0000, 0001, 0010,
                    0011, 0100, 0101,
                    0111, 1000, 1001,
                    1010}

// =======================
// === FUNCTONS
// =======================

// Init WiFi connection
void taskInitConn() {
  WiFi.begin(wifiSSID, wifiPSK);
  
  Serial.println("Attempting connection to: " + wifiSSID);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println(" ");
  Serial.println("Connection successfull.");

  vTaskDelete(NULL);
}

<<<<<<< Updated upstream
void taskWebServer() {
  for (;;) {
     // Handle HTTP_GET
     server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
     request->send_P(200, "text/html", htmlRoot);
     });
     server.on("/setRTC", HTTP_GET, [](AsyncWebServerRequest *request){
     request->send_P(200, "text/html", htmlRTCControl);
     });
     server.on("/setTubes", HTTP_GET, [](AsyncWebServerRequest *request){
     request->send_P(200, "text/html", htmlTubeControl);
     });
     server.on("/setPower", HTTP_GET, [](AsyncWebServerRequest *request){
     request->send_P(200, "text/html", htmlPowerControl);
     });

     vTaskDelay(500);
  } 
}

=======
>>>>>>> Stashed changes
// RTC manipulation
void taskSetRTC(int manType) {
  // manType refers to manipulation type
  // 0 = Set via serial, 1 = Set via serial
  
  if (manType == 1) {
    // Set RTC time using NTP
    
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    
    // Verify that time has been obtained.
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
      Serial.println("[X] Failed to obtain time.");
      Serial.println("  > NTP server is: " + ntpServer);
      return;
    } else {
      Serial.println("[i] NTP query successfull.");
      Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
      return;
    }
    
    
  } else {
    
  }
  
}

void taskGetRTC() {
  
}

// =======================
// === MAIN
// =======================

void setup() {
  Serial.begin(115200);
  
  // Start WiFi connection
<<<<<<< Updated upstream
  xTaskCreate(taskInitConn, "Initiate WiFi", 1000, NULL, 5, NULL);  
  
  // Start task for web server handling
  xTaskCreate(taskWebServer, "Start HTTP handler", 1000, NULL, 5, NULL);
  
  // Verify that RTC has been set already
  int RTCstate = EEPROM.read(0);
  if (RTCstate == 0) {
    //RTC NOT set
    Serial.println("[i] RTC appears to have not been set yet.");
    xTaskCreate(taskSetRTC, "Set RTC via NTP", 1000, NTP, 4, NULL);
=======
  xTaskCreate(taskInitConn, "Initiate WiFi", 1000, NULL, 5, NULL);
  
  // Verify that RTC has been set already
  int RTCstate = EEPROM.read(0);
  
  if (RTCstate == 0) {
    //RTC NOT set
    Serial.println("[i] RTC appears to have not been set yet.");
    
  xTaskCreate(taskSetRTC, "Set RTC via NTP", 1000, NTP, 4, NULL);
>>>>>>> Stashed changes
  }
}

void loop() {
 
<<<<<<< Updated upstream
}
=======
}
>>>>>>> Stashed changes
