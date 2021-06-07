/*
    ESP32 Nixie Clock - RTC module
    (c) V. Klopfenstein, 2021

    Anything related to RTC and time goes in here.
*/

#include <arduino.h>
#include <system/rtc.h>

//  ---------------------
//  FUNCTIONS
//  ---------------------

String RTCModule::getTime() {
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

String RTCModule::parseRTCconfig(int mode) {
    if (!RTCready)
        return "[RTC: failure]";

    // Read file
    File rtcCfgFile = LITTLEFS.open(F("/config/rtcConfig.json"), "r");

    // Parse JSON
    StaticJsonDocument<250> cfgRTC;

    // > Deserialize
    DeserializationError error = deserializeJson(cfgRTC, rtcCfgFile);
    if (error) {
        String err = error.c_str();

        Serial.print("[X] RTC parser: Deserialization fault: "); Serial.println(err);
        return "[Deserialization fault: " + err + "]";
    } else {
        // Populate config struct
        strlcpy(rtcConfig.NTP, cfgRTC["NTP"], sizeof(rtcConfig.NTP));
        strlcpy(rtcConfig.Mode, cfgRTC["Mode"], sizeof(rtcConfig.Mode));
        rtcConfig.GMT = cfgRTC["GMT"];
        rtcConfig.DST = cfgRTC["DST"];

        switch (mode) {
            case 1: return rtcConfig.NTP; break;
            case 2: return rtcConfig.Mode; break;
            case 3: return String(rtcConfig.GMT); break;
            case 4: return String(rtcConfig.DST); break;
            default: return "[RTC: unknown mode]";
        }
    }

    rtcCfgFile.close();

    return String();
}