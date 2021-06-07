/*
    ESP32 Nixie Clock - System initialization
    (c) V. Klopfenstein, 2021

    Headers for tasks.cpp
*/

#ifndef sysInit_h
#define sysInit_h

#include <ArduinoJson.h>
#include "WiFi.h"
#include <WiFiClientSecure.h>

#define USE_LittleFS

#include <FS.h>
#ifdef USE_LittleFS
    #define SPIFFS LITTLEFS
    #include <LITTLEFS.h> 
    //#define CONFIG_LITTLEFS_SPIFFS_COMPAT 1
#else
    #include <SPIFFS.h>
#endif

extern bool FlashFSready;
extern bool WiFiReady;
extern bool APmode;
extern bool APisFallback;

class SysInit {
    public:
        String parseNetConfig(int);
        
        void listFilesInDir(File, int);

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

        netConfigStruct netConfig;
    private:
};

#endif