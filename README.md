# Overview
Nixie clock using IN-16 tubes, based on an ESP32 and programmed with the arduino framework.<br/>
Also an attempt to utilize FreeRTOS.

This project aims to implement the following basic features:
 - Acquire time using NTP.
 - Store time on external RTC.
 - Display time (duh).
 - WebGUI for configuring all of the above. 

:warning: **Safety warning**</br>
Vacuum tubes generally use high voltage (170V+) in operation.<br/>
Proper handling and grounding of HV components is critical for personal safety.

## Software libraries
This project uses the following libraries:<br/>
- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)<br/>
- [AsyncTCP](https://github.com/me-no-dev/AsyncTCP)<br/>
- [RTCLib](https://github.com/adafruit/RTClib)<br/>
- [LittleFS](https://github.com/lorol/LITTLEFS)<br/>
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson)<br/>

Note: "TinyWire" must be removed as a dependency from RTCLib.<br/>
This library is incompatible with the ESP32 and will result in compiler errors.

## Hardware
**Microcontroller:** ESP32</br>
**Indicator tubes:** 4x IN-14, 1x IN-3</br>
**Shift register:** 2x SN74HC595N (SIPO)</br>
**Driver IC:** 4x K155ID1

Theoretically, any cylindrical 13 pin IN tube could be used.<br/>
However, the PCB will be designed with the dimensions of the IN-14.

## Power
The circuit has an intake of 12VDC, which is used for the following purposes:
 - Step-up to 170VDC to power all nixie tubes. 
 - Step-down to 5VDC to power all ICs.
   - This 5VDC is further decreased to 3.3VDC to power the MCU and RTC.

The reason for 12VDC versus 5VDC is for a much stabler generation of 170V.  
Whilst it is possible to step-up said voltage from just 5VDC as well, it is very taxing on those components.  

:information_source: The step-up circuit for the 170VDC is not included in the schematic.<br/>

## Progress
Circuit diagrams and gerber files are currently WIP and not yet ready for release.<br/>
Software-wise, the following features have been implemented so far:
 - [X] Implement webserver.
 - [X] Implement RTC.
 - [X] Implement config files.
 - [X] Implement HUE API.
 - [X] Change configs from webGUI.
 - [ ] Implement proper manual time control.
 - [x] Implement nixie logic.
   - [X] Test said implementation.
 - [ ] Implement nixie config in webGUI.
 - [X] Store SSID and PSK in config files.
 - [X] Change WiFi settings from webGUI..
 - [ ] Change network settings from webGUI.
