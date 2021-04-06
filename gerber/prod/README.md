# Production ready gerber files
This directory contains gerber files ready for production.

## Iterations
A quick summary about all PCB revisions.
### Iteration 1 - 19.10.2020
#### nixie_clock-logic_unit_rev1.zip
Logic unit for all tube drivers and shift registers.  
Designed to mount a ESP32 dev kit and a small 12VDC-5VDC converter.  
Power is intented to be supplied using a traditional DC barrell jack.  
![PCB Rev1](https://i.imgur.com/5bnLabF.png)

### Iteration 2 - 06.04.2021
#### nixie_clock_prod_rev1.zip
Two seperate PCBs that mount all components for the clock.
The logic unit (lower part) houses the following systems:
- ESP32-WROOM-32, DS3231.
- 12VDC > 5V step-down using a fixed value LM2575T.

Both boards are designed to be connected using a 20-pin IDC connector.
170V power must be sourced externally.
![PCB Rev1](https://i.imgur.com/M2vcEAB.png)