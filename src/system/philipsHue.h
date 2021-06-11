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

// Instances
RTC_DS3231 rtcHUE;
HTTPClient httpHUE;

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
    StaticJsonDocument<200> cfgHUE;

    // > Deserialize
    DeserializationError error = deserializeJson(cfgHUE, hueConfig);
    if (error) {
        String err = error.c_str();

        Serial.print("[X] HUE parser: Deserialization fault: "); Serial.println(err);
        return "[Deserialization fault: " + err + "]";
    } else {
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
    }

    return String();
}

// Get amount of lights
int getHueLightIndex() {
    // Acquire JSON response from HUE bridge
    String URI = "http://" + parseHUEconfig(1) + "/api/" + parseHUEconfig(2) + "/lights";
    int lightsAmount = 0;

    httpHUE.useHTTP10(true);

    if (httpHUE.begin(URI)) {
        if (httpHUE.GET() > 0) {
            // Deserialize JSON response from HUE bridge
            // Ref: https://arduinojson.org/v6/how-to/use-arduinojson-with-httpclient/
            DynamicJsonDocument doc(3200);
            deserializeJson(doc, httpHUE.getStream());

            JsonObject root = doc.as<JsonObject>();
            
            // Iterate through all root elements in JSON object
            if (doc.containsKey("error")) {
                Serial.println("[HUE] Could not determine light amount.");
                return 99;
            }

            // Increment lightsAmount by amount of elements in 'root'
            for (JsonPair kv : root) { lightsAmount++; }
        } else {
            Serial.println("[!] HUE: Cannot HTTP/GET lights index.");
        }
    } else {
        Serial.println("[!] HUE: Cannot init http for lights index.");
    }

    httpHUE.end();

    return lightsAmount;
}

// Dispatch POST
char sendHUELightReq(int lightID, bool state) {
    // Init connection to HUE bridge
    // Ref: https://developers.meethue.com/develop/get-started-2/
    String URI = "http://" + parseHUEconfig(1) + "/api/" + parseHUEconfig(2) + "/lights/" + lightID + "/state";
        //Serial.println(URI);

    httpHUE.begin(URI);

    // POST and return response
    String request;

    httpHUE.addHeader("Content-Type", "application/json");
    if (state == false) { request = "{\"on\": false}"; }
    else { request = "{\"on\": true}"; }

    int httpResponse = httpHUE.PUT(request);

    /*Serial.print("[i] HUE: HTTP Response: ");
        Serial.println(httpResponse);*/

    httpHUE.end();

    return httpResponse;
}

//  ---------------------
//  TASKS
//  ---------------------

void taskSetupHUE(void* paramter) {
    Serial.println("[T] HUE: Looking for config...");

    // Wait for FS mount
    while (!FlashFSready) { vTaskDelay(1000); }

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

    while (!FlashFSready) { vTaskDelay(1000); }

    vTaskDelay(5000);

    bool turnedOn = false;
    bool turnedOff = false;

    for (;;) {
        DateTime time = rtcHUE.now();
        bool timeIsValid = true;

        // Acquire onTime and offTime
        int onTime = parseHUEconfig(3).toInt();
        int offTime = parseHUEconfig(4).toInt();
        int nowHour = time.hour();
        int nowMinute = time.minute();

        int nowTime = ((nowHour * 100) + nowMinute);

        // Verify time
        //Serial.printf("[i] hour: %d\n", hour);
        if (time.hour() > 23 || time.minute() > 59) timeIsValid = false;
        
        // Turn lights ON if inbetween onH/onM and offH/offM
        if (timeIsValid) {        
            #ifdef DEBUG
                Serial.printf("nowHour:nowMinute, onTime|offTime: %d:%d, %d|%d\n", nowHour, nowMinute, onTime, offTime);
            #endif

                if (nowTime > onTime && nowTime < offTime) {
                    #ifdef DEBUG
                        Serial.printf("[T] HUE: onTime triggered. (%d | %d)\n", nowTime, onTime);
                    #endif

                    if (!turnedOn) {
                        int l = getHueLightIndex();
                        if (l != 0) {
                            for (int i = 0; i < l; i++) { sendHUELightReq(i + 1, true); }
                        }
                    }

                    turnedOn = true;
                    turnedOff = false;
                } 
                #ifdef DEBUG
                    else {
                        Serial.printf("HUE: skipping onTime [%d,%d] \n", turnedOn, turnedOff);
                    }
                #endif
                
                // Turn lights OFF
                if (nowTime > offTime || nowTime < onTime) {
                    #ifdef DEBUG
                        Serial.printf("[T] HUE: offTime triggered. (%d | %d)\n", nowTime, offTime);
                    #endif

                    if (!turnedOff) {
                        int l = getHueLightIndex();
                        if (l != 0) {
                            for (int i = 0; i < l; i++) { sendHUELightReq(i + 1, false); }
                        }
                    }

                    turnedOn = false;
                    turnedOff = true;
                }
                #ifdef DEBUG
                    else {
                        Serial.printf("HUE: skipping offTime [%d,%d] \n", turnedOn, turnedOff);
                    }
                #endif
        }

        #ifdef DEBUG
            else { Serial.printf("[T] HUE: Time invalid: %d\n", nowHour);}
        #endif

        vTaskDelay(2500);
    }

    vTaskDelete(NULL);
}

#endif