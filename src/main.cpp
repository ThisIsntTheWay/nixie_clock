// ESP32 based nixie clock.
// REQUIRES: 8-Bit SIPO shift registers and BCD to DEC HV drivers.
// 
// (c) V. Klopfenstein, September 2020

// Inlcudes
#include <Arduino.h>

// Variables
#define srLatch = 2;
#define srClock = 2;
#define srData = 2;

byte BCDtable[10] = {0000, 0001, 0010,
                    0011, 0100, 0101,
                    0111, 1000, 1001,
                    1010}