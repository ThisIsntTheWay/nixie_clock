// Bitshift pins
#define DS_PIN  27   // Data
#define SH_CP   26   // Clock
#define ST_CP   25   // Latch

int loopNum = 0;
int oPins[] = {19, 18, 4, 15};      // Board v2 (REV5) (July 2021)

#define INVERTED_PCB_FOOTPRINT

// Convert dec to BCD
byte decToBCD(byte in) {
    return( (in/16*10) + (in%16) );
}

// The following function was inspired by:
// https://forum.arduino.cc/index.php?topic=449828.msg3094698#msg3094698
void displayNumber(int iN[]) {
    byte n1, n2, n3, n4, BCD1, BCD2;

    #ifdef INVERTED_PCB_FOOTPRINT
      for (int i = 0; i < 4; i++) {
        int a;
  
        switch (iN[i]) {
            case 0:
                a = 1;
                break;
            case 1:
                a = 0;
                break;
            default:
                a = 11 - iN[i];
        }
  
        iN[i] = a;
      }
    #endif
    
    n1 = iN[0];
    n2 = iN[1];
    n3 = iN[2];
    n4 = iN[3];
    
    Serial.printf("Displaying the following numbers: %d %d %d %d\n", n1, n2, n3, n4);

    BCD1 = (n1 << 4 | n2);
    BCD2 = (n4 << 4 | n3);

    Serial.print(" > Num2 BCD: "); Serial.println(n2, BIN);
    Serial.print(" > Pass 1 BCD: "); Serial.println(BCD1, BIN);
    Serial.print(" > Pass 2 BCD: "); Serial.println(BCD2, BIN);

    // Push to shift registers
    digitalWrite(ST_CP, LOW);
    shiftOut(DS_PIN, SH_CP, MSBFIRST, BCD1);  // REG1 [Hours]
    shiftOut(DS_PIN, SH_CP, MSBFIRST, BCD2);  // REG2 [Minutes]
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
    ledcWrite(i, 170);
  }

  Serial.println("System read, awaiting input...");
}

/*
IN  | GET
0 - 1
1 - 0
2 - 9
3 - 8
4 - 7
5 - 6
6 - 5
7 - 4
8 - 3
9 - 2

 */

void loop() {
  loopNum++;

  while(Serial.available() == 0) { }
  int i = Serial.parseInt();

  int iArr[] = {i, i, i, i};
  
  Serial.print(loopNum);
  Serial.printf(" ======================\nGot: %d\n",i);
  displayNumber(iArr);

  /*
  for (int i = 0; i < 10; i++) {
   displayNumber(i,i,i,i);
   delay(2550);
  } */
}
