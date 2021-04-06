/*
    ESP32 Nixie Clock - System initialization
    (c) V. Klopfenstein, 2021

    All tasks/functions in here are related to system intialization/preparation.
    It...
     - Connects WiFi.
     - Mounts the onboard flash FS.
    
    The results produced by these actions are relied on by all later tasks.
*/

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
bool WiFiReady = false;

//  ---------------------
//  FUNCTIONS
//  ---------------------

void listFilesInDir(File dir, int numTabs) {
    while (true) {
    
        File entry =  dir.openNextFile();
        if (! entry) {
            // no more files in the folder
            break;
        }
        for (uint8_t i = 0; i < numTabs; i++) {
            Serial.print('\t');
        }
        Serial.print(entry.name());

        if (entry.isDirectory()) {
            Serial.println("/");
            listFilesInDir(entry, numTabs + 1);
        } else {
            // display size for file, nothing for directory
            Serial.print("\t\t");
            Serial.println(entry.size(), DEC);
        }
        entry.close();
    }
}

//  ---------------------
//  TASKS
//  ---------------------

void taskWiFi(void* parameter) {
    const char* SSID = "Alter Eggstutz";
    const char* PSK  = "Fischer1";

    // Connect to WiFi
    Serial.println(F("[T] WiFi: Begin."));
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PSK);
    Serial.println(F("[T] WiFi: Trying to connect..."));

    int i = 0;
    while (WiFi.status() != WL_CONNECTED) {
        if (i > 15) {
            Serial.println("[X] WiFi: Retry timeout.");
            vTaskDelete(NULL);
        }

        vTaskDelay(1000);

        Serial.println("[T] WiFi: Not connected yet!");
        i++;
    }

    Serial.println(F("[T] WiFi: Connected successfully."));

    WiFiReady = true;
    Serial.print("[T] WiFi: IP: ");
        Serial.println(WiFi.localIP());

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

    // Get all information of SPIFFS
    // Taken from: https://diyprojects.io/esp32-get-started-spiff-library-read-write-modify-files/
    unsigned int totalBytes = SPIFFS.totalBytes();
    unsigned int usedBytes = SPIFFS.usedBytes();

    Serial.println("===== File system info =====");

    Serial.print("Total space:      ");
    Serial.print(totalBytes);
    Serial.println(" bytes");

    Serial.print("Total space used: ");
    Serial.print(usedBytes);
    Serial.println(" bytes");

    Serial.println();

    // Open dir folder
    File dir = LITTLEFS.open("/");
    
    // List file at root
    listFilesInDir(dir, 1);

    vTaskDelete(NULL);
}

#endif