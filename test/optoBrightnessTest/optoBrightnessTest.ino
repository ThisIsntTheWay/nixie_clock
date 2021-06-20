#include <Arduino.h>

// LED Channel oconfig
const int freq = 200; 
const int chPwm = 0; 
const int resolution = 8;

// Bitshift pins
#define DS_PIN  27   // Data
#define SH_CP   26   // Clock
#define ST_CP   25   // Latch

// Optoisolator IR LEDs
int opto1 = 2;
int opto2 = 15;
int opto3 = 4;
int opto4 = 5;

// binary 8
byte b8 = 0b1000;

void setup() {
  pinMode(opto1, OUTPUT);
  pinMode(opto2, OUTPUT);
  pinMode(opto3, OUTPUT);
  pinMode(opto4, OUTPUT);
  
  pinMode(DS_PIN, OUTPUT);
  pinMode(SH_CP, OUTPUT);
  pinMode(ST_CP, OUTPUT);

  // Shift out 0b10001000
  digitalWrite(ST_CP, LOW);
  shiftOut(DS_PIN, SH_CP, MSBFIRST, (b8 << 4) | b8);
  shiftOut(DS_PIN, SH_CP, MSBFIRST, (b8 << 4) | b8);
  digitalWrite(ST_CP, HIGH);
  
  Serial.begin(115200);

  // init LED ctrl channel and assign members
  ledcSetup(chPwm, freq, resolution);
  ledcAttachPin(opto1, chPwm);
  ledcAttachPin(opto2, chPwm);
  ledcAttachPin(opto3, chPwm);
  ledcAttachPin(opto4, chPwm);
}

void loop() {
  Serial.println("Advancing...");
  for(int dutyCycle = 1; dutyCycle <= 190; dutyCycle++){
      // changing the LED brightness with PWM
      ledcWrite(chPwm, dutyCycle);
      delay(5); 

      Serial.println(dutyCycle);
  }
  
  Serial.println("Regressing...");
  for(int dutyCycle = 190; dutyCycle >= 1; dutyCycle--){
      // changing the LED brightness with PWM
      ledcWrite(chPwm, dutyCycle);
      delay(5);
      
      Serial.println(dutyCycle);
   }
  
  delay(50);
}