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
#include <WiFiClientSecure.h>

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
bool APmode = true;

struct netConfigStruct {
    char Mode[7];
    char SSID[64];
    char PSK[64];
    char IP[16];
    char Netmask[16];
    char Gateway[16];
    char DNS[16];
};

struct netConfigStruct netConfig;

//  ---------------------
//  TASKS
//  ---------------------

void taskWiFi(void* parameter) {
    const char* AP_SSID = "ESP32 - Nixie clock";
    const char* AP_PSK  = "NixieClock2021";

    // Check for net config file
    Serial.println("[T] WiFi: Looking for config...");

    while (!FlashFSready) { vTaskDelay(1000); }
    if (!(LITTLEFS.exists("/config/netConfig.json"))) {
        Serial.println(F("[T] WiFi: No config found."));
        
        if (!LITTLEFS.exists("/config"))
            LITTLEFS.mkdir("/config");

        File netConfigF = LITTLEFS.open(F("/config/netConfig.json"), "w");

        // Construct JSON
        StaticJsonDocument<250> cfgNet;

        cfgNet["Mode"] = "AP";
        cfgNet["SSID"] = AP_SSID;
        cfgNet["PSK"] = AP_PSK;
        cfgNet["IP"] = 0;
        cfgNet["Netmask"] = 0;
        cfgNet["Gateway"] = 0;
        cfgNet["DNS"] = 0;

        // Write netConfig.cfg
        if (!(serializeJson(cfgNet, netConfigF)))
            Serial.println(F("[X] WiFi: Config write failure."));

        netConfigF.close();

        // Set time on RTC
        Serial.println(F("[>] WiFi: Config created."));
    }

    // Parse net config file
    // Read file
    File netConfigF = LITTLEFS.open(F("/config/netConfig.json"), "r");

    // Parse JSON
    StaticJsonDocument<250> cfgNet;
    DeserializationError error = deserializeJson(cfgNet, netConfigF);
    if (error)
        Serial.println(F("[X] WiFi: Could not deserialize JSON."));
        
    strlcpy(netConfig.Mode, cfgNet["Mode"], sizeof(netConfig.Mode));
    strlcpy(netConfig.SSID, cfgNet["SSID"], sizeof(netConfig.SSID));
    strlcpy(netConfig.PSK, cfgNet["PSK"], sizeof(netConfig.PSK));
    
    netConfigF.close();

    Serial.println(netConfig.Mode);

    // Start WiFi AP or client based on config file params
    if (strcmp(netConfig.Mode, "AP") == 0) {
        Serial.println("[i] WiFi: Starting AP.");

        WiFi.softAP(netConfig.SSID, netConfig.PSK);
        Serial.print("[i] WiFi: AP IP address: ");
            Serial.println(WiFi.softAPIP());

        WiFiReady = true;

    } else {
        bool APmode = false;
        Serial.println("[i] WiFi: Starting client.");
        Serial.print("[i] WiFi: Trying to connect to: ");
            Serial.println(netConfig.SSID);

        int i = 0;
        WiFi.mode(WIFI_STA);
        WiFi.begin(netConfig.SSID, netConfig.PSK);

        while (WiFi.status() != WL_CONNECTED) {
            if (i > 15) {
                Serial.println("[X] WiFi: Connection timeout.");
                vTaskDelete(NULL);
            }

            vTaskDelay(1000);

            Serial.println("[T] WiFi: Connecting...");
            i++;
        }

        Serial.println(F("[T] WiFi: Connected successfully."));

        WiFiReady = true;
        Serial.print("[T] WiFi: IP: ");
            Serial.println(WiFi.localIP());
    }

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
        //ESP.restart();
	}

    // Get all information of SPIFFS
    // Taken from: https://diyprojects.io/esp32-get-started-spiff-library-read-write-modify-files/
    Serial.println("===== File system info =====");
    Serial.print("Total space:      ");
    Serial.print(LITTLEFS.totalBytes());
    Serial.println(" bytes");

    Serial.print("Total space used: ");
    Serial.print(LITTLEFS.usedBytes());
    Serial.println(" bytes");

    Serial.println();

    // Open dir folder
    File dir = LITTLEFS.open("/");

    vTaskDelete(NULL);
}

#endif