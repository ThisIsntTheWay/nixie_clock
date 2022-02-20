#ifndef NIXIES_H
#define NIXIES_H

#include <Arduino.h>

class Nixies {
    public:
        Nixies();
        Nixies(int, int, int);

        void SetDisplay(int[4]);
        void SetIndicator(int, bool);
        void BlankDisplay();
        bool IsReady();

    private:
        void blankTube(int);
        bool ready;
        uint8_t SR_DS;
        uint8_t SR_ST;
        uint8_t SR_SH;
};

#endif