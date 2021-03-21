#include "Arduino.h"

// Custom headers
#include <webServer.h>
#include <sysInit.h>
#include <rtc.h>

//  ---------------------
//  MAIN
//  ---------------------

void setup() {
    Serial.begin(115200);

    Serial.println(F("[i] START BOOTUP"));

    // FreeRTOS task creation
    Serial.println(F("[i] Spawning tasks..."));
    xTaskCreate(taskWiFi, "WiFi initiator", 2048, NULL, 1, NULL);
    xTaskCreate(taskFSMount, "FS Mount", 2500, NULL, 1, NULL);
    xTaskCreate(taskSetupRTC, "RTC Setup", 2500, NULL, 1, NULL);

    webServerStartup();

    Serial.println(F("[i] Done with setup()."));
}

// Empty loop thanks to RTOS
void loop() {}