/*
    ESP32 Nixie Clock - System initialization
    (c) V. Klopfenstein, 2021

    All tasks/functions in here are related to system intialization/preparation.
    It...
     - Connects WiFi.
     - Mounts the onboard flash FS.
    
    The results produced by these actions are relied on by all other tasks.
*/

#include <ArduinoJson.h>
#include "WiFi.h"
#include <WiFiClientSecure.h>
#include <ESPmDNS.h>

#ifndef network_h
#define network_h

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

bool FlashFSready = false;
bool WiFiReady = false;
bool APmode = true;
bool APisFallback = false;

struct netConfigStruct {
    char Mode[7];
    char AP_SSID[64];
    char AP_PSK[64];
    char WiFi_SSID[64];
    char WiFi_PSK[64];
    char IP[16];
    char Netmask[16];
    char Gateway[16];
    char DNS[16];
};

struct netConfigStruct netConfig;

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
      // display zise for file, nothing for directory
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}


String parseNetConfig(int mode) {
    // Read file
    File netConfigF = LITTLEFS.open(F("/config/netConfig.json"), "r");

    // Parse JSON
    StaticJsonDocument<300> cfgNet;

    // > Deserialize
    DeserializationError error = deserializeJson(cfgNet, netConfigF);
    if (error)
        Serial.println(F("[X] NET_P: Could not deserialize JSON."));

    // Populate config struct
    strlcpy(netConfig.Mode, cfgNet["Mode"], sizeof(netConfig.Mode));
    strlcpy(netConfig.AP_SSID, cfgNet["AP_SSID"], sizeof(netConfig.AP_SSID));
    strlcpy(netConfig.AP_PSK, cfgNet["AP_PSK"], sizeof(netConfig.AP_PSK));
    strlcpy(netConfig.WiFi_SSID, cfgNet["WiFi_SSID"], sizeof(netConfig.WiFi_SSID));
    strlcpy(netConfig.WiFi_PSK, cfgNet["WiFi_PSK"], sizeof(netConfig.WiFi_PSK));
    
    netConfigF.close();

    switch (mode) {
        case 1: return netConfig.Mode; break;
        case 2: return netConfig.AP_SSID; break;
        case 3: return netConfig.AP_PSK; break;
        case 4: return netConfig.WiFi_SSID; break;
        case 5: return netConfig.WiFi_PSK; break;
        default: return "[NET: unknown mode]";
    }

    return String();
}

//  ---------------------
//  TASKS
//  ---------------------

void taskWiFi(void* parameter) {
    // AP credentials (WPA)
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
        cfgNet["AP_SSID"] = AP_SSID;
        cfgNet["AP_PSK"] = AP_PSK;
        cfgNet["WiFi_SSID"] = "NaN";
        cfgNet["WiFi_PSK"] = "NaN";
        cfgNet["IP"] = "NaN";
        cfgNet["Netmask"] = "NaN";
        cfgNet["Gateway"] = "NaN";
        cfgNet["DNS"] = "NaN";

        // Write netConfig.cfg
        if (!(serializeJson(cfgNet, netConfigF)))
            Serial.println(F("[X] WiFi: Config write failure."));

        netConfigF.close();

        // Set time on RTC
        Serial.println(F("[>] WiFi: Config created."));
    } else {
        Serial.println(F("[i] WiFi: Config found!"));
    }

    // Parse net config file
    // Read file
    File netConfigF = LITTLEFS.open(F("/config/netConfig.json"), "r");

    // Parse JSON
    StaticJsonDocument<300> cfgNet;
    DeserializationError error = deserializeJson(cfgNet, netConfigF);
    if (error) {
        Serial.print(F("[X] WiFi: Could not deserialize JSON: "));
            Serial.println(error.c_str());
    }
        
    strlcpy(netConfig.Mode, cfgNet["Mode"], sizeof(netConfig.Mode));
    strlcpy(netConfig.AP_SSID, cfgNet["AP_SSID"], sizeof(netConfig.AP_SSID));
    strlcpy(netConfig.AP_PSK, cfgNet["AP_PSK"], sizeof(netConfig.AP_PSK));
    
    netConfigF.close();

    // Start WiFi AP if mode is 'AP'
    if (strcmp(netConfig.Mode, "AP") == 0 || strcmp(netConfig.WiFi_SSID, "NaN") == 0) {
        if (strcmp(netConfig.WiFi_SSID, "NaN") == 0)
            Serial.println("[!] WiFi: Mode is 'client', but SSID is invalid.");
        Serial.println("[i] WiFi: Starting AP.");

        WiFi.softAP(netConfig.AP_SSID, netConfig.AP_PSK);
        Serial.print("[i] WiFi: AP IP address: ");
            Serial.println(WiFi.softAPIP());

        WiFiReady = true;

        // Start mDNS
        const char* host = "nixie-clock";
        if (!MDNS.begin(host)) {
            Serial.println("[X] WiFi: mDNS setup error!");
        }

    // Start WiFi client if mode is 'Client'
    } else {
        APmode = false;
        Serial.println("[i] WiFi: Starting client.");
        Serial.print("[i] WiFi: Connecting to '");
            Serial.print(parseNetConfig(4));
            Serial.println("'.");

        int i = 0;
        WiFi.mode(WIFI_STA);

        // For some reason, parseNetConfig() mus be called prior to extracting data from the netConfig struct.
        // I have no idea why (yet)
        WiFi.begin(netConfig.WiFi_SSID, netConfig.WiFi_PSK);

        while (WiFi.status() != WL_CONNECTED) {
            if (i > 12) {
                Serial.println(F("[X] WiFi: Connection timeout."));

                // Open up a WiFi AP instead
                Serial.println(F("[i] WiFi: Starting AP as fallback..."));
                WiFi.softAP(netConfig.AP_SSID, netConfig.AP_PSK);
                Serial.print("[i] WiFi: AP IP address: ");
                    Serial.println(WiFi.softAPIP());

                APisFallback = true;
                WiFiReady = true;

                vTaskDelete(NULL);
            }

            vTaskDelay(1000);

            Serial.println(F("[T] WiFi: Connecting..."));
            i++;
        }

        Serial.print(F("[T] WiFi: Connected. RSSI: ")); Serial.println(WiFi.RSSI());

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

    // Acquire info from SPIFFS
    // Taken from: https://diyprojects.io/esp32-get-started-spiff-library-read-write-modify-files/
    Serial.println(F("[i] FS: Filesystem info:"));
    Serial.print(" > Total space:      ");
    Serial.print(LITTLEFS.totalBytes());
    Serial.println(" bytes");

    Serial.print(" > Total space used: ");
    Serial.print(LITTLEFS.usedBytes());
    Serial.println(" bytes");

    Serial.println();

    // Open dir folder
    File dir = SPIFFS.open("/");
    // List file at root
    listFilesInDir(dir, 0);

    vTaskDelete(NULL);
}

#endif