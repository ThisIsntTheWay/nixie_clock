/*
    ESP32 Nixie Clock - Entrypoint
    (c) V. Klopfenstein, 2021

    This is the entrypoint of the whole program.
    The purpose of this file is to spawn all tasks built with FreeRTOS.
*/

#include "Arduino.h"

#include <system/tasks.h>
#include <utils/network.h>

//  ---------------------
//  MAIN
//  ---------------------

void setup() {
    Serial.begin(115200);

    Serial.println(F("[i] System booting up..."));
    Serial.printf("[i] Previous reset reason: Core 0: %d, Core 1: %d\n", rtc_get_reset_reason(0), rtc_get_reset_reason(1));

    // Initial scan for WiFi networks.
    // This will prevent getting empty results on WiFi Scans in the WebGUI
    Serial.println(F("[i] Conducting an initial WiFi scan..."));
    WiFi.scanNetworks();
    Serial.println(F("[i] Initial WiFi scan complete."));

    // FreeRTOS tasks
    Serial.println(F("[i] Starting tasks..."));

    // One-time / setup tasks
    xTaskCreate(taskWiFi, "WiFi initiator", 3500, NULL, 1, NULL);
    xTaskCreate(taskFSMount, "FS Mount", 2500, NULL, 1, NULL);
    xTaskCreate(taskSetupRTC, "RTC Setup", 3500, NULL, 1, NULL);
    xTaskCreate(taskSetupNixie, "RTC Setup", 2500, NULL, 1, NULL);
    xTaskCreate(taskSetupHUE, "HUE Setup", 3500, NULL, 1, NULL);
    xTaskCreate(taskSetupWebserver, "Webserver start", 5500, NULL, 1, NULL);

    // Perpetual tasks
    xTaskCreate(taskUpdateRTC, "RTC Sync", 5500, NULL, 1, &TaskRTC_Handle);
    xTaskCreate(taskUpdateNixie, "Nixie updater", 6000, NULL, 2, &TaskNixie_Handle);
    xTaskCreate(taskUpdateNixieBrightness, "Nixie brightness", 3000, NULL, 2, &TaskBright_Handle);
    xTaskCreate(taskMonitorHUE, "HUE monitor", 6000, NULL, 3, &TaskHUE_Handle);
    xTaskCreate(taskfactoryResetWDT, "Master reset", 2500, NULL, 1, NULL);
}

void loop() {
    ws.cleanupClients();
}