#include <Arduino.h>
#include <tasks.h>
#include <WS2812FX.h>

#define LED_COUNT 4
#define LED_PIN 17

WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);
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

  // WS LED init
  ws2812fx.init();
  ws2812fx.setBrightness(100);
  ws2812fx.setSpeed(200);
  ws2812fx.setMode(FX_MODE_RAINBOW_CYCLE);
  ws2812fx.start();

  delay(200);
  ws2812fx.service();
  
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