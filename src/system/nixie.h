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

// The following function was inspired by:
// https://forum.arduino.cc/index.php?topic=449828.msg3094698#msg3094698
void displayNumber(int number_1, int number_2) {//, int number_3, int number_4) {
    byte n1, n2, n3, n4;

    switch (number_1) {
        case 0: n1 = 0b0000; break;
        case 1: n1 = 0b1000; break;
        case 2: n1 = 0b0100; break;
        case 3: n1 = 0b1100; break;
        case 4: n1 = 0b0010; break;
        case 5: n1 = 0b1010; break;
        case 6: n1 = 0b0110; break;
        case 7: n1 = 0b1110; break;
        case 8: n1 = 0b0001; break;
        case 9: n1 = 0b1001; break;
    }

    switch (number_2) {
        case 0: n2 = 0b0000; break;
        case 1: n2 = 0b1000; break;
        case 2: n2 = 0b0100; break;
        case 3: n2 = 0b1100; break;
        case 4: n2 = 0b0010; break;
        case 5: n2 = 0b1010; break;
        case 6: n2 = 0b0110; break;
        case 7: n2 = 0b1110; break;
        case 8: n2 = 0b0001; break;
        case 9: n2 = 0b1001; break;
    }

    /*switch (number_3) {
        case 0: n3 = 0b0000; break;
        case 1: n3 = 0b1000; break;
        case 2: n3 = 0b0100; break;
        case 3: n3 = 0b1100; break;
        case 4: n3 = 0b0010; break;
        case 5: n3 = 0b1010; break;
        case 6: n3 = 0b0110; break;
        case 7: n3 = 0b1110; break;
        case 8: n3 = 0b0001; break;
        case 9: n3 = 0b1001; break;
    }

    switch (number_4) {
        case 0: n4 = 0b0000; break;
        case 1: n4 = 0b1000; break;
        case 2: n4 = 0b0100; break;
        case 3: n4 = 0b1100; break;
        case 4: n4 = 0b0010; break;
        case 5: n4 = 0b1010; break;
        case 6: n4 = 0b0110; break;
        case 7: n4 = 0b1110; break;
        case 8: n4 = 0b0001; break;
        case 9: n4 = 0b1001; break;
    }*/

    // Push to shift registers
    digitalWrite(DS_PIN, LOW);
    shiftOut(ST_CP, SH_CP, LSBFIRST, (n1 << 4) | n2);
    digitalWrite(DS_PIN, HIGH);
}

//  ---------------------
//  TASKS
//  ---------------------

void taskUpdateNixie(void* parameter) {
    Serial.println(F("[T] Nixie: preparing nixie updater..."));
    pinMode(DS_PIN, OUTPUT);

    while (!RTCready) { vTaskDelay(1000); }
    DateTime now = rtc.now();
    int lastMinute = now.minute();
    int lastHour = now.hour();
    
    Serial.println(F("[T] Nixie: Spawning nixie updater..."));
    for (;;) {
        Serial.print(F("[T] Nixie: lastHour: "));
            Serial.println(lastHour);
        Serial.print(F("[T] Nixie: lastMinute: "));
            Serial.println(lastMinute);
        
        // Periodically display time
        if (lastMinute != now.minute()) {
            Serial.println(F("[T] Nixie: Updating minutes..."));

            lastMinute = now.minute();
            displayNumber(now.hour(), now.minute());
        } else if (lastHour != now.hour()) {
            Serial.println(F("[T] Nixie: Updating hours..."));
            
            lastHour = now.minute();
            displayNumber(now.hour(), now.minute());
        } else {
            Serial.println(F("[T] Nixie: Idle..."));
        }

        vTaskDelay(3000);
    }
}

#endif