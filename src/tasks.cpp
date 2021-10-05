#include <Arduino.h>
#include <tasks.h>
#include <NTPClient.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include <nixies.h>
#include <rtc.h>
#include <webserver.h>

#define ONBOARD_LED 16

RTC rtc;
Nixies nixies;
Configurator config;

WiFiUDP ntpUDP;

void taskSysInit(void* parameter) {
    config.configInit = false;

    config.prepareFS();

    config.sysStatus = 0;

    while (!config.FSReady) {
        vTaskDelay(500);
    }

    // Cache config structs
    config.parseNetConfig();
    config.parseNixieConfig();
    config.parseRTCconfig();

    config.configInit = true;

    // List contents of /config
    File dir = LITTLEFS.open("/config/");
    listFilesInDir(dir, 2);

    vTaskDelete(NULL);
}

void taskMonitorStatus(void* parameter) {
    while (!nixies.isReady) {
        vTaskDelay(500);
    }

    for (;;) {
        // Monitor WiFi status
        switch (WiFi.status()) {
            case WL_CONNECTION_LOST:
                config.sysStatus = 4;
                break;
            case WL_DISCONNECTED:
                config.sysStatus = 4;
                break;
            default:
                break;
        }

        vTaskDelay(1000);
    }
}

void taskShowStatus(void* parameter) {
    // Onboard LED (NOT the one in the ESP32 DS)
    ledcSetup(9, 100, 8);
    ledcAttachPin(ONBOARD_LED, 9);
    ledcWrite(9,255);

    while (!nixies.isReady) {
        vTaskDelay(500);
    }

    byte stepSize = 25;

    for (;;) {
        switch (config.sysStatus) {
            case 0:              // System nominal (dim onboard LED)
                ledcWrite(9,10);
                break;
            case 1: {            // Connecting to WiFi
                // Pulse tubes
                while (config.connecting) {
                    for (int i = 1; i <= 255; i++) {
                        nixies.setBrightness(3, i, true);
                        vTaskDelay(45);

                        i += stepSize;
                        if (!config.connecting) break;
                    }
                    for (int i = 255; i >= 1; i++) {
                        nixies.setBrightness(3, i, true);
                        vTaskDelay(45);
                        
                        i -= stepSize;
                        if (!config.connecting) break;
                    }
                }

                // Blink tubes
                if (!config.connectTimeout) {
                    int n[] = {1,1,1,1};
                    nixies.changeDisplay(n);

                    for (int i = 1; i <= TUBES_BLINK_AMOUNT; i++) {
                        nixies.setBrightness(3, 10, true);
                        ledcWrite(9,0);
                        vTaskDelay(150);

                        nixies.setBrightness(3, 175, true);
                        ledcWrite(9,255);
                        vTaskDelay(150);
                    }
                } else {
                    int n[] = {0,0,0,0};
                    nixies.changeDisplay(n);

                    // Blink tubes
                    for (int i = 1; i <= TUBES_BLINK_AMOUNT; i++) {
                        nixies.setBrightness(0, 10, true);
                        ledcWrite(9,0);
                        vTaskDelay(150);

                        nixies.setBrightness(0, 175, true);
                        ledcWrite(9,255);
                        vTaskDelay(150);
                    }                    
                }

                if (!rtc.RTCfault)
                    config.sysStatus = 0;
                    
                break;
            }

            case 2: {             // AP active
                int n[] = {0,0,0,0};
                nixies.changeDisplay(n);

                // Blink tubes
                for (int i = 1; i <= TUBES_BLINK_AMOUNT; i++) {
                    nixies.setBrightness(0, 10, true);
                    ledcWrite(9,0);
                    vTaskDelay(150);

                    nixies.setBrightness(0, 175, true);
                    ledcWrite(9,255);
                    vTaskDelay(150);
                }

                if (!rtc.RTCfault)
                    config.sysStatus = 0;

                break;
            }

            case 3: {            // Error
                // Blink tubes
                while (true) {
                    nixies.setBrightness(0, 10, true);
                    ledcWrite(9,0);
                    vTaskDelay(450);

                    nixies.setBrightness(0, 175, true);
                    ledcWrite(9,255);
                    vTaskDelay(450);

                    if (config.sysStatus != 3)
                        break;
                }

                break;
            }

            case 4: {             // WiFi disconnected
                while (true) {
                    nixies.setBrightness(0, 10, true);
                    ledcWrite(9,0);
                    vTaskDelay(900);

                    nixies.setBrightness(0, 175, true);
                    ledcWrite(9,255);
                    vTaskDelay(900);

                    if (config.sysStatus != 4)
                        break;
                }
            }
        }
        vTaskDelay(100);
    }
    vTaskDelay(200);
}

void taskSetupNixies(void* paramter) {
    nixies.isReady = false;

    Serial.println(F("[T] Nixie: Setting up nixies."));
    nixies.initialize(DS_PIN, ST_PIN, SH_PIN, 100);
    vTaskDelay(1000);

    // Set initial brightness
    nixies.setBrightness(0, 175, true);

    nixies.tumbleDisplay();
    config.nixieConfiguration.mode = 1;

    // Set initial display
    int n[] = {1, 2, 3, 4};
    nixies.changeDisplay(n);

    nixies.isReady = true;

    vTaskDelete(NULL);
}

void taskUpdateCaches(void* parameter) {
    while (!config.FSReady) {
        vTaskDelay(500);
    }

    for (;;) {
        config.parseNixieConfig();
        config.parseRTCconfig();

        vTaskDelay(CACHE_RENEWAL_INT);
    }
}

void taskSetupRTC(void* parameter) {
    rtc.RTCready = false;

    Serial.println(F("[i] RTC: Preparing RTC..."));
    if (!rtc.initialize()) {
        Serial.println(F("[X] RTC: Could not initialize."));

        rtc.RTCfault = true;
        config.sysStatus = 3;

    } else {
        vTaskDelay(25);
        
        Serial.println(F("[i] RTC: Checking module..."));
        switch (rtc.checkStartup()) {
            case 0:
                rtc.RTCready = true;
                Serial.println(F("[i] RTC: Ready; Module healthy."));
                break;
            case 1:
                // Startup OK, but oscillator stopped
                rtc.RTCready = true;
                Serial.printf("[i] RTC: Ready; Initial time set to: %s\n", __TIME__);
                break;
            case 2:
                // Error during I2C operation
                Serial.println(F("[X] RTC: Failure; Module error."));
                rtc.RTCfault = true;
                break;
        }
    }    
    vTaskDelete(NULL);
}

void taskUpdateNixies(void* parameter) {
    // Stall until RTC is ready
    while (!rtc.RTCready) {
        Serial.println("[i] Nixie: Awaiting RTC...");
        vTaskDelay(500);

        if (rtc.RTCfault)
            vTaskDelete(NULL);
    }

    int n1; // Hour
    int n2; // Hour
    int n3; // Minute
    int n4; // Minute

    // Artificial delay to ensure correct RTC readings
    vTaskDelay(1000);

    int lastHour = rtc.getTime(2);
    Serial.printf("[i] Nixie: lastHour set to: %d.\n", lastHour);

    while (!nixies.isReady || config.sysStatus != 0) {
        vTaskDelay(500);
    }

    Serial.println("[i] Nixie: Now entering update routine.");
    for (;;) {
        // Skip sequence if tubes are tumbling
        if (!nixies.isTumbling) {
           // Obtain RTC time
            //int s = rtc.getTime(0);
            int m = rtc.getTime(1);
            int h = rtc.getTime(2);

            bool timeValid = true;

            // Sanity checks
            if (h > 24 || m >= 60)                      timeValid = false;
            if (lastHour - h > 4 || lastHour - h < -4)  timeValid = false;

            #ifdef DEBUG_VERBOSE
                Serial.printf("[i] Nixie: lastHour: '%d', timeValid: %d.\n", lastHour, timeValid);
            #endif

            // Cathode depoisoning
            if (nixies.forceUpdate || (timeValid && lastHour != h)) {
                #ifdef DEBUG
                    if (nixies.forceUpdate) {
                        Serial.println("[i] Nixie: Update enforced.");
                    }

                    Serial.printf("[i] Nixie: Hour change: %d -> %d\n", lastHour, h);
                #endif

                // Update RTC time once more
                m = rtc.getTime(1);
                h = rtc.getTime(2);

                // Sanity check
                timeValid = true;
                if (h > 24 || m >= 60)                      timeValid = false;
                if (lastHour - h > 4 || lastHour - h < -4)  timeValid = false;

                if (timeValid) {
                    lastHour = h;
                }
                #ifdef DEBUG
                    else {
                        Serial.println("[X] Nixie: Time not valid; Will not set lastHour.");
                    }
                #endif

                if (!nixies.isTumbling) 
                    config.nixieConfiguration.tumble = true;
            }
            
            // Brightness updater
            if (config.sysStatus == 0) {
                int pwm = config.nixieConfiguration.brightness;

                #ifdef DEBUG_VARIOUS
                    Serial.printf("[i] Nixie: Updating brightness, PWM: %d\n", pwm);
                #endif

                // Turn anode off if tube is off (has a number higher than 9, making the BCD decoder pulling all cathodes low).
                if (nixies.t1 > 9)  { nixies.setBrightness(0, 0, false); }
                else                { nixies.setBrightness(0, pwm, false); }

                if (nixies.t2 > 9)  { nixies.setBrightness(1, 0, false); }
                else                { nixies.setBrightness(1, pwm, false); }

                if (nixies.t3 > 9)  { nixies.setBrightness(2, 0, false); }
                else                { nixies.setBrightness(2, pwm, false); }

                if (nixies.t4 > 9)  { nixies.setBrightness(3, 0, false); }
                else                { nixies.setBrightness(3, pwm, false); }
            }

            // Update according to mode
            if (config.nixieConfiguration.tumble) {
                #ifdef DEBUG
                    Serial.println("[i] Nixie: Tumbling display due to mode.");
                #endif

                nixies.tumbleDisplay();
                config.nixieConfiguration.tumble = false;
            } else {
                // Determine mode
                // (Modes are described in struct 'NixieConfig')
                #ifdef DEBUG
                //    Serial.printf("[i] Nixie: Mode is: %d.\n", config.nixieConfiguration.mode);
                #endif

                switch (config.nixieConfiguration.mode) {
                    case 1:
                        #ifdef DEBUG
                            //Serial.printf("[i] Nixie: Time is: %d:%d:%d\n", h, m, s);
                        #endif

                        #ifdef DEBUG_VERBOSE
                            Serial.printf("[i] Nixie: RTC time: %d:%d.\n", h, m);
                        #endif

                        if (timeValid || nixies.forceUpdate) {
                            // Update display
                            // Hours
                            if (h < 9) {
                                n1 = 0;
                                n2 = h;
                            } else {
                                n1 = h / 10;
                                n2 = h % 10;
                            }
                            
                            // Minutes
                            if (m < 9) {
                                n3 = 0;
                                n4 = m;
                            } else {
                                n3 = m / 10;
                                n4 = m % 10;
                            }

                            int n[] = {n1, n2, n3, n4};
                            nixies.changeDisplay(n);
                        }

                        break;
                    case 2:    
                        int n[] = { config.nixieConfiguration.nNum1,
                                    config.nixieConfiguration.nNum2,
                                    config.nixieConfiguration.nNum3,
                                    config.nixieConfiguration.nNum4};
                        nixies.changeDisplay(n);
                        break;
                }
            }

            // Reset nixies.forceUpdate flag
            if (nixies.forceUpdate)
                nixies.forceUpdate = false;
        }
        
        vTaskDelay(500);
    }
}

void taskSetupNetwork(void* parameter) {
    while (!config.configInit) {
        vTaskDelay(500);
    }

    config.connectTimeout = false;
    config.connecting = false;

    if (config.netConfiguration.WiFiClient) {
        config.sysStatus = 1;
        Serial.printf("[i] Network: Connecting to: %s\n", config.netConfiguration.WiFi_SSID);
        config.connecting = true;

        WiFi.begin(config.netConfiguration.WiFi_SSID, config.netConfiguration.WiFi_PSK);
        int i = 0;

        while (WiFi.status() != WL_CONNECTED) {
            if (i >= 15) {
                Serial.println(F("[X] Network: Connection timed out."));
                config.connectTimeout = true;
                config.connecting = false;

                break;
            }

            Serial.println(F("[i] Network: Connecting..."));

            vTaskDelay(500);
            i++;
        }

        config.connecting = false;
        config.netReady = true;
    }

    if (config.connectTimeout || !config.netConfiguration.WiFiClient) {
        config.sysStatus = 2;
        const char* AP_SSID = "ESP32 - Nixie clock";
        const char* AP_PSK  = "NixieClock2021";

        Serial.printf("[i] Network: Starting AP with SSID: '%s' (PSK: '%s')\n", AP_SSID, AP_PSK);
        WiFi.softAP(AP_SSID, AP_PSK);
        
        config.isAP = true;
        config.netReady = true;
    }

    vTaskDelay(200);

    if (config.netReady) {        
        // Init mDNS
        if(!MDNS.begin("nixie-clock")) {
            Serial.println("[X] Network: Error starting mDNS.");
        }

        Serial.print("[i] Network: Ready! IP: ");
        if (config.isAP) {
            IPAddress IP = WiFi.softAPIP();
            Serial.println(IP);
        } else {
            Serial.println(WiFi.localIP());
        }
    }

    vTaskDelete(NULL);
}

void taskSetupWebserver(void* parameter) {
    while (!config.netReady) {
        vTaskDelay(500);
    }

    // Spawn webserver handlers
    webServerStaticContent();
    webServerAPIs();
    webServerInitWS();

    vTaskDelete(NULL);
}

void taskUpdateRTC(void* parameter) {
    while (!config.netReady) {
        vTaskDelay(500);
    }
    
    #ifdef DEBUG_VERBOSE
        Serial.print(F("[i] RTC: tzOffset set at: "));
            Serial.println(config.rtcConfiguration.tzOffset);
    #endif

    // Create NTPClient instance
    config.parseRTCconfig();

    if (config.rtcConfiguration.isDST) {
        config.rtcConfiguration.tzOffset += 3600;
    }

    #ifdef DEBUG_VERBOSE
        Serial.print(F("[i] RTC: DST active, tzOffset is now: "));
            Serial.println(config.rtcConfiguration.tzOffset);
    #endif

    NTPClient timeClient(ntpUDP, config.rtcConfiguration.ntpSource, config.rtcConfiguration.tzOffset);
    
    //unsigned long ntpEpoch;
    int ntpHours;
    int ntpMinutes;
    int ntpSeconds;

    for (;;) {
        config.parseRTCconfig();

        // Update from NTP if applicable
        #ifdef DEBUG
            Serial.print("[i] RTC: isNTP set to: ");
            Serial.println(config.rtcConfiguration.isNTP);
        #endif

        if (config.rtcConfiguration.isNTP) {
            if (!config.isAP && config.netConfiguration.WiFiClient) {
                #ifdef DEBUG_VERBOSE
                    Serial.print("[i] RTC: Using NTP source: ");
                    Serial.println(config.rtcConfiguration.ntpSource);
                #endif

                timeClient.begin();
                timeClient.update();

                //ntpEpoch = timeClient.getEpochTime();
                ntpHours = timeClient.getHours();
                ntpMinutes = timeClient.getMinutes();
                ntpSeconds = timeClient.getSeconds();

                // Resolve discrepancies
                int rtcH = rtc.getTime(2);
                int rtcM = rtc.getTime(1);
                int rtcS = rtc.getTime(0);

                // Sanity checks
                bool rtcTimeValid = true;
                if (rtcH > 24 || rtcM > 59) rtcTimeValid = false;
                
                if (rtcTimeValid) {
                    if (rtcH != ntpHours) {
                        nixies.forceUpdate = true;
                        Serial.printf("[i] RTC: Updating hours from NTP: %d > %d\n", rtcH, ntpHours);
                        rtc.setTime(2, ntpHours);
                    }
                    if (rtcM != ntpMinutes) {
                        //nixies.forceUpdate = true;
                        Serial.printf("[i] RTC: Updating minutes from NTP: %d > %d\n", rtcM, ntpMinutes);
                        rtc.setTime(1, ntpMinutes);
                    }
                    if (rtcS != ntpSeconds) {
                        //nixies.forceUpdate = true;
                        Serial.printf("[i] RTC: Updating seconds from NTP: %d > %d\n", rtcS, ntpSeconds);
                        rtc.setTime(0, ntpSeconds);
                    }
                }

            } else {
                Serial.println("[i] NTP: Cannot update in AP mode.");
            }
        } else {
            Serial.println("[i] NTP: Disabled, will not update.");
        }

        vTaskDelay(5000);
    }
}