/*
    ESP32 Nixie Clock - Nixie module
    (c) V. Klopfenstein, 2021

    All tasks/functions in here are related to nixie tube and bitshift operations.
    It...
     - Prepares and reads JSON configuration files related to nixie operations.
     - Regularely updates the nixie tubes.
*/

#include <arduino.h>
#include <system/rtc.h>
#ifndef nixie_h
#define nixie_h

//  ---------------------
//  VARIABLES
//  ---------------------

// Bitshift pins
#define DS_PIN  27   // Data
#define SH_CP   26   // Clock
#define ST_CP   25   // Latch

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

// Instances
RTC_DS3231 rtcNixie;
HTTPClient httpNIXIE;

// Structs
struct nixieConfigStruct {
    char crypto_asset[8];
    char crypto_quote[8];
    int cathodeDepoisonTime;
    int cathodeDepoisonMode;
    int cathodeDepoisonInterval;
    int anodePWM;
};

struct nixieConfigStruct nixieConfigJSON;

//  ---------------------
//  FUNCTIONS
//  ---------------------

// Convert dec to BCD
byte decToBCD(byte in) {
    return( (in/16*10) + (in%16) );
}

// The following function was inspired by:
// https://forum.arduino.cc/index.php?topic=449828.msg3094698#msg3094698
void displayNumber(int number_1, int number_2, int number_3, int number_4) {
    byte n1, n2, n3, n4;

    // Save digits in global vars
    if (number_1 < 10) { tube1Digit = number_1+'0'; } else { tube1Digit = 'X'; };
    if (number_2 < 10) { tube2Digit = number_2+'0'; } else { tube2Digit = 'X'; };
    if (number_3 < 10) { tube3Digit = number_3+'0'; } else { tube3Digit = 'X'; };
    if (number_4 < 10) { tube4Digit = number_4+'0'; } else { tube4Digit = 'X'; };

    n1 = decToBCD(number_1);
    n2 = decToBCD(number_2);
    n3 = decToBCD(number_3);
    n4 = decToBCD(number_4);

    // Push to shift registers
    digitalWrite(ST_CP, LOW);
    shiftOut(DS_PIN, SH_CP, MSBFIRST, (n1 << 4) | n2);
    shiftOut(DS_PIN, SH_CP, MSBFIRST, (n3 << 4) | n4);
    digitalWrite(ST_CP, HIGH);
}

int displayCryptoPrice(String ticker, String quote) {
    if (!WiFiReady)
        return 0;

    // Obtain ticker price from binance
    httpNIXIE.useHTTP10(true);
    httpNIXIE.begin("https://api.binance.com/api/v3/ticker/price?symbol=" + ticker + quote);

    httpNIXIE.GET();

    if (httpNIXIE.connected()) {
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, httpNIXIE.getStream());
        
        httpNIXIE.end();

        if (error) {
            Serial.print(F("[X] Crypto: Could not deserialize JSON: "));
            Serial.println(error.c_str());

            return 0;
        } else {
            if (doc.containsKey("error"))
                return 0;

            int price = doc["price"];

            // Trim price if it is greater than 10'000$
            if (price > 9999) price = price * 0.1;

            // Split price into four digits
            int p1 = 10;
            int p2 = 10;
            int p3 = 10;
            int p4 = 10;

            if (price > 999) {              // 1000
                p1 = (price / 1000) % 10;
                p2 = (price / 100) % 10;
                p3 = (price / 10) % 10;
                p4 = price % 10;        
            } else if (price > 99) {        // 100
                p2 = (price / 100) % 10;
                p3 = (price / 10) % 10;
                p4 = price % 10;
            } else if (price > 9) {         // 10
                p3 = price / 10;
                p4 = price % 10;
            } else {                        // 1
                p4 = price;
            }
            
            displayNumber(p1, p2, p3, p4);

            return price;
        }
    }

    return 0;
}

String parseNixieConfig(int mode) {
    // Read file
    File nixieConfig = LITTLEFS.open(F("/config/nixieConfig.json"), "r");

    // Parse JSON
    StaticJsonDocument<250> cfgNixie;

    // > Deserialize
    DeserializationError error = deserializeJson(cfgNixie, nixieConfig);
    if (error) {
        String err = error.c_str();

        Serial.print("[X] Nixie parser: Deserialization fault: "); Serial.println(err);
        return "[Deserialization fault: " + err + "]";
    } else {
        // Populate config struct
        strlcpy(nixieConfigJSON.crypto_asset, cfgNixie["crypto_asset"], sizeof(nixieConfigJSON.crypto_asset));
        strlcpy(nixieConfigJSON.crypto_quote, cfgNixie["crypto_quote"], sizeof(nixieConfigJSON.crypto_quote));
        nixieConfigJSON.cathodeDepoisonTime = cfgNixie["cathodeDepoisonTime"];
        nixieConfigJSON.cathodeDepoisonMode = cfgNixie["cathodeDepoisonMode"];
        nixieConfigJSON.cathodeDepoisonInterval = cfgNixie["cathodeDepoisonInterval"];
        nixieConfigJSON.anodePWM = cfgNixie["anodePWM"];

        switch (mode) {
            case 1: return String(nixieConfigJSON.crypto_asset) + "/" + String(nixieConfigJSON.crypto_quote); break;
            case 2: return String(nixieConfigJSON.cathodeDepoisonTime); break;
            case 3: return String(nixieConfigJSON.cathodeDepoisonMode); break;
            case 4: return String(nixieConfigJSON.cathodeDepoisonInterval); break;
            case 5: return String(nixieConfigJSON.anodePWM); break;
            case 6: return String((nixieConfigJSON.anodePWM * 100) / 255); break;   //anodePWM as a % of 255
            default: return "[NIXIE: unknown mode]";
        }
    }
    
    nixieConfig.close();

    return String();
}

//  ---------------------
//  TASKS
//  ---------------------

void taskSetupNixie(void* parameter) {
    while (!FlashFSready) { vTaskDelay(500); }

    if (!(LITTLEFS.exists("/config/nixieConfig.json"))) {
        Serial.println(F("[T] Nixie: No config found."));
        
        if (!LITTLEFS.exists("/config"))
            LITTLEFS.mkdir("/config");

        File nixieConfig = LITTLEFS.open(F("/config/nixieConfig.json"), "w");

        // Construct JSON
        StaticJsonDocument<200> cfgNixie;

        cfgNixie["crypto_asset"] = "BTC";
        cfgNixie["crypto_quote"] = "USD";
        cfgNixie["cathodeDepoisonTime"] = 600;
        cfgNixie["cathodeDepoisonMode"] = 1;
        cfgNixie["anodePWM"] = 255;

        // Write rtcConfig.cfg
        if (!(serializeJson(cfgNixie, nixieConfig)))
            Serial.println(F("[X] Nixie: config write failure."));

        nixieConfig.close();

        // Set time on RTC
        Serial.println(F("[>] Nixie: Config created."));
    } else {
        Serial.println(F("[T] Nixie: Config found."));
    }

    nixieSetupComplete = true;
    vTaskDelete(NULL);
}

void taskUpdateNixie(void* parameter) {
    Serial.println("[T] Nixie: preparing nixie updater...");

    while (!RTCready) { vTaskDelay(1000); }
    Serial.println("[T] Nixie: RTC ready.");

    int hour = 0;
    int minute = 0;
    int lastMinute = 0;
    int lastHour = 0;

    // Cathode depoisoning stuff
    bool justCycled = false;
    bool timeIsValid;
    cycleNixies = true;

    Serial.println("[T] Nixie: Starting nixie updater...");
    for (;;) {
        // Cache nixie config
        parseNixieConfig(9);

        DateTime rtcDT = rtcNixie.now();

        timeIsValid = true;

        // Save hour and minute as variables in order to have consistent data for further manipulation
        hour = rtcDT.hour();
        minute = rtcDT.minute();
        
        // Verify time and account for bad RTC data
        if (hour > 23) {
            timeIsValid = false;
            hour = 0;
        }
        if (minute > 59){
            timeIsValid = false;
            minute = 0;
        }

        // Check if cathode depoisoning should occur
        if (!justCycled && timeIsValid) {
            switch (nixieConfigJSON.cathodeDepoisonMode) {
                case 1: // On hour change
                    if (lastHour != hour) cycleNixies = true;
                    break;
                case 2: // On schedule
                    if (timeIsValid) {
                        int nowTime = ((hour * 100) + minute);                    
                    }
                    break;
                default:
                    cycleNixies = false;                
            }
        }

        // Check if nixies should update manually or automatically
        if (!cycleNixies && nixieAutonomous) {
            // if lastHour and/or lastMinute are invalid, enforce update because the RTC must have stored invalid data
            if (lastHour > 24 || lastMinute > 60) forceUpdate = true;

            // Update nixies if numbers are valid
            if (timeIsValid || forceUpdate) {
                // Reset cathode depoisoning flag
                justCycled = false;

                // Split numbers
                int hourD1   = hour / 10;
                int hourD2   = hour % 10;
                int minuteD1 = minute / 10;
                int minuteD2 = minute % 10;
                
                // Periodically display time
                if (lastMinute != rtcDT.minute() || lastHour != rtcDT.hour() || forceUpdate) {
                    if (timeIsValid) {
                        Serial.println("[T] Nixie: Updating time:");
                        Serial.print(" > Minutes: "); Serial.print(lastMinute); Serial.print(" > "); Serial.println(rtcDT.minute());
                        Serial.print(" > Hours: "); Serial.print(lastHour); Serial.print(" > "); Serial.println(rtcDT.hour());
                        Serial.print(" > Epoch: "); Serial.println(rtcDT.unixtime());
                    } else {
                        // Use NTP time as RTC is providing bad data.
                        if (!APmode && NTPisValid) {
                            NTPClient timeTMP(ntpUDP, config.NTP, config.GMT + config.DST);
                            timeTMP.begin();

                            if (timeTMP.forceUpdate()) {
                                Serial.println("[T] Nixie: Using NTP time.");

                                long ntpTime = timeTMP.getEpochTime();
                                rtcNixie.adjust(DateTime(ntpTime));
                            }

                            timeTMP.end();
                        }
                    }

                    // Update lastX values
                    if (lastHour != rtcDT.hour()) {
                        lastHour = rtcDT.hour();
                    }

                    lastMinute = rtcDT.minute();

                    displayNumber(hourD1, hourD2, minuteD1, minuteD2);

                    // Revert force update
                    if (forceUpdate) forceUpdate = false;
                }
            }
        } else if (cycleNixies && !justCycled) {
            Serial.println("[T] Nixie: Cycling nixies...");

            // Cycle all nixies, first incrementing then decrementing them
            for (int i = 0; i < 10; i++) {
                displayNumber(i,i,i,i);
                vTaskDelay(50);
            }

            for (int i = 9; i > -1; i--) {
                displayNumber(i,i,i,i);
                vTaskDelay(50);
            }

            // Reset nixies
            nixieAutonomous = true;
            cycleNixies = false;
            forceUpdate = true;
            justCycled = true;
        } else if (crypto && !cycleNixies) {
            Serial.println("[T] Nixie: Displaying crypto price...");
            displayCryptoPrice(String(nixieConfigJSON.crypto_asset), String(nixieConfigJSON.crypto_quote));
        }

        vTaskDelay(400);
    }
}

void taskUpdateNixieBrightness(void* parameter) {
    // Wait for LittleFS and taskSetupNixie to be ready
    while (!nixieSetupComplete) { vTaskDelay(500); }

    // Cache nixieConfig parser
    parseNixieConfig(5);

    // Opto-Isolator stuff
    int opto1 = 5;
    int opto2 = 4;
    int opto3 = 2;
    int opto4 = 15;
    pinMode(opto1, OUTPUT);
    pinMode(opto2, OUTPUT);
    pinMode(opto3, OUTPUT);
    pinMode(opto4, OUTPUT);

    // Create and assign LED controllers to each opto-isolator channel
    int pwmFreq = 100;
    ledcSetup(1, pwmFreq, 8);
        ledcAttachPin(opto1, 1);

    ledcSetup(2, pwmFreq, 8);
        ledcAttachPin(opto2, 2);
    
    ledcSetup(3, pwmFreq, 8);
        ledcAttachPin(opto3, 3);
    
    ledcSetup(4, pwmFreq, 8);
        ledcAttachPin(opto4, 4);

    Serial.println("[T] Nixie: Starting brightness updater...");
    for (;;) {
        // Set brightness of nixies
        int pwm = nixieConfigJSON.anodePWM;

        // Check if the tubes are actually displaying stuff, otherwise turn them off.
        // This prevents the anode from floating.
        if (tube1Digit == 'X') { ledcWrite(1, 0); } else { ledcWrite(1, pwm); }
        if (tube2Digit == 'X') { ledcWrite(2, 0); } else { ledcWrite(2, pwm); }
        if (tube3Digit == 'X') { ledcWrite(3, 0); } else { ledcWrite(3, pwm); }
        if (tube4Digit == 'X') { ledcWrite(4, 0); } else { ledcWrite(4, pwm); }

        vTaskDelay(500);
    }
}

#endif