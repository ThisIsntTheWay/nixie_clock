/*
    ESP32 Nixie Clock - Utilities module
    (c) V. Klopfenstein, 2021

    This code block contains various functions that cannot be categorized.
*/

#include <arduino.h>
#ifndef utils_h
#define utils_h

int validateEntry(const char *input, const int mode, const int versus) {
    // Convert char to string
    String inString = String(input);
    bool out = 1;

    switch (mode) {
        // Verify length
        // Return FALSE if less than versus
        case 1:
            if (inString.length() < versus)
                out = 0;
            break;

        // Verify number
        case 2:
            break;
    }

    // By default, return TRUE
    return out;
}

#endif