#include <EEPROM.h>
#include <SPI.h>
#include <RtcDS3234.h>
#include <sysInit.h>
#include <ArduinoJson.h>

#ifndef rtc_h
#define rtc_h

//  ---------------------
//  VARIABLES
//  ---------------------
// Time settings
char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

const uint8_t DS3234_CS_PIN = 5;

// Create RTC instance
RtcDS3234<SPIClass> Rtc(SPI, DS3234_CS_PIN);

// Structs
struct rtcConfigStruct {
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
            break;
        case 2:
            return config.Mode;
            break;
    }

    return String();
}

//  ---------------------
//  TASKS
//  ---------------------

void taskSetupRTC (void* parameters) {
    Serial.println(F("[T] RTC: Starting setup..."));

    // Wait for FlashFS mount mount
    while (!FlashFSready) {
        Serial.println(F("[T] RTC: No FlashFS yet."));
        vTaskDelay(500);
    }

    SPI.begin();
    Rtc.Begin();

    if (!Rtc.GetIsRunning()) {
        Serial.println(F("[T] RTC: RTC starting up."));
        Rtc.SetIsRunning(true);
    }

    // Create RTC config if it does not yet exist.
    // Additionally, set up RTC if it actually does not exist.
    Serial.println(F("[T] RTC: Looking for config..."));

    if (!(LITTLEFS.exists("/config/rtcConfig.json"))) {
        Serial.println(F("[T] RTC: No RTC config yet."));
        
        if (!LITTLEFS.exists("/config")) {
            LITTLEFS.mkdir("/config");
        }

        File rtcConfig = LITTLEFS.open(F("/config/rtcConfig.json"), "w");

        // Clear RTC
        Rtc.Enable32kHzPin(false);
        Rtc.SetSquareWavePin(DS3234SquareWavePin_ModeNone);

        // Construct JSON
        StaticJsonDocument<200> cfgRTC;

        cfgRTC["NTP"] = ntpServer;
        cfgRTC["GMT"] = gmtOffset_sec;
        cfgRTC["DST"] = daylightOffset_sec;
        cfgRTC["Mode"] = "NTP";

        // Write rtcConfig.cfg
        if (!(serializeJson(cfgRTC, rtcConfig))) {
            Serial.println(F("[X] RTC: RTC config write failure."));
        }

        rtcConfig.close();

        // Sync time with NTP
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        //struct tm timeinfo;

        // Set time on RTC
        Rtc.SetDateTime(RtcDateTime(__DATE__, __TIME__));
        Serial.println(F("[>] RTC: RTC config created."));
    } else {
        // ToDo: Check if RTC is behind NTP time
        Serial.println(F("[T] RTC: Config found!"));
        parseRTCconfig(1);
    }

    // ToDo: Implement handling of rtcConfig.json
    
    vTaskDelete(NULL);
}

#endif