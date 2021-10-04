#ifndef NIXIES_H
#define NIXIES_H

class Nixies {
    public:
        bool isTumbling;
        bool isReady;
        bool forceUpdate = false;

        void initialize(int DS, int ST, int SH, int pwmFreq);
        void changeDisplay(int numArr[]);
        void setBrightness(int ch, int pwm, bool all);
        void tumbleDisplay();
        void blankDisplay();

        int returnCache();

        static int t1;
        static int t2;
        static int t3;
        static int t4;

    private:
        byte decToBcd(byte val);
};

#endif