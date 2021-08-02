#ifndef CONFIG_H
#define CONFIG_H

#include <WiFi.h>
#include <ArduinoJson.h>
#include "LITTLEFS.h"

#define SPIFFS LITTLEFS

/****************************************************/
//  GLOBAL VARS
/****************************************************/
// PROGRAM INFO
#define BUILD_VERSION   0.5
#define BUILD_REL_TYPE  "Beta"

// PINS
#define DS_PIN  27   // Data
#define SH_PIN  26   // Clock
#define ST_PIN  25   // Latch

// VARIOUS
#define PWM_FREQUENCY       100     // PWM Frequency
#define DEPOISON_DELAY      40      // Interval in-between digit changes during cathode depoison routine
#define TUBES_BLINK_AMOUNT  3       // Amount of times the tubes should blink at bootup
#define CACHE_RENEWAL_INT   250     // Frequency of updating structs in the Configurator class

// FLAGS
#define EXP_BOARD_INSTALLED     // Expansion board installed (4 tubes)
//#define I2C_SLOWMODE          // I2C 100kHz
#define DEBUG                   // DEBUG serial output
//#define DEBUG_VERBOSE         // More DEBUG serial output

void listFilesInDir(File dir, int numTabs);

/****************************************************/
//  CONFIG CLASS
/****************************************************/
class Configurator {
    public:
        void prepareFS();
        void parseRTCconfig();
        void parseNixieConfig();
        void parseNetConfig();
        void nukeConfig();

        bool FSReady;           // FS mounted
        bool configInit;        // Config structs populated
        bool netReady;          // Network ready
        bool isAP;
        bool connectTimeout;    // WiFi client has timed out
        bool connecting;        // WiFi client is connecting

        static byte sysStatus;
            // 0: Connecting to WiFi network
            // 1: Starting AP
            // 2: Error

        TaskHandle_t *task_perp_nix;
        TaskHandle_t *task_perp_rtc;
        TaskHandle_t *task_perp_cac;

        struct RTCConfig {
            char ntpSource[64];
            bool isNTP;
            bool isDST;
            int tzOffset;
            int manEpoch;
        };
        
        struct NixieConfig {
            int mode;
                // 1: Clock
                // 2: Manual
                // 3: Crypto
            int brightness;
            int depoisonMode;
                // 1: On hour change
                // 2: On interval
            int depoisonInterval;
            
            bool crypto;
            bool tumble;
            
            char cryptoAsset[8];
            char cryptoQuote[8];

            // Tube numbers
            int nNum1;
            int nNum2;
            int nNum3;
            int nNum4;
        };

        struct NetConfig {
            char WiFi_SSID[64];
            char WiFi_PSK[64];
            bool WiFiClient;
            int IP[4];
            int Netmask[4];     // Caution: referenced as subnet by 'IPAdress'
            int Gateway[4];
            int DNS[4];
        };

        static struct RTCConfig rtcConfiguration;
        static struct NixieConfig nixieConfiguration;
        static struct NetConfig netConfiguration;
        static String buildInfo;
};

#endif