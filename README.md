# Overview
Nixie clock using IN-14 tubes, based on an ESP32 and programmed with the arduino framework.<br/>
Employs FreeRTOS for multitasking.  

This project aims to implement the following basic features:
 - Acquire time using NTP.
 - Store time on external RTC.
 - Display time (duh).
 - WebGUI for configuring all of the above. 

:warning: **Safety warning**</br>
These tubes use high voltage (140V+) in operation.<br/>
Proper handling and grounding of HV components is critical for personal safety.

## Software libraries
This project uses the following libraries:<br/>
- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)<br/>
- [AsyncTCP](https://github.com/me-no-dev/AsyncTCP)<br/>
- [LittleFS](https://github.com/lorol/LITTLEFS)<br/>
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson)<br/>

## Hardware
**Microcontroller:** ESP32</br>
**Indicator tubes:** 4x IN-14, 1x IN-3</br>
**Opto-isolator:** 1x TLP 627-4</br>
**Shift register:** 2x SN74HC595N (SIPO)</br>
**Driver IC:** 4x K155ID1

Theoretically, any cylindrical 13 pin IN tube could be used.<br/>
However, the PCB will be designed with the dimensions of the IN-14.

## Power
The circuit has an intake of 5VDC, which is used for the following purposes:
 - Step-up to 170VDC to power all nixie tubes. 
 - Step-down to 3.3VDC to power all ICs.
   - Exception for the K155D1s as they are not CMOS-based.

:information_source: The step-up circuit for the 170VDC is not included in the schematic.<br/>

## Progress
Software-wise, the following features have been implemented so far:
 - [X] Implement webserver.
 - [X] Implement RTC.
 - [X] Implement config files.
 - [X] ~~Implement HUE API.~~
   - **Removed as of commit dbf017c67a21eaf4032c0a3266b7488ecc361137**
 - [X] Change configs from webGUI.
 - [ ] Implement proper manual time control.
 - [x] Implement nixie logic.
   - [X] Test said implementation.
 - [X] Implement nixie config in webGUI.
 - [X] Store SSID and PSK in config files.
 - [X] Change WiFi settings from webGUI.
 - [ ] Change network settings from webGUI.

### Notes
A major code rewrite was conducted on 02.08.2021 that has completely overhauled the base system.  
The system runs much more stable now and the codebase has become easier to maintain.
