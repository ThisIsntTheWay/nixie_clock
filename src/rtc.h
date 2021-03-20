#include <EEPROM.h>
#include <sysInit.h>

#ifndef rtc
#define rtc

// Time settings
char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

void taskSetupRTC (void* parameters) {
    Serial.println(F("[T] RTC Setup: Starting..."));

    // Wait for SPIFFS mount
    while (!SPIFFSready) {
        Serial.println(F("[T] RTC Setup: SPIFFS unavailable."));
        vTaskDelay(500);
    }

    // Read RTC config within SPIFFS
    File rtcConfigW = SPIFFS.open("/rtcConfig.cfg", FILE_WRITE);

    // Create RTC config if it does not yet exist.
    if (!rtcConfigW) {
      Serial.println(F("[T] RTC Setup: No rtcConfig.cfg yet."));

      rtcConfigW.print("NTP = ");
      rtcConfigW.println(ntpServer);
      rtcConfigW.print("GMT = ");
      rtcConfigW.println(gmtOffset_sec);
      rtcConfigW.print("DST = ");
      rtcConfigW.println(daylightOffset_sec);
      rtcConfigW.println("MODE = NTP");
    }

    // Detect if RTC has been set for the first time already
    // If not, set using NTP
    Serial.print(F("[i] RTC Setup: Reading EEPROM: "));
    Serial.print(EEPROM.read(0));
    Serial.println(EEPROM.read(1));
    /*if (EEPROM.read(0) != 1) {
        Serial.println(F(" > [!] Wrote to EEPROM."));

        EEPROM.write(0, 1);
    }*/
    Serial.println(F("[i] RTC Setup: configTime set."));
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    //struct tm timeinfo;
    
    vTaskDelete(NULL);
}

#endif