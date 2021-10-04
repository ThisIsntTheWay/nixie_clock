#include <Arduino.h>
#include <tasks.h>
Configurator c;

void setup() {
  Serial.begin(115200);
  
  // Create build info
  c.buildInfo = "v" + String(BUILD_VERSION) + " (" + BUILD_REL_TYPE + ") - " + __DATE__ + ", " + __TIME__;

  /* // Write build info
  File buildInfoFile = LITTLEFS.open(F("buildinfo"), "w");
  buildInfoFile.println(buildInfo);
  buildInfoFile.close(); */

  Serial.print(F("[i] System: Build: "));
    Serial.println(c.buildInfo);

  // Conduct an initial WiFi Scan
  WiFi.scanNetworks(true);
  WiFi.scanDelete();

  delay(200);
  
  #ifdef DEBUG
    Serial.println("[i] System: DEBUG flag is set.");
  #endif
  #ifdef DEBUG_VERBOSE
    Serial.println("[i] System: DEBUG_VERBOSE flag is set.");
  #endif
  #ifdef FULL_TUBESET
    Serial.println("[i] System: FULL_TUBESET flag is set.");
  #endif

  // Setup tasks
  xTaskCreate(taskSysInit, "Nixie setup", 3500, NULL, 1, NULL);

  xTaskCreate(taskSetupNetwork, "Network setup", 3500, NULL, 1, NULL);
  xTaskCreate(taskSetupRTC, "RTC setup", 2500, NULL, 1, NULL);
  xTaskCreate(taskSetupNixies, "Nixie setup", 3500, NULL, 1, NULL);
  xTaskCreate(taskSetupWebserver, "Webserver setup", 5500, NULL, 1, NULL);
  
  // Perp. tasks
  xTaskCreate(taskMonitorStatus, "Status monitor", 3500, NULL, 1, NULL);
  xTaskCreate(taskUpdateNixies, "Nixie updater", 4500, NULL, 1, c.task_perp_nix);
  xTaskCreate(taskUpdateRTC, "RTC updater", 4500, NULL, 1, c.task_perp_rtc);
  xTaskCreate(taskUpdateCaches, "Cache updater", 5500, NULL, 1, c.task_perp_cac);
}

void loop() {}