#include "Arduino.h"
#include "WiFi.h"
#include <EEPROM.h>
#include <webServer.h>

#define SSID    "Alter Eggstutz"
#define PSK     "Fischer1"

// Time settings
char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;


//  ---------------------
//  MAIN
//  ---------------------

void setup() {

    Serial.begin(115200);
    Serial.println(F("START BOOTUP"));

    // Connect to WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PSK);
    Serial.println(F("Attempting to establish a  WiFi connection!"));

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(500);
    }

    Serial.println(F("Connected successfully."));
    Serial.print("IP: ");
        Serial.println(WiFi.localIP());

    // Detect if RTC has been set for the first time already
    // If not, set using NTP
    if (EEPROM.read(0) != 1) {
        

        EEPROM.write(0, 1);
    }
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    struct tm timeinfo;

    // FreeRTOS task creation
    xTaskCreate(
        webServerRequestHandler,      // Function that should be called
        "Web request handler",        // Name of the task (for debugging)
        1000,                       // Stack size (bytes)
        NULL,                       // Parameter to pass
        1,                          // Task priority
        NULL                        // Task handle
    );
    
}

// Stays empty as we have built an RTOS infrastructure
void loop() {}