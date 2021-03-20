#include "Arduino.h"
#include <EEPROM.h>
#include <webServer.h>
#include <sysInit.h>

// Time settings
char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;


//  ---------------------
//  MAIN
//  ---------------------

void setup() {

    Serial.begin(115200);
    Serial.println(F("[i] START BOOTUP"));



    // Detect if RTC has been set for the first time already
    // If not, set using NTP
    Serial.print(F("[i] Trying to read EEPROM: "));
    Serial.print(EEPROM.read(0));
    Serial.println(EEPROM.read(1));
    /*if (EEPROM.read(0) != 1) {
        Serial.println(F(" > [!] Wrote to EEPROM."));

        EEPROM.write(0, 1);
    }*/
    Serial.println(F("[i] configTime set."));
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    //struct tm timeinfo;

    // Start webserver
    webServerRequestHandler();

    // FreeRTOS task creation
    Serial.println(F("[i] Spawning tasks..."));
    xTaskCreate(
        taskWiFi,      // Function that should be called
        "WiFi initiator",           // Name of the task (for debugging)
        2048,                       // Stack size (bytes)
        NULL,                       // Parameter to pass
        1,                          // Task priority
        NULL                        // Task handle
    );
    
    xTaskCreate(
        taskFSMount,
        "FS Mount",
        2000,
        NULL,
        1,
        NULL
    );
    Serial.println(F("[i] Done with setup"));
}

// Stays empty as we have built an RTOS infrastructure
void loop() {}