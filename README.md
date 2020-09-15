# Overview
Nixie clock using IN-16 tubes, based on an ESP32 and programmed with the arduino framework.
Also an attempt to utilize FreeRTOS.

This project aims to implement the following basic features:
 - Acquire time using NTP
 - Store time on external RTC
 - Display time (duh)
 
 Optional features might also get implemented sometime:
 - Simulate ticking noises with relays
   - Might be too obnoxious
 - WebGUI for interfacing with the MCU
 - (Very basic) REST API endpoint

:warning: **Safety warning**</br>
Nixie/vacuum tubes generally use high voltage (180V+) and have sufficient current to induce harm and/or death.<br/>
Proper handling and grounding of HV components is critical.

## Hardware
**Microcontroller:** ESP32</br>
**Indicator tubes:** 6x IN-16, 2x IN-3</br>
**Shift register:** SN74HC595N (SIPO)</br>
**Driver IC:** K155ID1

### Substitutions
Not all the hardware listed here must be used in order for this project to work.

- Microcontroller</br>
Any MCU that is compatible with the arduino framework can be used.</br>
The ESP32 was chosen due to its built-in wirless capabilities.

- Shift registers</br>
Any shift register _could_ be used, but an 8-bit **SIPO** register is highly recommended.</br>
Chips with more or less flip-flops may need some code modifications.

- Indicator tubes</br>
Theoretically, any cylindrical 13 pin IN tube could be used.<br/>
Keep in mind that the design of the PCB is based on the dimensions of the IN-16!

## Power
The MCU will be driven using 5V, enough to power the shift registers and driver ICs.<br/>
The tubes must be supplied with 180V due to their design.

:information_source: The 180V are sourced externally.<br/>
No DC-DC conversion circuit diagrams are provided.

## Notes
Circuit diagrams and gerber files are stored in EasyEDA and will be uploaded once ready for prototyping.<br/>
