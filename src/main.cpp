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
#include "web.h"
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

// Declare server object and ini on TCP/80
AsyncWebServer server(80);

// Definitions
#define srLatch = 22;
#define srClock = 23;
#define srData = 21;

// RTC / time
#define RTCaddr = 0x76;
const char* ntpServer = "ch.pool.ntp.org";
const long  gmtOffset_sec = 3600;       // Timezone
const int   daylightOffset_sec = 3600;  // Daylight savings

// WiFi
const char* wifiSSID = "TBD";
const char* wifiPSK  = "TBD";

byte BCDtable[10] = {0000, 0001, 0010,
                    0011, 0100, 0101,
                    0111, 1000, 1001,
                    1010};

// =======================
// === FUNCTONS
// =======================

// Init WiFi connection
void taskInitConn() {
  WiFi.begin(wifiSSID, wifiPSK);
  
  Serial.print("Attempting connection to: ");
    Serial.println(wifiSSID);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println(" ");
  Serial.println("Connection successfull.");

  vTaskDelete(NULL);
}

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

// RTC manipulation
void taskSetRTC(int manType) {
  // manType refers to manipulation type
  // 0 = Set via serial, 1 = Set via NTP
  
  if (manType == 1) {
    // Set RTC time using NTP
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    
    // Verify that time has been obtained.
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
      Serial.println("[X] Failed to obtain time.");
      Serial.print("  > NTP server is: ");
        Serial.println(ntpServer):
	    
      return;
    } else {
      Serial.println("[i] NTP query successfull.");
      Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
	    
	    //ToDo: Implement setting time on RTC
	    
      return;
    }
  } else {
    
  }
  
}

void taskGetRTC() { 
}

void writeToSR(int bcd) {
	// Shift out BCD val to shift register using most significant bit first
	shiftOut(srData, srClock, MSBFIRST, bcd);
}

// =======================
// === MAIN
// =======================

void setup() {
	Serial.begin(115200);
	// Start WiFi connection

	xTaskCreate(taskInitConn, "Initiate WiFi", 1000, NULL, 5, NULL);  
	  
	// Start task for web server handling
	xTaskCreate(taskWebServer, "Start HTTP handler", 1000, NULL, 5, NULL);
	  
	// Verify that RTC has been set already
	int RTCstate = EEPROM.read(0);
	if (RTCstate == 0) {
		//RTC NOT set
		Serial.println("[i] RTC appears to have not been set yet.");
		xTaskCreate(taskSetRTC, "Set RTC via NTP", 1000, NTP, 4, NULL);
		xTaskCreate(taskInitConn, "Initiate WiFi", 1000, NULL, 5, NULL);
		  
		  // Verify that RTC has been set already
		int RTCstate = EEPROM.read(0);
		  
		if (RTCstate == 0) {
			//RTC NOT set
			Serial.println("[i] RTC appears to have not been set yet.");
			
			xTaskCreate(taskSetRTC, "Set RTC via NTP", 1000, NTP, 4, NULL);
		}
	}
}

void loop() {
}
