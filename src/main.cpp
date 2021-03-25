/*
    ESP32 Nixie Clock - Entrypoint
    (c) V. Klopfenstein, 2021

    This is the entrypoint of the whole program.
    The purpose of this file is to spawn all tasks built with FreeRTOS.
*/

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

    // FreeRTOS tasks
    Serial.println(F("[i] Spawning tasks..."));
    xTaskCreate(taskWiFi, "WiFi initiator", 2048, NULL, 1, NULL);
    xTaskCreate(taskFSMount, "FS Mount", 2500, NULL, 1, NULL);
    xTaskCreate(taskSetupRTC, "RTC Setup", 3500, NULL, 1, NULL);
    xTaskCreate(taskSetupHUE, "HUE Setup", 3500, NULL, 1, NULL);

    xTaskCreate(taskUpdateRTC, "RTC Sync", 3500, NULL, 1, NULL);

    webServerStartup();

    Serial.println(F("[i] Done with setup()."));
    Serial.print("[i] Free heap: ");
        Serial.println(ESP.getFreeHeap());
}

// Empty loop thanks to RTOS
void loop() {}