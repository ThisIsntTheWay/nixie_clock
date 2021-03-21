#include <EEPROM.h>
#include <RTClib.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <sysInit.h>

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
RTC_DS1307 rtc;
WiFiUDP ntpUDP;

// Structs
struct rtcConfigStruct {
    DateTime manualTime;
    char NTP[64];
    char Mode[8];
    int GMT;
    int DST;
};

struct rtcConfigStruct config;

//  ---------------------
//  FUNCTIONS
//  ---------------------

String getTime() {
    /*
    Serial.println("GET TIME CALLED");
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        String out = "<i>Cannot get time!</i>";
    }

    Serial.println("CONSTRUCTED TIME");
    String out = (&timeinfo, "%A, %B %d %Y %H:%M:%S");
    return out;*/
    return "NODATA";
}

String parseRTCconfig(int mode) {
    // Mode param:
    // 1: Return NTP server
    // 2: Return config mode
    
    bool AutoConfig = true;

    // Read file
    File rtcConfig = LITTLEFS.open(F("/config/rtcConfig.json"), "r");

    // Parse JSON
    StaticJsonDocument<200> cfgRTC;

    // > Deserialize
    DeserializationError error = deserializeJson(cfgRTC, rtcConfig);
    if (error)
        Serial.println(F("[X] RTC_P: Could not deserialize JSON."));

    // Populate config struct
    strlcpy(config.NTP, cfgRTC["NTP"], sizeof(config.NTP));
    strlcpy(config.Mode, cfgRTC["Mode"], sizeof(config.Mode));
    config.GMT = cfgRTC["GMT"];
    config.DST = cfgRTC["DST"];
    
    rtcConfig.close();

    switch (mode) {
        case 1:
            return config.NTP;
            Serial.println(config.NTP);
            break;
        case 2:
            return config.Mode;
            Serial.println(config.Mode);
            break;
    }

    return String();
}

//  ---------------------
//  TASKS
//  ---------------------

void taskSetupRTC (void* parameters) {
    Serial.println(F("[T] RTC: Starting setup..."));

    // Wait for FS mount
    while (!FlashFSready) {
        Serial.println(F("[T] RTC: No FS yet."));
        vTaskDelay(500);
    }

    if (! rtc.begin())
        Serial.println("[X] RTC: No module found.");
    
    // Set RTC datetime if it hasn't been running yet
    if (! rtc.isrunning()) {
        Serial.println("[i] RTC: Toggling module.");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    // Create RTC config if it does not yet exist.
    // Additionally, set up RTC if it actually does not exist.
    Serial.println(F("[T] RTC: Looking for config..."));

    if (!(LITTLEFS.exists("/config/rtcConfig.json"))) {
        Serial.println(F("[T] RTC: No config found."));
        
        if (!LITTLEFS.exists("/config")) {
            LITTLEFS.mkdir("/config");
        }

        File rtcConfig = LITTLEFS.open(F("/config/rtcConfig.json"), "w");

        // Construct JSON
        StaticJsonDocument<200> cfgRTC;

        cfgRTC["NTP"] = ntpServer;
        cfgRTC["GMT"] = gmtOffset_sec;
        cfgRTC["DST"] = daylightOffset_sec;
        cfgRTC["Mode"] = "NTP";

        // Write rtcConfig.cfg
        if (!(serializeJson(cfgRTC, rtcConfig))) {
            Serial.println(F("[X] RTC: Config write failure."));
        }

        rtcConfig.close();

        // Sync time with NTP
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        //struct tm timeinfo;

        // Set time on RTC
        Serial.println(F("[>] RTC: Config created."));
    } else {
        // ToDo: Check if RTC is behind NTP time
        Serial.println(F("[T] RTC: Config found!"));

        // Sync RTC with NTP if mode is set to NTP
        if (parseRTCconfig(2) == "NTP") {
            Serial.print(F("[T] RTC: Syncing with NTP... "));
            // Initiate NTP client, but wait for WiFi first.
            while (!WiFiReady) {
                vTaskDelay(500);
            }
            NTPClient timeClient(ntpUDP, config.NTP, config.GMT);

            timeClient.begin();
            timeClient.update();

            long ntpTime = timeClient.getEpochTime();
            Serial.print("[T] RTC: NTP Epoch: ");
                Serial.println(ntpTime);
            Serial.print("[T] RTC: RTC Epoch: ");
                Serial.println(rtc.now().unixtime());
            
            // Adjust RTC
            rtc.adjust(DateTime(ntpTime));
        }
    }
    
    vTaskDelete(NULL);
}

void taskUpdateRTC(void* parameter) {
    // Wait for FS mount
    while (!FlashFSready) { vTaskDelay(500); }

    NTPClient timeClient(ntpUDP, config.NTP, config.GMT);
    // Check if updating is even required.

    if (parseRTCconfig(2) == "NTP") {
        timeClient.begin();
        timeClient.update();

        // Check if NTP and RTC epochs are different
        long ntpTime = timeClient.getEpochTime();
    }

    vTaskDelay(5000);
}

#endif