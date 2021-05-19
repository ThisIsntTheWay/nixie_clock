/*
    ESP32 Nixie Clock - RTC module
    (c) V. Klopfenstein, 2021

    All tasks/functions in here are related to RTC initialization and usage.
    It...
     - Inits the RTC
     - Prepares and reads JSON configuration files related to RTC operations.
     - Regularely synchronizes the RTC with an NTP endpoint.
*/

#include <RTClib.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <utils/network.h>

#ifndef rtc_h
#define rtc_h

//  ---------------------
//  VARIABLES
//  ---------------------
// Time settings
char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// Instances
//RTC_DS1307 rtc;
RTC_DS3231 rtc;
WiFiUDP ntpUDP;

// Structs
struct rtcConfigStruct {
    int manualTime;
    char NTP[64];
    char Mode[8];
    int GMT;
    int DST;
};

struct rtcConfigStruct config;

bool RTCready = false;

//  ---------------------
//  FUNCTIONS
//  ---------------------

String getTime() {
    // Assemble datetime string
    if (!RTCready) {
        return String("<span style='color:red'>RTC failure</span>");
    } else {
        char buf1[15];
        DateTime now = rtc.now();
        //Serial.printf("[i] Free heap: %d\n", ESP.getFreeHeap());

        snprintf(buf1, sizeof(buf1), "%02d:%02d:%02d",  now.hour(), now.minute(), now.second());

        return buf1;
    }
}

String parseRTCconfig(int mode) {
    // Mode param:
    // 1: Return NTP server
    // 2: Return config mode

    if (!RTCready)
        return "[RTC: failure]";

    // Read file
    File rtcConfig = LITTLEFS.open(F("/config/rtcConfig.json"), "r");

    // Parse JSON
    StaticJsonDocument<250> cfgRTC;

    // > Deserialize
    DeserializationError error = deserializeJson(cfgRTC, rtcConfig);
    if (error) {
        String err = error.c_str();

        Serial.print("[X] RTC parser: Deserialization fault: "); Serial.println(err);
        return "[Deserialization fault: " + err + "]";
    } else {
        // Populate config struct
        strlcpy(config.NTP, cfgRTC["NTP"], sizeof(config.NTP));
        strlcpy(config.Mode, cfgRTC["Mode"], sizeof(config.Mode));
        config.GMT = cfgRTC["GMT"];
        config.DST = cfgRTC["DST"];

        String gmt = String(config.GMT);
        String dst = String(config.DST);
        
        rtcConfig.close();

        switch (mode) {
            case 1: return config.NTP; break;
            case 2: return config.Mode; break;
            case 3: return gmt; break;
            case 4: return dst; break;
            default: return "[RTC: unknown mode]";
        }
    }

    return String();
}

//  ---------------------
//  TASKS
//  ---------------------

void taskSetupRTC (void* parameters) {
    Serial.println(F("[T] RTC: Starting setup..."));

    // Wait for FS mount
    int i = 0;
    while (!FlashFSready) {
        if (i > 10) {
            Serial.println("[X] RTC: FS mount timeout.");
            vTaskDelete(NULL);
        }

        i++;
        vTaskDelay(500);
    }

    if (!rtc.begin()) {
        Serial.println(F("[X] RTC: No module found."));
        Serial.println(F("[X] RTC: Aborting..."));
        vTaskDelete(NULL);
    } else { RTCready = true; }
    
    // Set RTC datetime if it hasn't been running yet
    // USE WITH DS1307 -> if (! rtc.isrunning()) {
    if (rtc.lostPower()) {
        Serial.println("[i] RTC: Module not configured, setting compile time...");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    // Create RTC config if it does not yet exist.
    // Additionally, set up RTC if it actually does not exist.
    Serial.println(F("[T] RTC: Looking for config..."));

    if (!(LITTLEFS.exists("/config/rtcConfig.json"))) {
        Serial.println(F("[T] RTC: No config found."));
        
        if (!LITTLEFS.exists("/config"))
            LITTLEFS.mkdir("/config");

        File rtcConfig = LITTLEFS.open(F("/config/rtcConfig.json"), "w");

        // Construct JSON
        StaticJsonDocument<200> cfgRTC;

        cfgRTC["NTP"] = ntpServer;
        cfgRTC["GMT"] = gmtOffset_sec;
        cfgRTC["DST"] = daylightOffset_sec;
        cfgRTC["Mode"] = "ntp";

        // Write rtcConfig.cfg
        if (!(serializeJson(cfgRTC, rtcConfig)))
            Serial.println(F("[X] RTC: Config write failure."));

        rtcConfig.close();

        // Sync time with NTP
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

        // Set time on RTC
        Serial.println(F("[>] RTC: Config created."));
    }
    
    vTaskDelete(NULL);
}

void taskUpdateRTC(void* parameter) {
    // Wait for FS mount and WiFi to be connected
    
    int i = 0;
    while (!FlashFSready) { vTaskDelay(500); }
    while (!WiFiReady) {
        vTaskDelay(500);

        while (!WiFiReady) {
            if (i > 30) {
                Serial.println("[X] RTC sync: Network timeout.");
                vTaskDelete(NULL);
            }
            i++;
            
            vTaskDelay(500);
        } 
    }
    Serial.println("[T] RTC sync: FS and WiFi both ready.");

    // Artificial delay to wait for network
    vTaskDelay(1000);

    for (;;) {
        NTPClient timeClient(ntpUDP, config.NTP, config.GMT + config.DST);

        // Check if RTC should be synced with NTP
        bool timeClientRunning = false;
        if (parseRTCconfig(2) == "ntp") {
            if (!APmode) {
                if (!timeClientRunning)
                    timeClient.begin();
                    
                timeClient.forceUpdate();

                timeClientRunning = true;

                // Check if NTP and RTC epochs are different
                long ntpTime = timeClient.getEpochTime();
                long epochDiff = ntpTime - rtc.now().unixtime();

                // Sync if epoch time differs too greatly from NTP and RTC
                // Also ignore discrepancy if its difference is way too huge, indicating corrupt NTP packets
                if ((epochDiff < -10 || epochDiff > 10) && !(epochDiff < -1200000000 || epochDiff > 1200000000)) {
                    Serial.print("[T] RTC sync: Clearing epoch difference of ");
                        Serial.println(epochDiff);
                    rtc.adjust(DateTime(ntpTime));
                }
            } else {
                // No sync if in AP mode as no internet connection is possible.
                Serial.println("[X] RTC sync: AP mode is active.");

                // Terminate NTP client
                if (timeClientRunning)
                    timeClient.end();
            }
        } else {
            Serial.println("[i] RTC sync: Manual mode is active.");

            // Terminate NTP client
            if (timeClientRunning)
                timeClient.end();
        }

        vTaskDelay(60000);
    }
}

#endif