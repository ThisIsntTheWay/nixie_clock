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

bool nixieAutonomous = true;
bool cycleNixies = false;
bool crypto = false;

// Nixie digits
char tube1Digit = 0;
char tube2Digit = 0;
char tube3Digit = 0;
char tube4Digit = 0;

// Temp sotrage for nixie digits
int oldDigit1;
int oldDigit2;
int oldDigit3;
int oldDigit4;

// Structs
struct nixieConfigStruct {
    char crypto_asset[8];
    char crypto_quote[8];
    int cathodeDepoisonTime;
    int cathodeDepoisonMode;
    int cathodeDepoisonInterval;
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
    shiftOut(DS_PIN, SH_CP, MSBFIRST, (n3 << 4) | n4);
    shiftOut(DS_PIN, SH_CP, MSBFIRST, (n1 << 4) | n2);
    digitalWrite(ST_CP, HIGH);
}

int getCryptoPrice(String ticker, String quote) {
    if (!WiFiReady)
        return 0;

    // Obtain ticker price from binance
    http.useHTTP10(true);
    http.begin("https://api.binance.com/api/v3/ticker/price?symbol=" + ticker + quote);
    http.GET();

    StaticJsonDocument<100> doc;
    deserializeJson(doc, http.getStream());
    http.end();
    
    if (doc.containsKey("error"))
        return 0;

    int price = doc["price"];

    return doc["price"];
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
        
        nixieConfig.close();

        switch (mode) {
            case 1: return String(String(nixieConfigJSON.crypto_asset) + "/" + String(nixieConfigJSON.crypto_quote)); break;
            case 2: return String(nixieConfigJSON.cathodeDepoisonTime); break;
            case 3: return String(nixieConfigJSON.cathodeDepoisonMode); break;
            case 4: return String(nixieConfigJSON.cathodeDepoisonInterval); break;
            default: return "[NIXIE: unknown mode]";
        }
    }

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

        // Write rtcConfig.cfg
        if (!(serializeJson(cfgNixie, nixieConfig)))
            Serial.println(F("[X] Nixie: config write failure."));

        nixieConfig.close();

        // Set time on RTC
        Serial.println(F("[>] Nixie: Config created."));
    } else {
        Serial.println(F("[T] Nixie: Config found."));
    }

    vTaskDelete(NULL);
}

void taskUpdateNixie(void* parameter) {
    Serial.println("[T] Nixie: preparing nixie updater...");

    while (!RTCready) { vTaskDelay(1000); }
    Serial.println("[T] Nixie: RTC ready.");

    int lastMinute = 0;
    int lastHour = 0;

    bool forceUpdate = true;
    cycleNixies = true;

    Serial.println("[T] Nixie: Starting nixie updater...");
    for (;;) {

        // Check if nixies should update manually or automatically
        if (nixieAutonomous && !cycleNixies) {
            DateTime rtcDT = rtc.now();

            bool timeIsValid = true;

            // Save hour and minute as variables in order to have consistent data for further manipulation
            int hour = rtcDT.hour();
            int minute = rtcDT.minute();
            //Serial.print("hour: "); Serial.println(hour);
            //Serial.print("minute: "); Serial.println(minute);

            // Verify time
            //Serial.printf("[i] hour: %d\n", hour);
            if (hour > 23 || minute > 59) timeIsValid = false;
            if ((lastHour - hour) > 5 || (lastHour - hour) < -5) timeIsValid = false;

            // Update nixies if valid numbers are valid
            if (timeIsValid || forceUpdate) {
                // Split numbers
                int hourD1   = hour / 10;
                int hourD2   = hour % 10;
                int minuteD1 = minute / 10;
                int minuteD2 = minute % 10;
                
                // Periodically display time
                if (lastMinute != rtcDT.minute() || lastHour != rtcDT.hour() || forceUpdate) {
                    Serial.println("[T] Nixie: Updating time:");
                    Serial.print(" > Minutes: "); Serial.print(lastMinute); Serial.print(" > "); Serial.println(rtcDT.minute());
                    Serial.print(" > Hours: "); Serial.print(lastHour); Serial.print(" > "); Serial.println(rtcDT.hour());
                    Serial.print(" > Epoch: "); Serial.println(rtcDT.unixtime());

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
        } else if (cycleNixies) {
            Serial.println("[T] Nixie: Cycling nixies...");

            // Cycle all nixies, first incrementing then decrementing them
            for (int i = 0; i < 10; i++) {
                displayNumber(i,i,i,i);
                vTaskDelay(65);
            }

            for (int i = 9; i > -1; i--) {
                displayNumber(i,i,i,i);
                vTaskDelay(65);
            }

            // Reset nixies
            nixieAutonomous = true;
            cycleNixies = false;
            forceUpdate = true;
        }

        vTaskDelay(400);
    }
}

#endif