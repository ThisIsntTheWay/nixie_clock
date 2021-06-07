/*
    ESP32 Nixie Clock - Nixie module
    (c) V. Klopfenstein, 2021

    All tasks/functions in here are related to nixie tube and bitshift operations.
    It...
     - Prepares and reads JSON configuration files related to nixie operations.
     - Regularely updates the nixie tubes.
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

bool nixieSetupComplete = false;
bool nixieAutonomous = true;
bool forceUpdate = true;
bool cycleNixies = false;
bool crypto = false;

// Nixie digits
char tube1Digit = 0;
char tube2Digit = 0;
char tube3Digit = 0;
char tube4Digit = 0;

// Temp storage for nixie digits
int oldDigit1;
int oldDigit2;
int oldDigit3;
int oldDigit4;

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