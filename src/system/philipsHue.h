/*
    ESP32 Nixie Clock - Philips HUE module
    (c) V. Klopfenstein, 2021

    All tasks/functions in here are related to interfacing with a philips HUE ecosystem.
    It...
     - Prepares and reads JSON configuration files related to philips HUE operations.
     - Interacts with a philips HUE bridge on demand.
*/

#include <utils/network.h>
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
        default: return "[HUE: unknown mode]";
    }

    return String();
}

// Get amount of lights
int getHueLightIndex() {
    // Acquire JSON response from HUE bridge
    String URI = "http://" + parseHUEconfig(1) + "/api/" + parseHUEconfig(2) + "/lights";

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
        //Serial.println(URI);

    http.begin(URI);

    // POST and return response
    String request;

    http.addHeader("Content-Type", "application/json");
    if (state == false) { request = "{\"on\": false}"; }
    else { request = "{\"on\": true}"; }

    int httpResponse = http.PUT(request);

    /*Serial.print("[i] HUE: HTTP Response: ");
        Serial.println(httpResponse);*/

    http.end();

    return httpResponse;
}

//  ---------------------
//  TASKS
//  ---------------------

void taskSetupHUE(void* paramter) {
    Serial.println("[T] HUE: Looking for config...");

    // Wait for FS mount
    int i = 0;
    while (!FlashFSready) {
        if (i > 10) {
            Serial.println("[X] HUE: FS mount timeout.");
            vTaskDelete(NULL);
        }

        i++;
        vTaskDelay(500);
    }

    // Open HUE config
    if (!(LITTLEFS.exists("/config/hueConfig.json"))) {
        Serial.println("[T] HUE: No config found.");
        
        if (!LITTLEFS.exists("/config"))
            LITTLEFS.mkdir("/config");

        File hueConfig = LITTLEFS.open(F("/config/hueConfig.json"), "w");

        // Construct JSON
        StaticJsonDocument<150> cfgHUE;

        cfgHUE["IP"] = "192.168.1.0";
        cfgHUE["user"] = "CHANGEME";
        cfgHUE["toggleOnTime"] = "0800";
        cfgHUE["toggleOffTime"] = "1800";

        // Write config file
        if (!(serializeJson(cfgHUE, hueConfig)))
            Serial.println(F("[X] HUE: Config write failure."));

        hueConfig.close();
        Serial.println(F("[>] HUE: Config created."));
    }

    vTaskDelete(NULL);
}

void taskMonitorHUE(void* parameter) {
    DateTime time = rtc.now();

    while (!FlashFSready) { vTaskDelay(1000); }

    vTaskDelay(5000);

    bool turnedOn = false;
    bool turnedOff = false;

    for (;;) {
        // Copy strings into respective char arrays.
        // First, declare char arrays of the length of the respective string.
        // (The + 1 is required for the null-terminated char arrays.)
        int onTimeStrLength = (parseHUEconfig(3)).length() + 1;
        int offTimeStrLength = (parseHUEconfig(4)).length() + 1;

        char onTime[onTimeStrLength];
        char offTime[offTimeStrLength];
        
        // Then, copy said string into char array.
        (parseHUEconfig(3)).toCharArray(onTime, onTimeStrLength);
        (parseHUEconfig(4)).toCharArray(offTime, offTimeStrLength);

        // ------------
        // Turn ON lights if ON time has bigger than HOUR and minutes
        
        /*
        Serial.print("onTime: ");
        Serial.print( 10*(onTime[0] - '0') + (onTime[1] - '0') );
        Serial.println( 10*(onTime[2] - '0') + (onTime[3] - '0') );
        Serial.print("ofTime: ");
        Serial.print( 10*(offTime[0] - '0') + (offTime[1] - '0') );
        Serial.println( 10*(offTime[2] - '0') + (offTime[3] - '0') );
        Serial.println("------");*/

        // In order to compare against hours and minutes seperately, we need to split an int of 4 numbers into 2.
        // Because a char array saves stuff as ASCII characters, a typical int operation will lead to undesirable results.
        // As such, '0' needs to be substracted from 'char_array[n]'.

        int onH = 10*(onTime[0] - '0') + (onTime[1] - '0');
        int onM = 10*(onTime[2] - '0') + (onTime[3] - '0');
        int ofH = 10*(offTime[0] - '0') + (offTime[1] - '0');
        int ofM = 10*(offTime[2] - '0') + (offTime[3] - '0');

        // Turn lights ON
        if (onH > time.hour() && onM > time.minute()  && !turnedOn) {
            Serial.println("[T] HUE: onTime triggered.");

            //int l = getHueLightIndex();
            //for (int i = 0; i < l; i++) { sendHUELightReq(i + 1, true); }

            turnedOn = true;
            turnedOff = false;
        }
        
        // ------------
        // Turn lights OFF
        if ( (ofH > time.hour() || ofH < time.hour()) && \
             (ofM > time.minute() || ofM < time.minute()) \
             && !turnedOff ) {

                Serial.println("[T] HUE: offTime triggered.");

                //int l = getHueLightIndex();
                //for (int i = 0; i < l; i++) { sendHUELightReq(i + 1, false); }

                turnedOn = false;
                turnedOff = true;
        }

        vTaskDelay(2500);

    }

    vTaskDelete(NULL);
}

#endif