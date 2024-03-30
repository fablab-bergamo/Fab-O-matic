# PCB version

## Revision 0.2

Fully working with some issues requiring rework.

Errata in this revision:

- Relay requires more current than ESP32 can provide. A PNP transistor can be soldered next to the terminal blocks to solve the problem. Connect Base on K pin, Emitter on 3V3 pin and Collector to coil (+). The track between CMD and Coil(+) passing below the relay must be cut. Relay takes 70 mA approx. The pin must be configured active_low.

- Buzzer must be 3V3 low current. However common Arduino buzzers require 5V and high current. When using this model, the ESP32 resets due to voltage dropping. There are two solutions: use a 3 pin Arduino buzzer with VIN/GND/Signal (the PCB is compatible with such buzzers), or use a low current buzzer.

- LCD RW pin must be grounded (fix with soldering wire). Otherwise the LCD does not accept commands from ESP32.

## Revision 0.3

Changes:
- fixed errata of REV 0.2 (added 2N3906 for through hole/smd)
- smaller footprints for capacitors
- added values on silkscreen for all components
- slight repositioning of RFID module to avoid overlap with default switch

# Assembly instructions

Order is important:
- (front side) Solder ESP32S3-WROOM-1 module. Check for shorts. 
- (front side) Solder all others SMD components
- (front side) Solder through-hole components (terminal blocks, relay, Q1, USB, capacitor)
- (reverse side) Solder pins for RFID / LCD modules

Test procedure
- Continuity check : power lines (3V3, 5V) to ESP32, LDO, LCD/RFID modules.
- Terminal blocks: NC and COM shall be continuous. NO and COM shall not be continuous.
- Check for shorts between 5V and - ; 3V3 and - ; + and - pins (there should be any)
- Connect +5V to the 5V terminal block and GND to - terminal block
- Check that voltage on 3V3 pin is 3.3V
- Disconnect external power. 
- Connect USB cable to PC
- Check voltages on 5V and 3V3 pins
- Push BOOT, Push RESET, Release RESET, release BOOT
- ESP32S3 shall be recognized on PC as a serial port
- Upload software with PlatformIO 
- Disconnect USB
- Connect modules
- Connect USB
- Open Monitor and check for errors.
