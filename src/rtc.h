#include <EEPROM.h>
#include <SPI.h>
#include <RtcDS3234.h>
#include <sysInit.h>

#ifndef rtc_h
#define rtc_h

// Time settings
char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

const uint8_t DS3234_CS_PIN = 5;

char CFGFILE [512] = {'\0'};

// Create RTC instance
RtcDS3234<SPIClass> Rtc(SPI, DS3234_CS_PIN);

//  ---------------------
//  FUNCTIONS
//  ---------------------

String getTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        String out = "<i>Cannot get time!</i>";
    }

    String out = (&timeinfo, "%A, %B %d %Y %H:%M:%S");
    return out;
}

void parseRTCconfig() {
    // Read file
    File rtcConfig = LITTLEFS.open(F("/config/rtcConfig.cfg"), "r");

    // Iterate through file
    int i = 0;

    while (rtcConfig.available()) {
        CFGFILE [i] = rtcConfig.read();

        i++;
    }

    rtcConfig.close();
}

//  ---------------------
//  TASKS
//  ---------------------

void taskSetupRTC (void* parameters) {
    Serial.println(F("[T] RTC: Starting setup..."));

    // Wait for FlashFS mount mount
    while (!FlashFSready) {
        Serial.println(F("[T] RTC: No FlashFS yet."));
        vTaskDelay(500);
    }

    SPI.begin();
    Rtc.Begin();

    if (!Rtc.GetIsRunning()) {
        Serial.println(F("[T] RTC: RTC starting up."));
        Rtc.SetIsRunning(true);
    }

    // Create RTC config if it does not yet exist.
    // Additionally, set up RTC if it actually does not exist.
    Serial.println(F("[T] RTC: Looking for config..."));
    if (!(LITTLEFS.exists("/config/rtcConfig.cfg"))) {
        Serial.println(F("[T] RTC: No RTC config yet."));
        
        if (!LITTLEFS.exists("/config")) {
            LITTLEFS.mkdir("/config");
        }

        File rtcConfig = LITTLEFS.open(F("/config/rtcConfig.cfg"), "w");

        // Clear RTC
        Rtc.Enable32kHzPin(false);
        Rtc.SetSquareWavePin(DS3234SquareWavePin_ModeNone);

        // Write rtcConfig.cfg

        rtcConfig.print("NTP = ");
        rtcConfig.println(ntpServer);
        rtcConfig.print("GMT = ");
        rtcConfig.println(gmtOffset_sec);
        rtcConfig.print("DST = ");
        rtcConfig.println(daylightOffset_sec);
        rtcConfig.println("MODE = NTP");

        rtcConfig.close();

        // Sync time with NTP
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        //struct tm timeinfo;

        // Set time on RTC
        Rtc.SetDateTime(RtcDateTime(__DATE__, __TIME__));
        Serial.println(F("[>] RTC: RTC config created."));
    } else {
        Serial.println(F("[T] RTC: Config found!"));
        parseRTCconfig();
    }

    // ToDo: Implement handling of rtcConfig.cfg
    
    vTaskDelete(NULL);
}

#endif