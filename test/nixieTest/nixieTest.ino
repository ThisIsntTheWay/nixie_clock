// Bitshift pins
#define DS_PIN  27   // Data
#define SH_CP   26   // Clock
#define ST_CP   25   // Latch

int loopNum = 0;
int oPins[] = {19, 18, 4, 15};      // Board v2 (REV5) (July 2021)

// Convert dec to BCD
byte decToBCD(byte in) {
    return( (in/16*10) + (in%16) );
}

// The following function was inspired by:
// https://forum.arduino.cc/index.php?topic=449828.msg3094698#msg3094698
void displayNumber(int number_1, int number_2, int number_3, int number_4) {
    byte n1, n2, n3, n4;
    
    Serial.print("Displaying the following numbers: ");
        Serial.print(number_1);
        Serial.print(" ");
        Serial.print(number_2);
        Serial.print(" ");
        Serial.print(number_3);
        Serial.print(" ");
        Serial.println(number_4);

    n1 = decToBCD(number_1);
    n2 = decToBCD(number_2);
    n3 = decToBCD(number_3);
    n4 = decToBCD(number_4);

    Serial.print(" > Num2 BCD: "); Serial.println(n2, BIN);
    Serial.print(" > Pass 1 BCD: "); Serial.println((n3 << 4 | n4), BIN);
    Serial.print(" > Pass 2 BCD: "); Serial.println((n1 << 4 | n2), BIN);

    // Push to shift registers
    digitalWrite(ST_CP, LOW);
    shiftOut(DS_PIN, SH_CP, MSBFIRST, (n3 << 4) | n4);  // REG1 [Hours]
    shiftOut(DS_PIN, SH_CP, MSBFIRST, (n1 << 4) | n2);  // REG2 [Minutes]
    digitalWrite(ST_CP, HIGH);
}

void setup() {
  Serial.begin(115200);

  pinMode(DS_PIN, OUTPUT);
  pinMode(SH_CP, OUTPUT);
  pinMode(ST_CP, OUTPUT);

  // Opto isolator
  for (int i = 0; i < 4; i++) {
    int p = oPins[i];

    pinMode(p, OUTPUT);
    
    ledcSetup(i, 100, 8);
    ledcAttachPin(p, i);
    ledcWrite(i, 255);
  }
}

void loop() {
  loopNum++;
  Serial.println(loopNum);

  for (int i = 0; i < 10; i++) {
   displayNumber(i,i,i,i);
   delay(2550);
  }
}
