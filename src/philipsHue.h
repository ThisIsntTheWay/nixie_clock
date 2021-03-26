/*
    ESP32 Nixie Clock - Philips HUE module
    (c) V. Klopfenstein, 2021

    All tasks/functions in here are related to interfacing with a philips HUE ecosystem.
    It...
     - Prepares and reads JSON configuration files related to philips HUE operations.
     - Interacts with a philips HUE bridge on demand.
*/

#include <sysInit.h>
#include <ArduinoJson.h>

#ifndef philipsHue_h
#define philipsHue_h

//  ---------------------
//  VARIABLES
//  ---------------------

// Structs
struct hueConfigStruct {
    char IP[15];
    int toggleOnTime;
    int toggleOffTime;
};

struct hueConfigStruct hueConfigStr;

//  ---------------------
//  FUNCTIONS
//  ---------------------

String parseHUEconfig(int mode) {
    // Read file
    File hueConfig = LITTLEFS.open(F("/config/hueConfig.json"), "r");

    // Parse JSON
    StaticJsonDocument<150> cfgHUE;

    // > Deserialize
    DeserializationError error = deserializeJson(cfgHUE, hueConfig);
    if (error)
        Serial.println(F("[X] RTC_P: Could not deserialize JSON."));

    // Populate config struct
    strlcpy(hueConfigStr.IP, cfgHUE["IP"], sizeof(hueConfigStr.IP));
    hueConfigStr.toggleOnTime = cfgHUE["toggleOnTime"];
    hueConfigStr.toggleOffTime = cfgHUE["toggleOffTime"];

    hueConfig.close();

    // Determine what variable to return
    switch (mode) {
        case 1:
            return hueConfigStr.IP;
            break;
        case 2:
            return String(hueConfigStr.toggleOnTime);
            break;
        case 3:
            return String(hueConfigStr.toggleOffTime);
    }

    return String();
}

//  ---------------------
//  TASKS
//  ---------------------

void taskSetupHUE(void* paramter) {
    Serial.println(F("[T] HUE: Looking for config..."));

    // Wait for FS mount
    while (!FlashFSready) {
        Serial.println(F("[T] HUE: No FS yet."));
        vTaskDelay(500);
    }

    // Open HUE config
    if (!(LITTLEFS.exists("/config/hueConfig.json"))) {
        Serial.println(F("[T] HUE: No config yet."));
        
        if (!LITTLEFS.exists("/config")) {
            LITTLEFS.mkdir("/config");
        }

        File hueConfig = LITTLEFS.open(F("/config/hueConfig.json"), "w");

        // Construct JSON
        StaticJsonDocument<150> cfgHUE;

        cfgHUE["IP"] = "192.168.1.0";
        cfgHUE["toggleOnTime"] = "0800";
        cfgHUE["toggleOffTime"] = "1800";

        // Write config file
        if (!(serializeJson(cfgHUE, hueConfig))) {
            Serial.println(F("[X] HUE: Config write failure."));
        }

        hueConfig.close();
        Serial.println(F("[>] HUE: Config created."));
    } else {
        // ToDo: Check if RTC is behind NTP time
        Serial.println(F("[T] HUE: Config found!"));
    }

    vTaskDelete(NULL);
}

#endif