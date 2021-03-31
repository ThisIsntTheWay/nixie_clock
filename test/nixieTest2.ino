// Bitshift pins
#define latchPin 19   // Latch  | ST_CP
#define clockPin 18   // Clock  | SH_CP
#define dataPin 23    // Data   | DS

/*  a sketch to drive two nixie tubes using two SN74141 BCD chips and a single SN74HC595N shift register
    developed from the tutorials on Adafruit.com and arduino.cc
    the sketch will cause two nixie tubes to count from 0 to 99 but you can change it to create any two-digit number and have the nixie tube display it
    Jeff Glans 2013
    Released into the public domain
    
*/
//set up the pins for communication with the shift register
int x; //create a counting variable


// create an array that translates decimal numbers into an appropriate byte for sending to the shift register
int charTable[] = {
0,1,2,3,4,5,6,7,8,9,
16,17,18,19,20,21,22,23,24,25,
32,33,34,35,36,37,38,39,40,41,
48,49,50,51,52,53,54,55,56,57,
64,65,66,67,68,69,70,71,72,73,
80,81,82,83,84,85,86,87,88,89,
96,97,98,99,100,101,102,103,104,105,
112,113,114,115,116,117,118,119,120,121,
128,129,130,131,132,133,134,135,136,137,
144,145,146,147,148,149,150,151,152,153};


byte nixies = 255; //initiate the byte to be sent to the shift register and set it to blank the nixies

void setup(){
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  Serial.begin(115200);
}

void loop(){
  nixies = 255; // create a blank byte
  updateShiftRegister(); // send the blank byte to the shift register
  delay(500);
  

  for (x = 0; x<100; x++){ // count from 0 to 99
    nixies = charTable[x]; // translate into a byte to send to the shift register
     
    updateShiftRegister(); //send to the shift register
    delay(500);
  
  Serial.print("x = ");
  Serial.println(x);
  Serial.print("nixies = ");
  Serial.println(nixies);}
}
  //the process of sending a byte to the shift register
void updateShiftRegister(){
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, nixies);
  digitalWrite(latchPin, HIGH);
}
  


