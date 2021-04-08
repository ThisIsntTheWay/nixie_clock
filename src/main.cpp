/*
    ESP32 Nixie Clock - Entrypoint
    (c) V. Klopfenstein, 2021

    This is the entrypoint of the whole program.
    The purpose of this file is to spawn all tasks built with FreeRTOS.
*/

#include "Arduino.h"

// Custom headers
#include <system/webServer.h>
#include <system/rtc.h>
#include <system/nixie.h>
#include <utils/network.h>

TaskHandle_t TaskRTC_Handle;
TaskHandle_t TaskNixie_Handle;

// https://i0.wp.com/randomnerdtutorials.com/wp-content/uploads/2018/08/esp32-pinout-chip-ESP-WROOM-32.png
// Factory reset button GPIO pin
#define FACT_RST 17

// Task to monitor if a factory reset has been triggered.
void taskfactoryResetWDT(void* parameter) {
    unsigned long lastMillis = millis();
    unsigned long currMillis;

    pinMode(FACT_RST, INPUT_PULLDOWN);

    for (;;) {
        // Check if factory reset button is pressed using some millis() math
        currMillis = millis();
        if (digitalRead(FACT_RST)) {
            // Trigger factory reset if pin is HIGH for at least 5 seconds
            if ((currMillis > lastMillis + 5000) || EnforceFactoryReset) {
                // Perform factory reset
                Serial.println("[!!!] FACTORY RESET INITIATED [!!!]");

                // Destroy all tasks
                Serial.println("[!] Reset: Destroying perpetual tasks...");
                vTaskDelete(TaskRTC_Handle);
                vTaskDelete(TaskNixie_Handle);
                server.end();

                // Destroy config files
                Serial.println("[!] Reset: Nuking /config dir...");
                File configDir = LITTLEFS.open("/config");
                File configFile = configDir.openNextFile();

                while (configFile) {
                    Serial.print(" > Destroying: ");
                        Serial.println(configFile.name());
                    
                    LITTLEFS.remove(configFile.name());

                    configFile = configDir.openNextFile();
                }

                // At last, restart ESP
                Serial.println("[!] Reset: Restarting ESP...");
                ESP.restart();
            }
        } else { lastMillis = millis(); }

        vTaskDelay(100);
    }
}

//  ---------------------
//  MAIN
//  ---------------------

void setup() {
    Serial.begin(115200);

    // FreeRTOS tasks
    Serial.println(F("[i] Spawning tasks..."));

    // One-time / setup tasks
        xTaskCreate(taskWiFi, "WiFi initiator", 3500, NULL, 1, NULL);
        xTaskCreate(taskFSMount, "FS Mount", 2500, NULL, 1, NULL);
        xTaskCreate(taskSetupRTC, "RTC Setup", 3500, NULL, 1, NULL);
        xTaskCreate(taskSetupHUE, "HUE Setup", 3500, NULL, 1, NULL);
        xTaskCreate(taskSetupWebserver, "Webserser start", 5500, NULL, 1, NULL);

    // Perpetual tasks
        xTaskCreate(taskUpdateRTC, "RTC Sync", 3500, NULL, 1, &TaskRTC_Handle);
        xTaskCreate(taskUpdateNixie, "Nixie updater", 5500, NULL, 1, &TaskNixie_Handle);
        xTaskCreate(taskfactoryResetWDT, "FRST WDT", 2000, NULL, 1, NULL);

    Serial.println(F("[i] Done with setup()."));
    Serial.print("[i] Free heap: ");
        Serial.println(ESP.getFreeHeap());
}

void loop() {}