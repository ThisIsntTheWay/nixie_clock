/*
    ESP32 Nixie Clock - RTC module
    (c) V. Klopfenstein, 2021

    All tasks/functions in here are related to RTC initialization and usage.
    It...
     - Inits the RTC
     - Prepares and reads JSON configuration files related to RTC operations.
     - Regularely synchronizes the RTC with an NTP endpoint.
*/

#ifndef rtc_h
#define rtc_h

#include <RTClib.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <utils/sysInit.h>

//  ---------------------
//  VARIABLES
//  ---------------------

// Instances
//RTC_DS1307 rtc;
extern RTC_DS3231 rtc;
extern WiFiUDP ntpUDP;

extern bool RTCready;
extern bool NTPisValid;

//  ---------------------
//  Class
//  ---------------------
class RTCModule {
    public:
        RTCModule();

        String getTime();
        String parseRTCconfig(int);

        struct rtcConfigStruct {
            int manualTime;
            char NTP[64];
            char Mode[8];
            int GMT;
            int DST;
        };

        rtcConfigStruct rtcConfig;
    private:
};

#endif