/*
    ESP32 Nixie Clock - Tasks module
    (c) V. Klopfenstein, 2021

    Headers for nixie.cpp
*/

#ifndef nixie_h
#define nixie_h

#include <arduino.h>
#include <HTTPClient.h>

#ifndef rtc_h
    #include <system/rtc.h>
#endif

//  ---------------------
//  GLOBAL VARIABLES
//  ---------------------

extern bool nixieSetupComplete;
extern bool nixieAutonomous;
extern bool forceUpdate;
extern bool cycleNixies;
extern bool crypto;

// Nixie digits
extern char tube1Digit;
extern char tube2Digit;
extern char tube3Digit;
extern char tube4Digit;

//  ---------------------
//  Class
//  ---------------------
class Nixie {
    public:
        Nixie();

        byte decToBCD(byte);
        int displayCryptoPrice(String, String);
        String parseNixieConfig(int);

        void displayNumber(int, int, int, int);
        void updateBrightness(int);

        void update();
        void setup();

        struct nixieConfigStruct {
            char crypto_asset[8];
            char crypto_quote[8];
            int cathodeDepoisonTime;
            int cathodeDepoisonMode;
            int cathodeDepoisonInterval;
            int anodePWM;
        };

        nixieConfigStruct nixieConfig;

        int lastHour;
        int lastMinute;
    private:
        void setOptos();
        void clear();
};

#endif