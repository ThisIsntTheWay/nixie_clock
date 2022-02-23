#ifndef DISPLAYCONTROL_H
#define DISPLAYCONTROL_H

#define ONBOARD_LED_PIN 16
#define DS_PIN  27   // Data
#define SH_PIN  26   // Clock
#define ST_PIN  25   // Latch

#define TASK_TICK_DELAY 10
#define LEDC_PWM_FREQUENCY 100
#define ONBOARD_LEDC_CHANNEL 15

#define ONBOARD_LED_BLINK_INTERVAL 200
#define ONBOARD_LED_PULSE_INTERVAL 5

#include <Arduino.h>
#include <nixies.h>

class DisplayController {
    public:
        static int TubeVals[4][2];
        static uint8_t OnboardLedPWM;
        static uint8_t OnboardLEDmode;
        static uint8_t OnboardLEDblinkAmount;
        static uint8_t DetoxCycle;
        static bool AllowRESTcontrol;
        static bool Clock;
        static bool DoDetox;
};

void taskSetDisplay(void* parameter);
void taskSetStatusLED(void* parameter);
void taskSetLeds(void* parameter);
void taskVisIndicator(void* paramter);

#endif