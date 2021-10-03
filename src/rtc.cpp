#include <Arduino.h>
#include <Wire.h>
#include <rtc.h>

// Designed to work with RTC models DS1307/DS3231 (or similar)

/****************************************************/
//  DEFINITIONS
/****************************************************/
//#define DEBUG

#define CUSTOM_I2C            // Board REV5 and up
#define I2C_SLOWMODE          // I2C 100kHz

#ifdef CUSTOM_I2C
    #define SDA_PIN 33
    #define SCL_PIN 13
#endif

// Declare statics
bool RTC::RTCfault = false;
bool RTC::i2cLock = false;

/****************************************************/
//  MAIN
/****************************************************/

/**************************************************************************/
/*!
    @brief  Converts BCD to a decimal value
    @param val BCD value to convert
*/
/**************************************************************************/
byte RTC::bcdToDec(byte val) {
    if (val == 0)   { return 0; }
    else            { return ((val/16*10) + (val%16)); }
}

/**************************************************************************/
/*!
    @brief  Converts decimal to a BCD value
    @param val Decimal value to convert
*/
/**************************************************************************/
byte RTC::decToBcd(byte val) {
    if (val == 0)   { return 0; }
    else            { return ((val/10*16) + (val%10)); }
}

/**************************************************************************/
//  CONSTRUCTORS
/**************************************************************************/
RTC::RTC() : _Wire(Wire) {}
RTC::RTC(TwoWire & w) : _Wire(w) {}

/**************************************************************************/
/*!
    @brief  Starts Wire and checks if the RTC has responded.
*/
/**************************************************************************/
bool RTC::initialize() {
    #ifdef CUSTOM_I2C
        _Wire.begin(SDA_PIN, SCL_PIN);
    #else
        _Wire.begin();
    #endif

    #ifdef I2C_SLOWMODE
        _Wire.setClock(100000);
    #else
        _Wire.setClock(400000);
    #endif

    _Wire.beginTransmission(RTC_ADDR);
    if (_Wire.endTransmission() == 0) {
        return true;
    }
    
    return false;
}

/**************************************************************************/
/*!
    @brief  Checks for the OSF bit in the control/status register.
*/
/**************************************************************************/
int RTC::checkStartup() {
    byte val = RTC::readRegister(0x0F);
    #ifdef DEBUG
        Serial.print("[i] RTC: Register 0x0F: "); Serial.println(val, BIN);
    #endif

    // '99' means that readRegister() experienced an error
    if (val != 99) {
        // Isolate bit 8 (OSF register)
        byte OSFval = bitRead(val, 7);
        #ifdef DEBUG
            Serial.print("[i] RTC: OSF bit set to: "); Serial.println(OSFval, BIN);
        #endif

        // Reset RTC time if OSF bit is set to 1
        if (OSFval == 1) {
            // Get compile time and trim the string so that it can be used with toInt()
            String tmp = __TIME__;      //xx:yy:zz
            tmp.remove(2,1);            //xxyy:zz
            tmp.remove(4,1);            //xxyyzz

            int timeStr = tmp.toInt();
            
            // Seperate into seconds, minutes and hours
            int timeArr[3] =  {
                (timeStr % 100), 
                ((timeStr / 100) % 100),
                (timeStr / 10000)
            };

            for (int i = 0; i <= 2; i++) {
                #ifdef DEBUG
                    Serial.printf("[i]: Setting RTC time for unit %d: %d\n", i, timeArr[i]);
                #endif
                RTC::setTime(i, timeArr[i]);
            }

            // Set OSF bit to 0, indicating that time has been set
            val = val & 0b01111111;
            RTC::writeRegister(0x0F, val);
            
            #ifdef DEBUG
                Serial.print(F("[i]: val for Address 0x0F is now at: "));
                Serial.println(val, BIN);
            #endif

            // All OK, but time had to be set on the RTC
            return 1;
        } else {
            // All OK
            return 0;
        }
    } else {
        // Assume irrecoverable error
        return 2;
    }

    // Default return, should never occur
    return 3;
}

/**************************************************************************/
/*!
    @brief  Reads from a specified register.
    @param reg Register to write to (in HEX).
*/
/**************************************************************************/
byte RTC::readRegister(byte reg) {
    byte error;
    _Wire.beginTransmission(RTC_ADDR);
    _Wire.write(reg);
    error = _Wire.endTransmission();

    // Only proceed if endTransmission exited successfully
    if (error == 0) {
        _Wire.requestFrom(RTC_ADDR, 1);
        return _Wire.read();
    } else {
        return 99;
    }
}

/**************************************************************************/
/*!
    @brief  Writes to a specific register.
    @param reg Register to write to (in HEX).
    @param payload Payload, binary.
*/
/**************************************************************************/
byte RTC::writeRegister(byte reg, byte payload) {
    _Wire.beginTransmission(RTC_ADDR);
    _Wire.write(reg);
    _Wire.write(payload);

    byte result;
    result = _Wire.endTransmission();

    return result;
}

/**************************************************************************/
/*!
    @brief  Reads from a timekeeping register. Returns a DEC number.
    @param unit 0: Seconds, 1: Minutes, 2: Hours.
    @warning If a register higher than 0x02 is accessed, the number '99' will be rerturned.
*/
/**************************************************************************/
int RTC::getTime(int unit) {
    if (unit >= 3) {
        return 99;
    } else {
        return RTC::bcdToDec(RTC::readRegister(unit));
    }
}

/**************************************************************************/
/*!
    @brief  Sets the time.
    @param unit 0: Seconds, 1: Minutes, 2: Hours.
    @param data Number (DEC) to write to specified time register.
*/
/**************************************************************************/
int RTC::setTime(int unit, int data) {
    byte val = RTC::decToBcd(data);
    return RTC::writeRegister(unit, val);    
}

/**************************************************************************/
/*!
    @brief  Get time as string(): "hh:mm:ss"
*/
/**************************************************************************/
String RTC::getTimeText() {
    int s = RTC::getTime(0);
    int m = RTC::getTime(1);
    int h = RTC::getTime(2);

    // Padding
    String sT;
    if (s <= 9) {sT = "0" + String(s);}
    else        {sT = String(s);}
    
    String mT;
    if (m <= 9) {mT = "0" + String(m);}
    else        {mT = String(m);}

    String hT;
    if (h <= 9) {hT = "0" + String(h);}
    else        {hT = String(h);}

    return hT + ":" + mT + ":" + sT;
}