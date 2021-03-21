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
    xTaskCreate(
        taskWiFi,                   // Function that should be called
        "WiFi initiator",           // Name of the task (for debugging)
        2048,                       // Stack size (bytes)
        NULL,                       // Parameter to pass
        1,                          // Task priority
        NULL                        // Task handle
    );
    
    xTaskCreate(taskFSMount, "FS Mount", 2000, NULL, 1, NULL);
    xTaskCreate(taskSetupRTC, "FS Mount", 2000, NULL, 1, NULL);

    Serial.println(F("[i] Done with setup()."));
}

// Stays empty as we have built an RTOS infrastructure
void loop() {}