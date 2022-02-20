#include <displayController.h>
#include <timekeeper.h>

#define SOCKET_FOOTPRINT_INVERTED

/* -------------------
    Vars
   ------------------- */

Nixies nixies(DS_PIN, ST_PIN, SH_PIN);

int DisplayController::TubeVals[4][2] = {{9, 255}, {9, 255}, {9, 255}, {9, 255}};

bool DisplayController::AllowRESTcontrol = true;
bool DisplayController::Clock = false;

uint8_t DisplayController::OnboardLedPWM = 75;
uint8_t DisplayController::OnboardLEDmode = 0;
uint8_t DisplayController::OnboardLEDblinkAmount = 0;

/* -------------------
    Main
   ------------------- */

void taskSetDisplay(void* parameter) {
    while (!nixies.IsReady()) {
        vTaskDelay(200);
    }

    Timekeeper timekeeper;

    for (;;) {
        int t[4] = {11, 11, 11, 11};

        if (!DisplayController::Clock) {
            // Normal operation
            for (int i = 0; i < 4; i++) {
                int8_t tubeVal = DisplayController::TubeVals[i][0];
                int8_t tubePWM = DisplayController::TubeVals[i][1];

                #ifdef SOCKET_FOOTPRINT_INVERTED
                    uint8_t a;
                    switch (tubeVal) {
                        case 0:
                            a = 1;
                            break;
                        case 1:
                            a = 0;
                            break;
                        default:
                            a = 11 - tubeVal;
                    }

                    t[i] = a;
                #else
                    t[i] = tubeVal;
                #endif

                // Fully turn off tube instead of leaving cathodes floating.
                if (tubeVal > 9)    { ledcWrite(i, 0); }
                else                { ledcWrite(i, tubePWM); }
            }
        } else {
            t[0] = timekeeper.time.hours / 10;
            t[1] = timekeeper.time.hours % 10;
            t[2] = timekeeper.time.minutes / 10;
            t[3] = timekeeper.time.minutes % 10;

            for (int i = 0; i < 4; i++) {
                int8_t tubePWM = DisplayController::TubeVals[i][1];
                ledcWrite(i, tubePWM);
            }
        }
        
        // Blank all tubes that were not considered. (Their index is missing in TubeVals)
        for (int i = 0; i < 4; i++) {
            if (t[i] == 11) {
                ledcWrite(i, 0);
            }
        }

        nixies.SetDisplay(t);
        vTaskDelay(TASK_TICK_DELAY);
    }
}

// Status LED for ESP32
void taskSetStatusLED(void* parameter) {
    pinMode(ONBOARD_LED_PIN, OUTPUT);

    ledcSetup(ONBOARD_LEDC_CHANNEL, LEDC_PWM_FREQUENCY, 8);
    ledcAttachPin(ONBOARD_LED_PIN, ONBOARD_LEDC_CHANNEL);

    for (;;) {
        switch (DisplayController::OnboardLEDmode) {
            case 1:
                // Blink
                // -> LED is turned OFF but turns ON in a blinking matter
                for (int i = 0; i <= DisplayController::OnboardLEDblinkAmount; i++) {
                    ledcWrite(ONBOARD_LEDC_CHANNEL, DisplayController::OnboardLedPWM);
                    vTaskDelay(ONBOARD_LED_BLINK_INTERVAL);
                    ledcWrite(ONBOARD_LEDC_CHANNEL, 0);
                    vTaskDelay(ONBOARD_LED_BLINK_INTERVAL);
                }

                vTaskDelay(1000);
                break;
            case 2:
                // "Inverted" blink
                // -> LED is turned ON but turns OFF in a blinking matter
                ledcWrite(ONBOARD_LEDC_CHANNEL, DisplayController::OnboardLedPWM);
                for (int i = 0; i <= DisplayController::OnboardLEDblinkAmount; i++) {
                    ledcWrite(ONBOARD_LEDC_CHANNEL, 0);
                    vTaskDelay(ONBOARD_LED_BLINK_INTERVAL);
                    ledcWrite(ONBOARD_LEDC_CHANNEL, DisplayController::OnboardLedPWM);
                    vTaskDelay(ONBOARD_LED_BLINK_INTERVAL);
                }

                vTaskDelay(1000);
                break;
            case 3: {
                // Pulsate
                // Duty cycles above 150 are hardly perceived as brighter, so this value serves as the ceiling.
                uint8_t maxPWM = 150;
                for (int i = 0; i < maxPWM; i++) {
                    if (DisplayController::OnboardLEDmode != 3) break;

                    ledcWrite(ONBOARD_LEDC_CHANNEL, i);
                    vTaskDelay(ONBOARD_LED_PULSE_INTERVAL);
                }
                for (int i = 170; i > 0; i--) {
                    if (DisplayController::OnboardLEDmode != 3) break;

                    ledcWrite(ONBOARD_LEDC_CHANNEL, i);
                    vTaskDelay(ONBOARD_LED_PULSE_INTERVAL);
                }
                break;
            }
            default:
                // Light up using specified PWM, a.k.a mode "0".
                ledcWrite(ONBOARD_LEDC_CHANNEL, DisplayController::OnboardLedPWM);
                break;
        }

        vTaskDelay(TASK_TICK_DELAY);
    }
}