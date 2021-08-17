#include <Arduino.h>
#include <nixies.h>
#include <config.h>

int oPins[] = {5, 4, 2, 15};        // Board REV4 and lower
//int oPins[] = {19, 18, 4, 15};      // Board v2 (REV5) (July 2021)

// Instantiate statics
int Nixies::t1 = 0;
int Nixies::t2 = 0;
int Nixies::t3 = 0;
int Nixies::t4 = 0;

byte Nixies::decToBcd(byte val) {
    if (val == 0)   { return 0; }
    else            { return((val/10*16) + (val%10)); }
}

/**************************************************************************/
/*!
    @brief  Initialize nixies by setting up the shift registers and LEDC.
    @param DS Data-In pin of shift registers
    @param ST Shift register clock pin
    @param SH Shift register latch pin
    @param pwmFreq PWM frequency for LEDC
*/
/**************************************************************************/
void Nixies::initialize(int DS, int ST, int SH, int pwmFreq) {
    pinMode(DS, OUTPUT);
    pinMode(ST, OUTPUT);
    pinMode(SH, OUTPUT);

    // Set up ledC, creating a channel for each opto
    Serial.println(F("[i] Nixie: Setting up LED controller..."));

    for (int i = 0; i < 4; i++) {
        int p = oPins[i];

        pinMode(p, OUTPUT);
        
        ledcSetup(i, pwmFreq, 8);
        ledcAttachPin(p, i);
        
        #ifdef DEBUG
            Serial.printf("[i] Nixie: LEDC channel #%d on pin '%d' ready.\n", i, p);
        #endif
    }
}

/**************************************************************************/
/*!
    @brief  Change tube displays
    @param n1 Number for tube #1
    @param n2 Number for tube #2
    @param n3 Number for tube #3
    @param n4 Number for tube #4
    @warning params 'n3' and 'n4' will be ignored if 'FULL_TUBESET' is undefined.
*/
/**************************************************************************/
void Nixies::changeDisplay(int n1, int n2, int n3, int n4) {
    digitalWrite(ST_PIN, 0);
        shiftOut(DS_PIN, SH_PIN, MSBFIRST, (n1 << 4) | n2);
    #ifdef FULL_TUBESET
        shiftOut(DS_PIN, SH_PIN, MSBFIRST, (n3 << 4) | n4);
    #endif
    digitalWrite(ST_PIN, 1);

    #ifdef DEBUG_VERBOSE
        Serial.printf("[T] Nixie: Setting tubes: %d%d %d%d\n", n1, n2, n3, n4);
    #endif

    // Update number cache
    t1 = n1;
    t2 = n2;
    t3 = n3;
    t4 = n4;

    #ifdef DEBUG_VERBOSE
        Serial.printf("[i] Nixies: Display cache: %d%d %d%d\n", t1, t2, t3, t4);
    #endif
}

/**************************************************************************/
/*!
    @brief  Returns the nixie tube display cache as single number. (examples: 1234, 8888, 1637)
*/
/**************************************************************************/
int Nixies::returnCache() {
    Serial.printf("vals are: %d%d%d%d\n", t1, t2, t3, t4);

    int tmp = (t1 * 1000) + (t2 * 100) + (t3 * 10) + t4;
    Serial.printf("[i] returning nixe cache: %d\n", tmp);
    return tmp;
}

/**************************************************************************/
/*!
    @brief  "Tumble" the display, essentially achieving cathode depoisoning
*/
/**************************************************************************/
void Nixies::tumbleDisplay() {
    Nixies::isTumbling = true;

    #ifdef DEBUG
        Serial.println("[T] Nixie: FUNC: Tumbling display.");
    #endif

    for (int i = 0; i < 10; i++) {
        byte a = Nixies::decToBcd(i);

        Nixies::changeDisplay(a, a, a, a);
        vTaskDelay(DEPOISON_DELAY);
    }
    
    for (int i = 9; i > -1; i--) {
        byte a = Nixies::decToBcd(i);

        Nixies::changeDisplay(a, a, a, a);
        vTaskDelay(DEPOISON_DELAY);
    }

    Nixies::isTumbling = false;
}

/**************************************************************************/
/*!
    @brief  Turn off the display.
*/
/**************************************************************************/
void Nixies::blankDisplay() {
    // Push 0xFF to BCD decoders, disabling all outputs
    digitalWrite(ST_PIN, 0);
        shiftOut(DS_PIN, SH_PIN, MSBFIRST, 0b1111111);
        #ifdef FULL_TUBESET
            shiftOut(DS_PIN, SH_PIN, MSBFIRST, 0b1111111);
        #endif
    digitalWrite(ST_PIN, 1);

    // Turn anode(s) off to prevent floating cathodes
    int aSize = sizeof(oPins)/sizeof(oPins[0]);
    for (int i = 1; i < aSize; i++) {
        ledcWrite(i, 0);
    }
}

/**************************************************************************/
/*!
    @brief  Set the brightness of a tube
    @param ch LEDC channel
    @param pwm Duty cycle length
    @param all Change all channels
*/
/**************************************************************************/
void Nixies::setBrightness(int ch, int pwm, bool all) {
    #ifdef DEBUG_VERBOSE
        Serial.printf("[T] Nixie: Setting PWM of '%d' on LEDC channel #%d\n", pwm, ch);
    #endif

    if (all) {
        for (int i = 0; i <= 3; i++) {
            ledcWrite(i, pwm);
        }
    } else {
        ledcWrite(ch, pwm);
    }
        
}