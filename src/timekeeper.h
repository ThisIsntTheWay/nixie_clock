#ifndef TIMEKEEPER_H
#define TIMEKEEPER_H

#include <Arduino.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <LITTLEFS.h>
#include <ArduinoJson.h>
#include <networkConfig.h>

class Timekeeper {
    public:
        static long BootEpoch;
        static long NowEpoch;
        static int DstOffset;
        static int UtcOffset;
        static int UpdateInterval;
        static char NtpSource[32];
        static bool MountStatus;
        static bool RtcHealthy;
        
        void ParseNTPconfig(String);

        struct Time {
            uint8_t seconds;
            uint8_t minutes;
            uint8_t hours;
        };

        static struct Time time;
};

void taskTimekeeper (void *parameter);

#endif