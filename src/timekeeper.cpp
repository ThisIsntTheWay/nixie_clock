#include <timekeeper.h>
#include <rtc.h>
#include <displayController.h>

WiFiUDP ntpUDP;
RTC rtc;
Timekeeper _timekeeper;
DisplayController _displayController;

bool Timekeeper::MountStatus = true;
bool Timekeeper::RtcHealthy = true;
int Timekeeper::UpdateInterval = 60000;
int8_t Timekeeper::LastHour = 99;
long Timekeeper::BootEpoch;
long Timekeeper::NowEpoch;

Timekeeper::Time Timekeeper::time;

int Timekeeper::DstOffset = 3600;
int Timekeeper::UtcOffset = 3600;
char Timekeeper::NtpSource[32];

void Timekeeper::ParseNTPconfig(String ntpFile) {
    if (!LITTLEFS.exists(ntpFile)) {
        // Construct default config
        File ntpConfig = LITTLEFS.open(ntpFile, "w");

        StaticJsonDocument<200> cfgNTP;

        const char* NtpSource = "ch.pool.ntp.org";

        cfgNTP["NtpSource"] = NtpSource;
        cfgNTP["DstOffset"] = 3600;
        cfgNTP["UtcOffset"] = 3600;

        // Write rtcConfig.cfg
        if (!(serializeJson(cfgNTP, ntpConfig)))
            Serial.println(F("[X] NTP: Config write failure."));

        ntpConfig.close();
    } else {
        // Parse existing config
        File ntpConfig = LITTLEFS.open(ntpFile, "r");
        
        StaticJsonDocument<250> cfgNTP;
        DeserializationError error = deserializeJson(cfgNTP, ntpConfig);
        if (error) {
            String err = error.c_str();

            Serial.print("[X] RTC parser: Deserialization fault: "); Serial.println(err);
        } else {
            strlcpy(this->NtpSource, cfgNTP["NtpSource"], sizeof(this->NtpSource));

            JsonVariant b = cfgNTP["DstOffset"];
            JsonVariant c = cfgNTP["UtcOffset"];
            this->DstOffset = b.as<int>();
            this->UtcOffset = c.as<int>();
        }

        ntpConfig.close();
    }
}

// Overload 1: Only write DST
bool Timekeeper::WriteNTPconfig(bool DstOffset) {
    File ntpConfig = LITTLEFS.open("/ntpConfig.json", "w");

    StaticJsonDocument<200> cfgNTP;

    cfgNTP["DstOffset"] = DstOffset ? 3600: 0;

    // Write rtcConfig.cfg
    if (!(serializeJson(cfgNTP, ntpConfig))) {
        Serial.println(F("[X] NTP: Config write failure."));
        ntpConfig.close();
        return false;
    }

    ntpConfig.close();
    return true;
}

// Overload 2: Write both DST and UTC
bool Timekeeper::WriteNTPconfig(bool DstOffset, int UtcOffset) {
    File ntpConfig = LITTLEFS.open("/ntpConfig.json", "w");

    StaticJsonDocument<200> cfgNTP;

    cfgNTP["DstOffset"] = DstOffset ? 3600: 0;;
    cfgNTP["UtcOffset"] = UtcOffset;

    // Write rtcConfig.cfg
    if (!(serializeJson(cfgNTP, ntpConfig))) {
        Serial.println(F("[X] NTP: Config write failure."));
        ntpConfig.close();
        return false;
    }

    ntpConfig.close();
    return true;
}

void taskTimekeeper(void *parameter) {
    String ntpFile = "/ntpConfig.json";

    if (!LITTLEFS.begin()) {
        Serial.println("[X] FS: Filesystem mount failure.");
        Timekeeper::MountStatus = false;
    } else {
        _timekeeper.ParseNTPconfig(ntpFile);
    }

    // Start updaing time
    NetworkConfig netConfig;

    NTPClient timeClient(ntpUDP, Timekeeper::NtpSource, Timekeeper::DstOffset, Timekeeper::UpdateInterval);
    timeClient.begin();
    timeClient.update();

    Timekeeper::BootEpoch = timeClient.getEpochTime();
    
    Timekeeper::RtcHealthy = rtc.initialize();
    Serial.printf("[i] RTC state: %d\n", Timekeeper::RtcHealthy);
    
    uint8_t rtcStartupSate = rtc.checkStartup();
    Serial.printf("[i] RTC startup state: %d\n", rtcStartupSate);

    bool checkRtcTime = true;

    for (;;) {
        if (!netConfig.IsAP) {
            timeClient.update();
            Timekeeper::NowEpoch = timeClient.getEpochTime();

            Timekeeper::time.seconds = timeClient.getSeconds();
            Timekeeper::time.minutes = timeClient.getMinutes();
            Timekeeper::time.hours = timeClient.getHours();

            // Schedule detox
            // Perpetual detox durin night hours
            if (Timekeeper::time.hours >= 1 && Timekeeper::time.hours <= 5) {
                _displayController.DoDetox = true;
            }

            // Schedule detox on hour change
            else if (Timekeeper::time.hours != Timekeeper::LastHour) {
                Timekeeper::LastHour = Timekeeper::time.hours;
                _displayController.DoDetox = true;
            }

            // Update RTC if applicable
            if (Timekeeper::RtcHealthy && checkRtcTime) {
                if (rtc.getTime(0) != Timekeeper::time.seconds) {
                    Serial.println("Updating RTC seconds...");
                    rtc.setTime(0, Timekeeper::time.seconds);
                }

                if (rtc.getTime(1) != Timekeeper::time.minutes) {
                    Serial.println("Updating RTC minutes...");
                    rtc.setTime(1, Timekeeper::time.minutes);
                }

                if (rtc.getTime(2) != Timekeeper::time.hours) {
                    Serial.println("Updating RTC hours...");
                    rtc.setTime(2, Timekeeper::time.hours);
                }

                checkRtcTime = false;
            }
        } else {
            Serial.println("Time source: RTC");
            if (Timekeeper::RtcHealthy) {
                Timekeeper::time.minutes = rtc.getTime(1);
                Timekeeper::time.hours = rtc.getTime(2);
            }
        }

        /*
        if (Timekeeper::RtcHealthy) {
            int timeArr[] = {
                rtc.getTime(2),
                rtc.getTime(1),
                rtc.getTime(0)
            };

            Serial.printf("[i] RTC time: %d:%d:%d\n", timeArr[0], timeArr[1], timeArr[2]);
            Serial.printf("[i] Timekeeper time: %d:%d:%d\n", Timekeeper::time.hours, Timekeeper::time.minutes, Timekeeper::time.seconds);
        }
        */

        vTaskDelay(1000);
    }
}