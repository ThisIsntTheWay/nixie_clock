/*
    ESP32 Nixie Clock - Philips HUE module
    (c) V. Klopfenstein, 2021

    All tasks/functions in here are related to interfacing with a philips HUE ecosystem.
    It...
     - Prepares and reads JSON configuration files related to philips HUE operations.
     - Interacts with a philips HUE bridge on demand.
*/

#include <utils/sysInit.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

#ifndef philipsHue_h
#define philipsHue_h

//  ---------------------
//  VARIABLES
//  ---------------------

HTTPClient http;

// Structs
struct hueConfigStruct {
    char IP[15];
    char username[42];
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
        //Serial.println("[X] HUE parser: Could not deserialize JSON.");

    // Populate config struct
    strlcpy(hueConfigStr.IP, cfgHUE["IP"], sizeof(hueConfigStr.IP));
    strlcpy(hueConfigStr.username, cfgHUE["user"], sizeof(hueConfigStr.username));
    hueConfigStr.toggleOnTime = cfgHUE["toggleOnTime"];
    hueConfigStr.toggleOffTime = cfgHUE["toggleOffTime"];

    hueConfig.close();

    // Determine what variable to return
    switch (mode) {
        case 1: return hueConfigStr.IP; break;
        case 2: return hueConfigStr.username; break;
        case 3: return String(hueConfigStr.toggleOnTime); break;
        case 4: return String(hueConfigStr.toggleOffTime); break;
        default: return "[HUE parser: wrong param]";
    }

    return String();
}

// Get amount of lights
int getHueLightIndex() {
    // Acquire JSON response from HUE bridge
    String URI = "http://" + parseHUEconfig(1) + "/api/" + parseHUEconfig(2) + "/lights";
    Serial.print("HUE LIGHT INDEX URI: ");
        Serial.println(URI);

    http.useHTTP10(true);
    http.begin(URI);
    http.GET();

    // Deserialize JSON response from HUE bridge
    // Ref: https://arduinojson.org/v6/how-to/use-arduinojson-with-httpclient/
    DynamicJsonDocument doc(3200);
    deserializeJson(doc, http.getStream());
    http.end();

    JsonObject root = doc.as<JsonObject>();
    
    // Iterate through all root elements in JSON object
    if (doc.containsKey("error")) {
        Serial.println("[HUE] Could not determine light amount.");
        return 99;
    }

    int lightsAmount = 0;
    for (JsonPair kv : root) { lightsAmount++; }

    return lightsAmount;
}

// Dispatch POST
char sendHUELightReq(int lightID, bool state) {
    // Init connection to HUE bridge
    // Ref: https://developers.meethue.com/develop/get-started-2/
    String URI = "http://" + parseHUEconfig(1) + "/api/" + parseHUEconfig(2) + "/lights/" + lightID + "/state";
    Serial.println(URI);

    http.begin(URI);

    // POST and return response
    String request;

    http.addHeader("Content-Type", "application/json");
    if (state == false) { request = "{\"on\": false}"; }
    else { request = "{\"on\": true}"; }

    int httpResponse = http.PUT(request);

    Serial.print("[i] HUE: HTTP Response: ");
        Serial.println(httpResponse);

    http.end();

    return httpResponse;
}

//  ---------------------
//  TASKS
//  ---------------------

void taskSetupHUE(void* paramter) {
    Serial.println("[T] HUE: Looking for config...");

    // Wait for FS mount
    while (!FlashFSready) {
        Serial.println("[T] HUE: FS not yet ready...");
        vTaskDelay(500);
    }

    // Open HUE config
    if (!(LITTLEFS.exists("/config/hueConfig.json"))) {
        Serial.println("[T] HUE: No config found.");
        
        if (!LITTLEFS.exists("/config")) {
            LITTLEFS.mkdir("/config");
        }

        File hueConfig = LITTLEFS.open(F("/config/hueConfig.json"), "w");

        // Construct JSON
        StaticJsonDocument<150> cfgHUE;

        cfgHUE["IP"] = "192.168.1.0";
        cfgHUE["user"] = "CHANGEME";
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