#ifndef RTC_H
#define RTC_H

#include <Arduino.h>
#include <rtc.h>
#include <wire.h>

#define RTC_ADDR 0x68

// Designed to work with DS3231
class RTC {
    public:
        bool RTCready;
        static bool RTCfault;
        static bool i2cLock;
        
        RTC();
        RTC(TwoWire & w);
        TwoWire & _Wire;

        bool initialize();

        byte readRegister(byte reg);
        byte writeRegister(byte reg, byte payload);

        int checkStartup();
        int getTime(int unit);
        int setTime(int unit, int value);

        String getTimeText();

    private:
        byte decToBcd(byte val);
        byte bcdToDec(byte val);
};

#endif