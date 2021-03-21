#include "WiFi.h"

// Switch to LittleFS if needed
#define USE_LittleFS

#include <FS.h>
#ifdef USE_LittleFS
    #define SPIFFS LITTLEFS
    #include <LITTLEFS.h> 
    //#define CONFIG_LITTLEFS_SPIFFS_COMPAT 1
#else
    #include <SPIFFS.h>
#endif

#ifndef sysInit_h
#define sysInit_h

bool FlashFSready = false;

void taskWiFi(void* parameter) {
    //const char* SSID = "Alter Eggstutz";
    //const char* PSK  = "***";

    // Connect to WiFi
    Serial.println(F("[T] WiFi: Begin."));
    WiFi.mode(WIFI_STA);
    WiFi.begin();
    Serial.println(F("[T] WiFi: Trying to connect..."));

    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(1000);
        Serial.print(F("[T] WiFi: Not connected yet!"));
    }
    Serial.println(F(""));
    
    Serial.println(F("[T] WiFi: Connected successfully."));
    Serial.print("[T] WiFi: IP: ");
        Serial.println(WiFi.localIP());

    Serial.println("[i] WiFi: Destroying task...");  
    vTaskDelete(NULL);
}

void taskFSMount(void* parameter) {

    Serial.println(F("[T] FS: Mounting..."));

	if (LITTLEFS.begin()) {
		Serial.println("[T] FS: Mounted.");
        FlashFSready = true;
	} else {
		Serial.println("[X] FS: Mount failure.");
		Serial.println("[X] FS: Rebooting ESP.");
        ESP.restart();
	}

    // List contents
    File root = LITTLEFS.open("/");
    File file = root.openNextFile();

    Serial.println("[i] FS: Listing files...");    
    while(file) {
      Serial.print("[>] FS: File found: ");
      Serial.println(file.name());
 
      file = root.openNextFile();
    }

    Serial.println("[i] FS: Destroying task...");  
    vTaskDelete(NULL);
}

#endif