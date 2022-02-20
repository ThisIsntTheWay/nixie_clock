#include <nixies.h>

/*  -------------------------------------
                VARS
    ------------------------------------- */
int optos[] = {4, 15, 18, 19};

/*  -------------------------------------
                MAIN
    ------------------------------------- */

/**************************************************************************/
/*!
    @brief Default constructor.
*/
/**************************************************************************/
Nixies::Nixies() {
}

/**************************************************************************/
/*!
    @brief Prepares shift registers and opto isolators.
    @param DS DS pin of shift registers.
    @param ST ST pin of shift registers.
    @param SH SH pin of shift registers.
*/
/**************************************************************************/
Nixies::Nixies(int DS, int ST, int SH) {
    this->SR_DS = DS;
    this->SR_ST = ST;
    this->SR_SH = SH;

    pinMode(DS, OUTPUT);
    pinMode(ST, OUTPUT);
    pinMode(SH, OUTPUT);

    // Set up ledC, creating a channel for each opto
    for (int i = 0; i < 4; i++) {
        int p = optos[i];
        pinMode(p, OUTPUT);
        
        ledcSetup(i, 100, 8);
        ledcAttachPin(p, i);
    }

    this->ready = true;
}

/**************************************************************************/
/*!
    @brief Returns true if nixies have been set up properly.
*/
/**************************************************************************/
bool Nixies::IsReady() {
    return this->ready;
}

/**************************************************************************/
/*!
    @brief Set display.
    @param displayVal Array of all tube values.
*/
/**************************************************************************/
void Nixies::SetDisplay(int displayVal[4]) {
    digitalWrite(this->SR_ST, 0);
        shiftOut(this->SR_DS, this->SR_SH, MSBFIRST, (displayVal[3] << 4) | displayVal[2]);
        shiftOut(this->SR_DS, this->SR_SH, MSBFIRST, (displayVal[1] << 4) | displayVal[0]);
    digitalWrite(this->SR_ST, 1);
}

/**************************************************************************/
/*!
    @brief Blank all tubes by writing invalid b1111 to all BCD decoders and turning anodes off.
*/
/**************************************************************************/
void Nixies::BlankDisplay() {
    // Push 0xFF to BCD decoders, disabling all outputs
    digitalWrite(this->SR_ST, 0);
        shiftOut(this->SR_DS, this->SR_SH, MSBFIRST, 0b1111111);
        shiftOut(this->SR_DS, this->SR_SH, MSBFIRST, 0b1111111);
    digitalWrite(this->SR_ST, 1);

    // Turn anode(s) off to prevent floating cathodes
    int aSize = sizeof(optos)/sizeof(optos[0]);
    for (int i = 0; i < aSize; i++) {
        ledcWrite(i, 0);
    }
}

/**************************************************************************/
/*!
    @brief Blanks a specific tube by turning its anode off.
    @param which LEDC channel of target tube, zero-indexed.
*/
/**************************************************************************/
void Nixies::blankTube(int which) {
    if (which > sizeof(optos)/sizeof(optos)) {
        Serial.printf("[X] BlankDisplay invalid 'which': %d.\n", which);
        throw;
    }

    ledcWrite(which, 0);
}