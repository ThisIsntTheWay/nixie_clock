# Overview
Nixie clock using IN-16 tubes, based on an ESP32.
Also an attempt to utilize FreeRTOS.

This project aims to implement the following features:
 - Acquire time using NTP
 - Store time on external RTC
 - Display time (duh)
 - Optionally simulate ticking noises using a relay

:warning: This project uses high voltage (180V+) and has sufficient current to induce harm/death.<br/>
**Caution is advised.**

## Hardware
Microcontroller: ESP32</br>
Indicator tubes: 6x IN-16, 2x IN-3</br>
Shift register: SN74HC595N (SIPO)</br>
Driver IC: K155ID1

Theoretically, the indicator tubes may be substituted with any IN tube.<br/>
As long as the tube is cylindrical and has identical pins, this should pose no problem.

Keep in mind that the design of the PCB incorporates the dimensions of the IN-16!

## Power
The MCU will be driven using 5V, enough to power the shift registers and driver ICs.<br/>
The tubes must be supplied with 180V due to their design.

:information_source: The 180V comes from an external power supply.<br/>
As such, no circuit diagramm for this power supply can be provided.

## Notes
Circuit diagrams and gerber files are stored in EasyEDA and will be uploaded once ready for prototyping.<br/>
