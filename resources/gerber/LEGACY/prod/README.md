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

### Iteration 2 - 06.04.2021
#### nixie_clock_prod_rev2.zip
The same principle as revision 1, this time with considerable changes though:
- Increased clearances.
- Changed RTC battery holder to a THT variant.
- Improved cap placement by RTC IC.
- Removed duplicate tracks - mostly GND.
- Expanded GND plane in logic unit.
- Improved all copper areas.
- Exposed an additional 12V and GND trace.
- Swapped the LM2575T with an MC7805CTG.
  - Reasons being less layout complexity and components cost.
  - Also added mounting holes for a TO-220 heatsink.  

![PCB Rev2](https://i.imgur.com/SypHGxR.png)

### Iteration 3 - 06.05.2021
#### nixie_clock_prod_rev3.zip
- Rounded board outline edges.
- Fix power rail for shift registers.
- Added vias to ESP thermal pad.
- Removed vias from mounting holes.
- Included clearance DRC entry for mounting holes.
  - This helps making the copper areas less blocky.
- Added heatsink mounting holes to GND net.
- Improved silkscreen element placings.
- Downgraded input voltage of 12VDC to 5VDC.
- Powered ALL components with down-stepped 3.3V.
- Added Micro-USB socket for 5V power supply.

![PCB Rev3](https://i.imgur.com/q4c0Rdl.png)


### Iteration 4 - 17.05.2021
#### nixie_clock_prod_rev4.zip
This iteration fixes many major issues of revision 3 with some additions.
- Fixed voltage supply for K155D1s.
- Fixed pin 37 of ESP32 being connected to GND (SPI Master).
- Fixed GND of 5V supply (THT, not USB socket)
- Fixed button pins.
- Exposed BOOT, REST and GND pins for ESP32 programming.
- Added SMD LED for IO16 of ESP32.
- Added jumper pins for IO17 of ESP32 (Factory reset).

![PCB Rev4](https://i.imgur.com/97tArDy.png)