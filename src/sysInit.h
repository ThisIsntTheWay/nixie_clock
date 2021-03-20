#include "WiFi.h"
#include <SPIFFS.h>

#ifndef sysInit
#define sysInit

void taskWiFi(void* parameter) {
    const char* SSID = "Alter Eggstutz";
    const char* PSK  = "Fischer1";

    // Connect to WiFi
    Serial.println(F("[T] WiFi: Begin."));
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PSK);
    Serial.print(F("[T] WiFi: Trying to connect"));

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(500);
    }
    Serial.println(F(" "));

    Serial.println(F("[T] WiFi: Connected successfully."));
    Serial.print("[T] WiFi: IP: ");
        Serial.println(WiFi.localIP());

    vTaskDelete(NULL);
}

void taskFSMount(void* parameter) {

    Serial.println(F("[T] SPFFS: Mounting..."));

	if (SPIFFS.begin()) {
		Serial.println("[T] SPFFS: Mounted.");
	} else {
		Serial.println("[X] SPFFS: Mount failure.");
	}

    // List contents
    File root = SPIFFS.open("/");
    File file = root.openNextFile();

    Serial.println("[i] SPFFS: Listing files...");    
    while(file) {
      Serial.print("[>] SPFFS: File found: ");
      Serial.println(file.name());
 
      file = root.openNextFile();
    }   

    vTaskDelete(NULL);
}

#endif