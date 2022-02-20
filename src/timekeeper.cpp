#include <timekeeper.h>
#include <rtc.h>

WiFiUDP ntpUDP;
RTC rtc;
Timekeeper _timekeeper;

//  Statics
bool Timekeeper::MountStatus = true;
int Timekeeper::UpdateInterval = 60000;
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
            JsonVariant b = cfgNTP["DstOffset"];
            JsonVariant c = cfgNTP["UtcOffset"];

            strlcpy(this->NtpSource, cfgNTP["NtpSource"], sizeof(this->NtpSource));
            this->DstOffset = b.as<int>();
            this->UtcOffset = c.as<int>();
        }

        ntpConfig.close();
    }
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
    
    bool rtcState = rtc.initialize();
    Serial.printf("[i] RTC state: %d\n", rtcState);
    
    uint8_t rtcStartupSate = rtc.checkStartup();
    Serial.printf("[i] RTC startup state: %d\n", rtcStartupSate);

    for (;;) {
        if (!netConfig.IsAP) {
            timeClient.update();
            Timekeeper::NowEpoch = timeClient.getEpochTime();

            Timekeeper::time.seconds = timeClient.getSeconds();
            Timekeeper::time.minutes = timeClient.getMinutes();
            Timekeeper::time.hours = timeClient.getHours();

            // Update RTC if applicable
            if (rtcState) {
                uint8_t rtcMinutes = rtc.getTime(1);
                uint8_t rtcHours = rtc.getTime(2);

                if (rtcMinutes != Timekeeper::time.minutes) {
                    Serial.println("Updating RTC minutes...");
                    rtc.setTime(1, Timekeeper::time.minutes);
                }
                if (rtcHours != Timekeeper::time.hours) {
                    Serial.println("Updating RTC hours...");
                    rtc.setTime(2, Timekeeper::time.hours);
                }
            }
        } else {
            Serial.println("Time source: RTC");
            if (rtcState) {
                Timekeeper::time.minutes = rtc.getTime(1);
                Timekeeper::time.hours = rtc.getTime(2);
            }
        }

        if (rtcState) {
            int timeArr[] = {
                rtc.getTime(2),
                rtc.getTime(1),
                rtc.getTime(0)
            };

            Serial.printf("[i] RTC time: %d:%d:%d\n", timeArr[0], timeArr[1], timeArr[2]);
        }

        vTaskDelay(1000);
    }
}