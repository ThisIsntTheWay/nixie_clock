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

    Serial.print("NUM INGRESS: ");
        Serial.print(number_1);
        Serial.print(" ");
        Serial.print(number_2);
        Serial.print(" : ");
        Serial.print(number_3);
        Serial.print(" ");
        Serial.println(number_4);

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

    return doc["price"];
}

//  ---------------------
//  TASKS
//  ---------------------

void taskUpdateNixie(void* parameter) {
    Serial.println("[T] Nixie: preparing nixie updater...");
    pinMode(DS_PIN, OUTPUT);
    pinMode(SH_CP, OUTPUT);
    pinMode(ST_CP, OUTPUT);

    while (!RTCready) { vTaskDelay(1000); }
    Serial.println("[T] Nixie: RTC ready.");

    DateTime rtcDT = rtc.now();

    int lastMinute = rtcDT.minute();
    int lastHour = rtcDT.hour();

    Serial.print("lastHour: "); Serial.println(lastHour);
    Serial.print("lastMinute: "); Serial.println(lastMinute);
    
    Serial.println("[T] Nixie: Starting ning nixie updater...");
    for (;;) {
        // Check if nixies should update manually or automatically
        if (nixieAutonomous) {

            DateTime rtcDT = rtc.now();

            bool timeIsValid = true;

            // Save hour and minute as variables in order to have consistent data for further manipulation
            int hour = rtcDT.hour();
            int minute = rtcDT.minute();
            //Serial.print("hour: "); Serial.println(hour);
            //Serial.print("minute: "); Serial.println(minute);

            // Verify numbers
            if (hour > 23 || minute > 59)
                timeIsValid = false;

            // Update nixies if valid numbers are valid
            if (timeIsValid) {
                // Split numbers
                int hourD1   = hour / 10;
                int hourD2   = hour % 10;
                int minuteD1 = minute / 10;
                int minuteD2 = minute % 10;
                
                // Periodically display time
                if (lastMinute != rtcDT.minute()) {
                    Serial.println("[T] Nixie: Updating time:");
                    Serial.print(" > Minutes: "); Serial.print(lastMinute); Serial.print(" > "); Serial.println(rtcDT.minute());
                    Serial.print(" > Hours: "); Serial.print(lastHour); Serial.print(" > "); Serial.println(rtcDT.hour());
                    Serial.print(" > Epoch: "); Serial.println(rtcDT.unixtime());

                    // Update last values
                    if (lastHour != rtcDT.hour()) {
                        lastHour = rtcDT.hour();
                    }
                    lastMinute = rtcDT.minute();

                    displayNumber(hourD1, hourD2, minuteD1, minuteD2);
                }
            }
        }

        vTaskDelay(500);
    }
}

#endif