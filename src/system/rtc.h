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
#include <utils/network.h>

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

bool RTCready = false;
bool NTPisValid = true;

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