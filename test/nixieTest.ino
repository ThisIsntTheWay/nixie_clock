
#define DS_PIN 19   // Latch | ST_CP
#define SH_CP 18    // Clock | SH_CP
#define ST_CP 23    // Data  | DS

int i = 0;

void displayNumber(int number_1, int number_2) {//, int number_3, int number_4) {
    byte n1, n2, n3, n4;

    switch (number_1) {
        case 0: n1 = 0b0000; break;
        case 1: n1 = 0b1000; break;
        case 2: n1 = 0b0100; break;
        case 3: n1 = 0b1100; break;
        case 4: n1 = 0b0010; break;
        case 5: n1 = 0b1010; break;
        case 6: n1 = 0b0110; break;
        case 7: n1 = 0b1110; break;
        case 8: n1 = 0b0001; break;
        case 9: n1 = 0b1001; break;
    }

    switch (number_2) {
        case 0: n2 = 0b0000; break;
        case 1: n2 = 0b1000; break;
        case 2: n2 = 0b0100; break;
        case 3: n2 = 0b1100; break;
        case 4: n2 = 0b0010; break;
        case 5: n2 = 0b1010; break;
        case 6: n2 = 0b0110; break;
        case 7: n2 = 0b1110; break;
        case 8: n2 = 0b0001; break;
        case 9: n2 = 0b1001; break;
    }

    // Push to shift registers
    digitalWrite(DS_PIN, LOW);
    shiftOut(ST_CP, SH_CP, MSBFIRST, (n1 << 4) | n2);
    //shiftOut(ST_CP, SH_CP, LSBFIRST, n2);
    digitalWrite(DS_PIN, HIGH);
}

void setup() {
  Serial.begin(115200);
  pinMode(DS_PIN, OUTPUT);
  pinMode(SH_CP, OUTPUT);
  pinMode(ST_CP, OUTPUT);
}

void loop() {
  i++;
  
  if (i > 9)
    i = 0;

  Serial.println(i);
  
  displayNumber(i,i+1);

  delay(2000);
}