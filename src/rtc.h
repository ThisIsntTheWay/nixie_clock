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

void parseRTCconfig() {
    // Read file
    int lineNumber = 0;
    String line;

    File rtcConfig = SPIFFS.open("/rtcConfig.cfg", FILE_READ);

    while (rtcConfig.available()) {
        lineNumber++;
        
        line = rtcConfig.readStringUntil('\n'); // Read line by line from the file
        Serial.print(lineNumber);
            Serial.println(line);
    }
}

// Create RTC instance
RtcDS3234<SPIClass> Rtc(SPI, DS3234_CS_PIN);

void taskSetupRTC (void* parameters) {
    Serial.println(F("[T] RTC: Starting setup..."));

    // Wait for SPIFFS mount
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
    if (!(SPIFFS.exists("/rtcConfig.cfg"))) {
        File rtcConfig = SPIFFS.open("/rtcConfig.cfg", FILE_WRITE);

        // Clear RTC
        Rtc.Enable32kHzPin(false);
        Rtc.SetSquareWavePin(DS3234SquareWavePin_ModeNone);

        // Write rtcConfig.cfg
        Serial.println(F("[T] RTC: No RTC config yet."));

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
        // Read file
        parseRTCconfig();
    }

    // ToDo: Implement handling of rtcConfig.cfg
    
    vTaskDelete(NULL);
}

#endif