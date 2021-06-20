// Basically just the I2C scanner sketch: https://playground.arduino.cc/Main/I2cScanner/

#include <Wire.h>
#include <Arduino.h>

void setup() {
  Wire.begin();
 
  Serial.begin(115200);
}
 
 
void loop() {
  byte error, address;
  int nDevices;
 
  //Serial.println("Scanning...");
 
  nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
 
    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");

      if (address<16)
        Serial.print("0");

      Serial.println(address,HEX);
 
      nDevices++;
    }
    else if (error==4) {
      Serial.print("Unknown error at address 0x");

      if (address<16)
        Serial.print("0");

      Serial.println(address,HEX);
    }    
  } if (nDevices == 0)
    Serial.println("No I2C devices found\n");
 
  delay(250);
}
