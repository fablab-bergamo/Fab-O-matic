# PCB information

* This folder contains Gerber file, BOM, schematics.
* Project has been sponsored by EasyEDA in the OpenHWLab [Project homepage](https://oshwlab.com/pascal.brunot/rfid-arduino-fablab-bg-v2_2024-04-01_14-22-11)

## Revision 1.1

Changes from rev 1.0:
- Fixed R9 connection to FB pin instead of 5V. This makes the 6.5-40V to 5V buck converter functional.


## Revision 1.0

This version is easy to order as pre-assembled on JLCPCB. For 10 boards, total cost (as per April 2024) is below 200 EUR total (not considering modules and boxes). Soldering map is available is you want to assemble yourself : <code>rev 1.0/soldering_map.html</code>

⚠ Errata in this revision ⚠

R9 shall be connected between GND and FB pin, because FB shall be the center of the voltage divider. In this version, R9 was connected between GND and 5V.
This can be fixed by rotating R9 by 90° and solder it to R11.

![image](https://github.com/fablab-bergamo/fab-o-matic/assets/6236243/046bd7b5-0c89-4604-947c-9c6126ae2a86)

Changes from rev 0.4:

* Added an integrated a buck converter for 5V generation instead of a module
* Replaced USB B port with a USB C
* SMD components wherever possible (Q1, USB, variable resistor, buttons)
* Components update based on [https://github.com/yaqwsx/jlcparts] availabilities
* ESP32-S3-WROOM-1-N4R8 selected for availability (N4 was enough).
* More compact board design
* Added a color printer PCB front panel design + layout adjustments to accomodate front panel design

## Revision 0.4

This version has no known errata.

Changes:

* Fixed errata of revision 0.3
* Removed 220u CAP after testing, it is not required.
* Moved the design to EASYEDA Pro (free) for future versions

## Assembly instructions

An interactive PCB map is available under <code>rev 0.4/soldering_map.html</code>

Order is important:

1. (front side) Solder ESP32S3-WROOM-1 module. Check for shorts.
2. (front side) Solder all others SMD components
3. (front side) Solder through-hole components (terminal blocks, relay, Q1, USB, capacitor)
4. (reverse side) Solder NeoPixel, 3V buzzer, female pins for RFID / LCD modules, defaults switch.

Test procedure

1. Continuity check : power lines (3V3, 5V) to ESP32, LDO, LCD/RFID modules.
2. Terminal blocks: NC and COM shall be continuous. NO and COM shall not be continuous.
3. Check for shorts between 5V and - ; 3V3 and - ; + and - pins (there should not be any)
4. Connect +5V to the 5V terminal block and GND to - terminal block
5. Check that voltage on 3V3 pin is 3.3V
6. Disconnect external power.
7. Connect USB cable to PC
8. Check voltages on 5V and 3V3 pins
9. Push BOOT, Push RESET, Release RESET, release BOOT
10. ESP32S3 shall be recognized on PC as a serial port

💡 A fresh ESP32-S3 will be in a reboot cycle before the first flash. It is normal to have the USB connect/disconnect sounds at this stage.

11. Upload software with PlatformIO
12. Disconnect USB
13. Connect modules (including buck converter if needed)
14. Connect USB
15. Open Monitor and check for errors.

## Revision 0.3

⚠ Errata in this revision ⚠

* Q1 base shall be pulled up by a 5-10kohm resistor to 3V3 to avoid relay flickering at boot (while ESP32 has not pulled up the pin yet).

Changes:

* fixed errata of REV 0.2 (2N3906 for through hole/smd + RW pin)
* smaller footprints for capacitors
* added component values on silkscreen
* slight repositioning of RFID module to avoid overlap with default switch
* added 220 uF capacitor on 5V rail for LCD
* removed terminal blocks IN+/OUT+ as there are enough connection ports on the board
* added input protections with Zener+resettable 100mA fuses.

Link to project: <https://oshwlab.com/pascal.brunot/rfid-arduino-fablab-bg_copy>

## Revision 0.2

Fully working with some issues requiring rework.

Assembled front:

![image](https://github.com/fablab-bergamo/rfid-arduino/assets/6236243/09caf084-0176-4699-97ed-f7ce86ed41d8)

Assembled back:

![image](https://github.com/fablab-bergamo/rfid-arduino/assets/6236243/1d27732c-2e89-48d1-b3e3-e541e1a960fa)

⚠ Errata in this revision ⚠

* Relay requires more current than ESP32 can provide. A PNP transistor can be soldered next to the terminal blocks to solve the problem. Connect Base on K pin, Emitter on 3V3 pin and Collector to coil (+). The track between CMD and Coil(+) passing below the relay must be cut. Relay takes 70 mA approx. The pin must be configured active_low.

![image](https://github.com/fablab-bergamo/rfid-arduino/assets/6236243/75bbd24b-5090-47d4-a61c-b619b16dba42)

* (NOT IN THE PHOTO) : a 10k resistor shall be added too between Base and 3V3 to avoid flickering at boot. Fixed in rev 0.4.

* Buzzer must be 3V3 low current. However common Arduino buzzers require 5V and high current. When using this model, the ESP32 resets due to voltage dropping. There are two solutions: use a 3 pin Arduino buzzer with VIN/GND/Signal (the PCB is compatible with such buzzers), or use a low current buzzer.

* LCD RW pin must be grounded (fix with soldering wire). Otherwise the LCD does not accept commands from ESP32.

![image](https://github.com/fablab-bergamo/rfid-arduino/assets/6236243/5ca70f12-2f2f-4102-b2e6-33797c79def8)

Changes during assembly:

* Variable resistor for brightness adjustment replaced with 1k resistor
* C2 and C3 were not mounted because their pads were damaged during ESP32 soldering.
* Zener protection diode D7 and F1 not mounted (overvoltage protection)
* D4 replaced with a wire (reverse polarity protection) because voltage drop was too high (need 0.3V)
