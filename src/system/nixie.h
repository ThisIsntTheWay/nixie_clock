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
#define DS_PIN 19   // Latch | ST_CP
#define SH_CP 18    // Clock | SH_CP
#define ST_CP 23    // Data  | DS

byte data;

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
    
    n1 = decToBCD(number_1);
    n2 = decToBCD(number_2);
    n3 = decToBCD(number_3);
    n4 = decToBCD(number_4);

    // Push to shift registers
    digitalWrite(DS_PIN, LOW);
    shiftOut(ST_CP, SH_CP, LSBFIRST, (n1 << 4) | n2);
    shiftOut(ST_CP, SH_CP, LSBFIRST, (n3 << 4) | n4);
    digitalWrite(DS_PIN, HIGH);
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

    while (!RTCready) { vTaskDelay(1000); }
    DateTime rtcDT = rtc.now();

    int lastMinute = rtcDT.minute();
    int lastHour = rtcDT.hour();
    
    Serial.println("[T] Nixie: Spawning nixie updater...");
    for (;;) {
        DateTime rtcDT = rtc.now();
        
        // Periodically display time
        if (lastMinute != rtcDT.minute()) {
            Serial.print("[T] Nixie: Updating minutes. ");
            Serial.print(lastMinute); Serial.print(" > "); Serial.println(rtcDT.minute());

            lastMinute = rtcDT.minute();
            displayNumber(rtcDT.hour(), rtcDT.minute(), 0, 0);
        }
        if (lastHour != rtcDT.hour()) {
            Serial.print("[T] Nixie: Updating hours. ");
            Serial.print(lastHour); Serial.print(" > "); Serial.println(rtcDT.hour());
            
            lastHour = rtcDT.hour();
            displayNumber(rtcDT.hour(), rtcDT.minute(), 0, 0);
        }

        vTaskDelay(1500);
    }
}

#endif