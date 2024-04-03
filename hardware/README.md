# PCB information

## Revision 0.2

Fully working with some issues requiring rework.

Assembled front:

![image](https://github.com/fablab-bergamo/rfid-arduino/assets/6236243/09caf084-0176-4699-97ed-f7ce86ed41d8)

Assembled back:

![image](https://github.com/fablab-bergamo/rfid-arduino/assets/6236243/1d27732c-2e89-48d1-b3e3-e541e1a960fa)

⚠ Errata in this revision ⚠

* Relay requires more current than ESP32 can provide. A PNP transistor can be soldered next to the terminal blocks to solve the problem. Connect Base on K pin, Emitter on 3V3 pin and Collector to coil (+). The track between CMD and Coil(+) passing below the relay must be cut. Relay takes 70 mA approx. The pin must be configured active_low.

![image](https://github.com/fablab-bergamo/rfid-arduino/assets/6236243/75bbd24b-5090-47d4-a61c-b619b16dba42)

* Buzzer must be 3V3 low current. However common Arduino buzzers require 5V and high current. When using this model, the ESP32 resets due to voltage dropping. There are two solutions: use a 3 pin Arduino buzzer with VIN/GND/Signal (the PCB is compatible with such buzzers), or use a low current buzzer.

* LCD RW pin must be grounded (fix with soldering wire). Otherwise the LCD does not accept commands from ESP32.

![image](https://github.com/fablab-bergamo/rfid-arduino/assets/6236243/5ca70f12-2f2f-4102-b2e6-33797c79def8)

Changes during assembly:

* Variable resistor for brightness adjustment replaced with 1k resistor
* C2 and C3 were not mounted because their pads were damaged during ESP32 soldering.
* Zener protection diode D7 and F1 not mounted (overvoltage protection)
* D4 replaced with a wire (reverse polarity protection) because voltage drop was too high (need 0.3V)

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

## Assembly instructions

An interactive PCB map is available under <code>rev 0.3/soldering_help.html</code>

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

>Some troubleshooting steps if USB is not recognized:
>
>* Check ESP32 reset pin (shall be high) + voltages
>* Check USB D+/D- tracks continuity in diode mode from ESP32 to usb port.
>* Check USB port. Excessive heat can make them bad.

11. Upload software with PlatformIO
12. Disconnect USB
13. Connect modules (including buck converter if needed)
14. Connect USB
15. Open Monitor and check for errors.
