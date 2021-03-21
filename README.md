# Overview
Nixie clock using IN-14 tubes, based on an ESP32 and programmed with the arduino framework.<br/>
Also an attempt to utilize FreeRTOS.

This project aims to implement the following basic features:
 - Acquire time using NTP.
 - Store time on external RTC.
 - Display time (duh).
 - WebGUI for configuring all of the above. 

:warning: **Safety warning**</br>
Vacuum tubes generally use high voltage (180V+) in operation.<br/>
Proper handling and grounding of HV components is critical for personal safety.

## Software libraries
This project uses the following libraries:<br/>
- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)<br/>
- [AsyncTCP](https://github.com/me-no-dev/AsyncTCP)<br/>
- [RTCLib](https://github.com/adafruit/RTClib)<br/>
- [LittleFS](https://github.com/lorol/LITTLEFS)<br/>
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson)<br/>

Note: "TinyWire" must be removed as a dependency from RTCLib.<br/>
This dependency is incompatible with ESP32 and will result in compiler errors.

## Hardware
**Microcontroller:** ESP32</br>
**Indicator tubes:** 4x IN-14, 1x IN-3</br>
**Shift register:** 2x SN74HC595N (SIPO)</br>
**Driver IC:** 4x K155ID1

Theoretically, any cylindrical 13 pin IN tube could be used.<br/>
The PCB will be designed with the dimensions of the IN-14 in mind though.

## Power
The MCU will be driven using 5V, enough to power the shift registers and driver ICs.<br/>
The tubes must be supplied with 180V due to their design.

:information_source: The 180V are sourced externally.<br/>
No DC-DC step-up circuit diagrams are provided.

## Progress
Circuit diagrams and gerber files are currently WIP and not yet ready for release.<br/>
Software-wise, the following features have been implemented so far:
 - [X] Implement webserver.
 - [X] Implement RTC.
 - [X] Implement config files.
 - [X] Change configs from webGUI.
 - [ ] Implement proper manual time control.
 - [ ] Implement nixie logic.
 - [ ] Implement nixie config in webGUI.
 - [ ] Store SSID and PSK in config files.
 - [ ] Change network settings from webGUI.