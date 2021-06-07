/*
    ESP32 Nixie Clock - Tasks module
    (c) V. Klopfenstein, 2021

    Incorporates all FreeRTOS tasks
*/

#include <Arduino.h>
#include <system/tasks.h>
#include <system/webserver.h>
#include <utils/sysInit.h>

// Load nixie stuff
Nixie nixie;
RTCModule rtcModule;
SysInit sysInit;

// https://i0.wp.com/randomnerdtutorials.com/wp-content/uploads/2018/08/esp32-pinout-chip-ESP-WROOM-32.png
// Factory reset button GPIO pin
#define FACT_RST 17

// --------------------------------------
//  General
// --------------------------------------
void taskFSMount(void* parameter) {
    Serial.println(F("[T] FS: Mounting..."));

	if (LITTLEFS.begin()) {
		Serial.println("[T] FS: Mounted.");
        FlashFSready = true;
	} else {
		Serial.println("[X] FS: Mount failure.");
		Serial.println("[X] FS: Rebooting ESP.");
        //ESP.restart();
	}

    // Acquire info from SPIFFS
    // Taken from: https://diyprojects.io/esp32-get-started-spiff-library-read-write-modify-files/
    Serial.println(F("[i] FS: Filesystem info:"));
    Serial.print(" > Total space:      ");
    Serial.print(LITTLEFS.totalBytes());
    Serial.println(" bytes");

    Serial.print(" > Total space used: ");
    Serial.print(LITTLEFS.usedBytes());
    Serial.println(" bytes");

    Serial.println();

    // Open dir folder
    File dir = SPIFFS.open("/");
    // List file at root
    sysInit.listFilesInDir(dir, 0);

    vTaskDelete(NULL);
}

void taskWiFi(void* parameter) {
    // AP credentials (WPA)
    const char* AP_SSID = "ESP32 - Nixie clock";
    const char* AP_PSK  = "NixieClock2021";

    // Check for net config file
    Serial.println("[T] WiFi: Looking for config...");

    while (!FlashFSready) { vTaskDelay(1000); }
    if (!(LITTLEFS.exists("/config/netConfig.json"))) {
        Serial.println(F("[T] WiFi: No config found."));
        
        if (!LITTLEFS.exists("/config"))
            LITTLEFS.mkdir("/config");

        File netConfigF = LITTLEFS.open(F("/config/netConfig.json"), "w");

        // Construct JSON
        StaticJsonDocument<250> cfgNet;

        cfgNet["Mode"] = "AP";
        cfgNet["AP_SSID"] = AP_SSID;
        cfgNet["AP_PSK"] = AP_PSK;
        cfgNet["WiFi_SSID"] = "NaN";
        cfgNet["WiFi_PSK"] = "NaN";
        cfgNet["IP"] = "NaN";
        cfgNet["Netmask"] = "NaN";
        cfgNet["Gateway"] = "NaN";
        cfgNet["DNS"] = "NaN";

        // Write netConfig.cfg
        if (!(serializeJson(cfgNet, netConfigF)))
            Serial.println(F("[X] WiFi: Config write failure."));

        netConfigF.close();

        // Set time on RTC
        Serial.println(F("[>] WiFi: Config created."));
    } else {
        Serial.println(F("[i] WiFi: Config found!"));
    }

    // Parse net config file
    // Read file
    File netConfigF = LITTLEFS.open(F("/config/netConfig.json"), "r");

    // Parse JSON
    StaticJsonDocument<300> cfgNet;
    DeserializationError error = deserializeJson(cfgNet, netConfigF);
    if (error) {
        Serial.print(F("[X] WiFi: Could not deserialize JSON: "));
            Serial.println(error.c_str());
    }
        
    strlcpy(s.netConfig.Mode, cfgNet["Mode"], sizeof(s.netConfig.Mode));
    strlcpy(s.netConfig.AP_SSID, cfgNet["AP_SSID"], sizeof(s.netConfig.AP_SSID));
    strlcpy(s.netConfig.AP_PSK, cfgNet["AP_PSK"], sizeof(s.netConfig.AP_PSK));
    
    netConfigF.close();

    // Start WiFi AP if mode is 'AP'
    if (strcmp(s.netConfig.Mode, "AP") == 0 || strcmp(s.netConfig.WiFi_SSID, "NaN") == 0) {
        if (strcmp(s.netConfig.WiFi_SSID, "NaN") == 0)
            Serial.println("[!] WiFi: Mode is 'client', but SSID is invalid.");
        Serial.println("[i] WiFi: Starting AP.");

        WiFi.softAP(s.netConfig.AP_SSID, s.netConfig.AP_PSK);
        Serial.print("[i] WiFi: AP IP address: ");
            Serial.println(WiFi.softAPIP());

        WiFiReady = true;

    // Start WiFi client if mode is 'Client'
    } else {
        APmode = false;
        Serial.println("[i] WiFi: Starting client.");
        Serial.print("[i] WiFi: Connecting to '");
            Serial.print(s.parseNetConfig(4));
            Serial.print("' using '");
            Serial.print(s.parseNetConfig(5));
            Serial.println("'.");

        int i = 0;
        WiFi.mode(WIFI_STA);

        // For some reason, parseNetConfig() mus be called prior to extracting data from the netConfig struct.
        // I have no idea why (yet)
        WiFi.begin(s.netConfig.WiFi_SSID, s.netConfig.WiFi_PSK);

        while (WiFi.status() != WL_CONNECTED) {
            if (i > 12) {
                Serial.println(F("[X] WiFi: Connection timeout."));

                // Open up a WiFi AP instead
                Serial.println(F("[i] WiFi: Starting AP as fallback..."));
                WiFi.softAP(s.netConfig.AP_SSID, s.netConfig.AP_PSK);
                Serial.print("[i] WiFi: AP IP address: ");
                    Serial.println(WiFi.softAPIP());

                APisFallback = true;
                WiFiReady = true;

                vTaskDelete(NULL);
            }

            vTaskDelay(1000);

            Serial.println(F("[T] WiFi: Connecting..."));
            i++;
        }

        Serial.print(F("[T] WiFi: Connected. RSSI: ")); Serial.println(WiFi.RSSI());

        WiFiReady = true;
        Serial.print("[T] WiFi: IP: ");
            Serial.println(WiFi.localIP());
    }

    vTaskDelete(NULL);
}

// --------------------------------------
//  Nixies
// --------------------------------------
void taskSetupNixie(void* parameter) {
    while (!FlashFSready) { vTaskDelay(500); }

    nixie.setup();

    vTaskDelete(NULL);
}

void taskUpdateNixie(void* parameter) {
    // First time run
    Serial.println("[T] Nixie: preparing nixie updater...");

    while (!RTCready) { vTaskDelay(1000); }
    Serial.println("[T] Nixie: RTC ready.");

    nixie.lastMinute = 0;
    nixie.lastHour = 0;

    cycleNixies = true;

    Serial.println("[T] Nixie: Starting nixie updater...");
    for (;;) {
        // Check if nixies should update manually or automatically
        if (nixieAutonomous && !cycleNixies) {
            nixie.update();
        } else if (cycleNixies) {
            Serial.println("[T] Nixie: Cycling nixies...");

            // Cycle all nixies, first incrementing then decrementing them
            for (int i = 0; i < 10; i++) {
                nixie.displayNumber(i,i,i,i);
                vTaskDelay(65);
            }

            for (int i = 9; i > -1; i--) {
                nixie.displayNumber(i,i,i,i);
                vTaskDelay(65);
            }

            // Reset nixie state
            nixieAutonomous = true;
            cycleNixies = false;
            forceUpdate = true;
        } else if (crypto && !cycleNixies) {
            Serial.println("[T] Nixie: Displaying crypto price...");
            nixie.displayCryptoPrice(String(nixie.nixieConfig.crypto_asset), String(nixie.nixieConfig.crypto_quote));
        }

        vTaskDelay(400);
    }
}

void taskUpdateNixieBrightness(void* parameter) {
    // Wait for LittleFS and taskSetupNixie to be ready
    while (!FlashFSready && !nixieSetupComplete) { vTaskDelay(500); }

    Serial.println("[T] Nixie: Starting brightness updater...");
    for (;;) {
        // Set brightness of nixies
        int b = nixie.parseNixieConfig(5).toInt();
        nixie.updateBrightness(b);

        vTaskDelay(100);
    }
}

// --------------------------------------
//  RTC
// --------------------------------------

void taskSetupRTC (void* parameters) {
    Serial.println(F("[T] RTC: Starting setup..."));

    // Wait for FS mount
    int i = 0;
    while (!FlashFSready) {
        if (i > 10) {
            Serial.println("[X] RTC: FS mount timeout.");
            vTaskDelete(NULL);
        }

        i++;
        vTaskDelay(500);
    }

    // Set I2C clock speed
    Wire.setClock(100000);

    // Detect RTC
    if (!rtc.begin()) {
        Serial.println(F("[X] RTC: No module found."));
        Serial.println(F("[X] RTC: Aborting..."));
        vTaskDelete(NULL);
    } else { RTCready = true; }
    
    // Set RTC datetime if it hasn't been running yet
    // USE WITH DS1307 -> if (! rtc.isrunning()) {
    if (rtc.lostPower()) {
        Serial.println("[i] RTC: Module not configured, setting compile time...");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    // Create RTC config if it does not yet exist.
    // Additionally, set up RTC if it actually does not exist.
    Serial.println(F("[T] RTC: Looking for config..."));

    if (!(LITTLEFS.exists("/config/rtcConfig.json"))) {
        Serial.println(F("[T] RTC: No config found."));
        
        if (!LITTLEFS.exists("/config"))
            LITTLEFS.mkdir("/config");

        File rtcConfig = LITTLEFS.open(F("/config/rtcConfig.json"), "w");

        // Construct JSON
        StaticJsonDocument<200> cfgRTC;
        
        // Time settings
        char* ntpServer = "pool.ntp.org";
        const long  gmtOffset_sec = 3600;
        const int   daylightOffset_sec = 3600;

        cfgRTC["NTP"] = ntpServer;
        cfgRTC["GMT"] = gmtOffset_sec;
        cfgRTC["DST"] = daylightOffset_sec;
        cfgRTC["Mode"] = "ntp";

        // Write rtcConfig.cfg
        if (!(serializeJson(cfgRTC, rtcConfig)))
            Serial.println(F("[X] RTC: Config write failure."));

        rtcConfig.close();

        // Sync time with NTP
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

        // Set time on RTC
        Serial.println(F("[>] RTC: Config created."));
    }
    
    vTaskDelete(NULL);
}

void taskUpdateRTC(void* parameter) {
    // Wait for FS mount and WiFi to be connected    
    int i = 0;
    while (!FlashFSready) { vTaskDelay(500); }
    while (!WiFiReady) {
        vTaskDelay(500);

        while (!WiFiReady) {
            if (i > 30) {
                Serial.println("[X] RTC sync: Network timeout.");
                vTaskDelete(NULL);
            }
            i++;
            
            vTaskDelay(500);
        } 
    }
    Serial.println("[T] RTC sync: FS and WiFi both ready.");

    bool initialSync = true;

    // Artificial delay to wait for network
    vTaskDelay(5000);

    for (;;) {
        NTPClient timeClient(ntpUDP, rtcModule.rtcConfig.NTP, rtcModule.rtcConfig.GMT + rtcModule.rtcConfig.DST);

        // Check if RTC should be synced with NTP
        bool timeClientRunning = false;
        if (rtcModule.parseRTCconfig(2) == "ntp") {
            if (!APmode) {
                if (!timeClientRunning)
                    timeClient.begin();

                timeClientRunning = true;
                if (timeClient.forceUpdate()) {
                    NTPisValid = true;

                    // Check if NTP and RTC epochs are different
                    long ntpTime = timeClient.getEpochTime();
                    long epochDiff = ntpTime - rtc.now().unixtime();

                    if (initialSync) rtc.adjust(DateTime(ntpTime));

                    // Sync if epoch time differs too greatly from NTP and RTC
                    // Also ignore discrepancy if its difference is way too huge, indicating corrupt NTP packets
                    if ((epochDiff < -10 || epochDiff > 10) && !(epochDiff < -1200000000 || epochDiff > 1200000000)) {
                        Serial.print("[T] RTC sync: Clearing epoch difference of ");
                            Serial.println(epochDiff);
                        rtc.adjust(DateTime(ntpTime));
                    }
                } else {
                    Serial.println(F("[X] RTC sync: NTP server unresponsive."));
                    NTPisValid = false;
                }
            } else {
                // No sync if in AP mode as no internet connection is possible.
                Serial.println("[X] RTC sync: AP mode is active.");

                // Terminate NTP client
                if (timeClientRunning)
                    timeClient.end();
            }
        } else {
            Serial.println("[i] RTC sync: Manual mode is active.");

            // Terminate NTP client
            if (timeClientRunning)
                timeClient.end();
        }

        vTaskDelay(60000);
    }
}

// --------------------------------------
//  MAIN
// --------------------------------------

// Task to monitor if a factory reset has been triggered.
void taskfactoryResetWDT(void* parameter) {
    unsigned long activeMillis;
    unsigned long currMillis;

    pinMode(FACT_RST, INPUT_PULLDOWN);

    for (;;) {
        vTaskDelete(NULL);

        // Check if factory reset button is pressed using some millis() math
        currMillis = millis();
        if (digitalRead(FACT_RST)) activeMillis = currMillis;

        // Trigger factory reset if pin is HIGH for at least 5 seconds
        if ((currMillis - activeMillis >= 5000) || EnforceFactoryReset) {
            // Perform factory reset
            Serial.println("[!!!] FACTORY RESET INITIATED [!!!]");

            while (!FlashFSready) {
                Serial.println("[!] Reset: Awaiting FS mount...");
                vTaskDelay(500);
            }

            // Destroy all tasks
            Serial.println("[!] Reset: Destroying perpetual tasks...");
            vTaskDelete(TaskRTC_Handle);
            vTaskDelete(TaskNixie_Handle);
            vTaskDelete(TaskHUE_Handle);

            server.end();

            // Destroy config files
            Serial.println("[!] Reset: Nuking /config dir...");
            File configDir = LITTLEFS.open("/config");
            File configFile = configDir.openNextFile();

            while (configFile) {
                Serial.print(" > Destroying: ");
                    Serial.println(configFile.name());
                
                LITTLEFS.remove(configFile.name());

                configFile = configDir.openNextFile();
            }

            // At last, restart ESP
            Serial.println("[!] Reset: Restarting ESP...");
            ESP.restart();
        }
    }

    vTaskDelay(100);
}