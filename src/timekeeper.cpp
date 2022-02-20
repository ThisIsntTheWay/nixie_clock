#include <timekeeper.h>

WiFiUDP ntpUDP;

//  Statics
bool Timekeeper::MountStatus = true;
int Timekeeper::UpdateInterval = 60000;
long Timekeeper::BootEpoch;
long Timekeeper::NowEpoch;

Timekeeper::Time Timekeeper::time;

int Timekeeper::DstOffset = 3600;
int Timekeeper::UtcOffset = 3600;
char Timekeeper::NtpSource[32];

Timekeeper _timekeeper;

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

    for (;;) {
        if (!netConfig.IsAP) {
            timeClient.update();
            Timekeeper::NowEpoch = timeClient.getEpochTime();
        }

        Timekeeper::time.seconds = timeClient.getSeconds();
        Timekeeper::time.minutes = timeClient.getMinutes();
        Timekeeper::time.hours = timeClient.getHours();
        
        vTaskDelay(1000);
    }
}