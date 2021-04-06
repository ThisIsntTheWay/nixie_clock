# Production ready gerber files
This directory contains gerber files ready for production.

## Iterations
A quick summary about all PCB revisions.
### Iteration 1 - 19.10.2020
Removed due to superior designs.  
![PCB Rev1](https://i.imgur.com/5bnLabF.png)

### Iteration 2 - 05.04.2021
#### nixie_clock_prod_rev1.zip
Two seperate PCBs that mount all components for the clock.
The logic unit (lower part) houses the following systems:
- ESP32-WROOM-32, DS3231.
- 12VDC > 5V step-down using a fixed value LM2575T.

Both boards are designed to be connected using a 20-pin IDC connector.  
170V power must be sourced externally.
![PCB Rev1](https://i.imgur.com/M2vcEAB.png)

### Iteration 3 - 06.04.2021
#### nixie_clock_prod_rev2.zip (WIP)
The same principle as revision 1, this time with considerable changes though:
- Increased clearances.
- Improved 0.1uF cap placement by DS3231.
- Removed duplicate tracks - mostly GND.
- Expanded GND plane in logic unit/bottom PCB.
- Improved all copper areas.
- Added an additional 12V and GND pin directly connected to the PSU.
- Exchanged the LM2575T with an MC7805CTG.
  - This has led to a decrease in layout complexity and amount of parts.

![PCB Rev2](https://i.imgur.com/CQF3Qro.png)