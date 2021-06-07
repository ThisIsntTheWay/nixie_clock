/*
    ESP32 Nixie Clock - Nixie module
    (c) V. Klopfenstein, 2021

    Code for addressing nixie tubes.
*/

#include <system/nixie.h>

// Bitshift pins
#define DS_PIN  27   // Data
#define SH_CP   26   // Clock
#define ST_CP   25   // Latch

extern Nixie nixie;

Nixie::Nixie() {
}

void Nixie::setup() {
    // Shift registers
    pinMode(DS_PIN, OUTPUT);
    pinMode(SH_CP, OUTPUT);
    pinMode(ST_CP, OUTPUT);

    this->clear();
    this->setOptos();

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
}

void Nixie::clear() {
    // Clear nixies
    digitalWrite(ST_CP, LOW);
    shiftOut(DS_PIN, SH_CP, MSBFIRST, 0b11111111);
    shiftOut(DS_PIN, SH_CP, MSBFIRST, 0b11111111);
    digitalWrite(ST_CP, HIGH);
}

void Nixie::update() {
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
}

void Nixie::updateBrightness(int b) {
    ledcWrite(1, b);
}

void Nixie::setOptos() {
    // Optoisolator IR LEDs
    int opto1 = 2;
    int opto2 = 15;
    int opto3 = 4;
    int opto4 = 5;

    pinMode(opto1, OUTPUT);
    pinMode(opto2, OUTPUT);
    pinMode(opto3, OUTPUT);
    pinMode(opto4, OUTPUT);

    // Prepare PWM
    ledcSetup(1, 200, 8);       // Channel ID, Frequency (Hz), Resolution (in bit)
    ledcAttachPin(opto1, 1);
    ledcAttachPin(opto2, 1);
    ledcAttachPin(opto3, 1);
    ledcAttachPin(opto4, 1);
}

// Convert dec to BCD
byte Nixie::decToBCD(byte in) {
    return( (in/16*10) + (in%16) );

}

// The following function was inspired by:
// https://forum.arduino.cc/index.php?topic=449828.msg3094698#msg3094698
void Nixie::displayNumber(int number_1, int number_2, int number_3, int number_4) {
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

int Nixie::displayCryptoPrice(String ticker, String quote) {
    HTTPClient httpNIXIE;

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
    } else {
        Serial.println("[X] Crypto: Connection to the server failed.");
    }

    return 0;
}

String Nixie::parseNixieConfig(int mode) {
    // Read file
    File nixieCfgFile = LITTLEFS.open(F("/config/nixieConfig.json"), "r");

    // Parse JSON
    StaticJsonDocument<250> cfgNixie;

    // > Deserialize
    DeserializationError error = deserializeJson(cfgNixie, nixieCfgFile);
    if (error) {
        String err = error.c_str();

        Serial.print("[X] Nixie parser: Deserialization fault: "); Serial.println(err);
        return "[Deserialization fault: " + err + "]";
    } else {
        // Populate config struct
        strlcpy(nixieConfig.crypto_asset, cfgNixie["crypto_asset"], sizeof(nixieConfig.crypto_asset));
        strlcpy(nixieConfig.crypto_quote, cfgNixie["crypto_quote"], sizeof(nixieConfig.crypto_quote));
        nixieConfig.cathodeDepoisonTime = cfgNixie["cathodeDepoisonTime"];
        nixieConfig.cathodeDepoisonMode = cfgNixie["cathodeDepoisonMode"];
        nixieConfig.cathodeDepoisonInterval = cfgNixie["cathodeDepoisonInterval"];
        nixieConfig.anodePWM = cfgNixie["anodePWM"];
        
        nixieCfgFile.close();

        switch (mode) {
            case 1: return String(nixieConfig.crypto_asset) + "/" + String(nixieConfig.crypto_quote); break;
            case 2: return String(nixieConfig.cathodeDepoisonTime); break;
            case 3: return String(nixieConfig.cathodeDepoisonMode); break;
            case 4: return String(nixieConfig.cathodeDepoisonInterval); break;
            case 5: return String(nixieConfig.anodePWM); break;
            case 6: return String((nixieConfig.anodePWM * 100) / 255); break;   //anodePWM as a % of 255
            default: return "[NIXIE: unknown mode]";
        }
    }

    return String();
}