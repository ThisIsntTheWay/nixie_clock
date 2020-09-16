# Overview
Nixie clock using IN-16 tubes, based on an ESP32 and programmed with the arduino framework.
Also an attempt to utilize FreeRTOS.

This project aims to implement the following basic features:
 - Acquire time using NTP
 - Store time on external RTC
 - Display time (duh)
 
 Optional features might also get implemented sometime:
 - Simulate ticking noises with relays
 - WebGUI for interfacing with the MCU

:warning: **Safety warning**</br>
Vacuum tubes generally use high voltage (180V+) in operation.<br/>
Proper handling and grounding of HV components is critical for personal safety.

## Software libraries
This project uses the following third-party libraries:<br/>
- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)<br/>
- [AsyncTCP](https://github.com/me-no-dev/AsyncTCP)

## Hardware
**Microcontroller:** ESP32</br>
**Indicator tubes:** 6x IN-16, 2x IN-3</br>
**Shift register:** SN74HC595N (SIPO)</br>
**Driver IC:** K155ID1

Theoretically, any cylindrical 13 pin IN tube could be used.<br/>
The PCB will be designed with the dimensions of the IN-16 in mind though.

## Power
The MCU will be driven using 5V, enough to power the shift registers and driver ICs.<br/>
The tubes must be supplied with 180V due to their design.

:information_source: The 180V are sourced externally.<br/>
No DC-DC step-up circuit diagrams are provided.

## Notes
Circuit diagrams and gerber files are currently WIP and not yet release-ready.
