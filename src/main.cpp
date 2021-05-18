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
TaskHandle_t TaskHUE_Handle;

// https://i0.wp.com/randomnerdtutorials.com/wp-content/uploads/2018/08/esp32-pinout-chip-ESP-WROOM-32.png
// Factory reset button GPIO pin
#define FACT_RST 17

// Task to monitor if a factory reset has been triggered.
void taskfactoryResetWDT(void* parameter) {
    unsigned long activeMillis;
    unsigned long currMillis;

    pinMode(FACT_RST, INPUT_PULLDOWN);

    for (;;) {
        vTaskDelete(NULL);

        // Check if factory reset button is pressed using some millis() math
        currMillis = millis();
        if (digitalRead(FACT_RST)) activeMillis = currMillis;

        // Trigger factory reset if pin is HIGH for at least 5 seconds
        if ((currMillis - activeMillis >= 5000) || EnforceFactoryReset) {
            // Perform factory reset
            Serial.println("[!!!] FACTORY RESET INITIATED [!!!]");

            while (!FlashFSready) {
                Serial.println("[!] Reset: Awaiting FS mount...");
                vTaskDelay(500);
            }

            // Destroy all tasks
            Serial.println("[!] Reset: Destroying perpetual tasks...");
            vTaskDelete(TaskRTC_Handle);
            vTaskDelete(TaskNixie_Handle);
            vTaskDelete(TaskHUE_Handle);

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
    }

    vTaskDelay(100);
}

//  ---------------------
//  MAIN
//  ---------------------

void setup() {
    Serial.begin(115200);

    // Initial scan for WiFi networks.
    // This will prevent getting empty results on WiFi Scans in the WebGUI
    WiFi.scanNetworks();

    // Shift registers
    pinMode(DS_PIN, OUTPUT);
    pinMode(SH_CP, OUTPUT);
    pinMode(ST_CP, OUTPUT);

    // Clear nixies
    digitalWrite(ST_CP, LOW);
    shiftOut(DS_PIN, SH_CP, MSBFIRST, 0b11111111);
    shiftOut(DS_PIN, SH_CP, MSBFIRST, 0b11111111);
    digitalWrite(ST_CP, HIGH);

    // FreeRTOS tasks
    Serial.println(F("[i] Starting tasks..."));

    // One-time / setup tasks
    xTaskCreate(taskWiFi, "WiFi initiator", 3500, NULL, 1, NULL);
    xTaskCreate(taskFSMount, "FS Mount", 2500, NULL, 1, NULL);
    xTaskCreate(taskSetupRTC, "RTC Setup", 3500, NULL, 1, NULL);
    xTaskCreate(taskSetupHUE, "HUE Setup", 3500, NULL, 1, NULL);
    xTaskCreate(taskSetupWebserver, "Webserver start", 5500, NULL, 1, NULL);

    // Perpetual tasks
    xTaskCreate(taskUpdateRTC, "RTC Sync", 3500, NULL, 1, &TaskRTC_Handle);
    xTaskCreate(taskUpdateNixie, "Nixie updater", 5500, NULL, 2, &TaskNixie_Handle);
    xTaskCreate(taskMonitorHUE, "FRST WDT", 5500, NULL, 3, &TaskHUE_Handle);
    xTaskCreate(taskfactoryResetWDT, "HUE monitor", 2500, NULL, 1, NULL);
}

void loop() {

    ws.cleanupClients();
}