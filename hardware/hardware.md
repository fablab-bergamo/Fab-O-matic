# PCB version

## Revision 0.2

Fully working with some issues requiring rework.

Assembled front:

![image](https://github.com/fablab-bergamo/rfid-arduino/assets/6236243/09caf084-0176-4699-97ed-f7ce86ed41d8)

Assembled back:

![image](https://github.com/fablab-bergamo/rfid-arduino/assets/6236243/1d27732c-2e89-48d1-b3e3-e541e1a960fa)


Errata in this revision:

- Relay requires more current than ESP32 can provide. A PNP transistor can be soldered next to the terminal blocks to solve the problem. Connect Base on K pin, Emitter on 3V3 pin and Collector to coil (+). The track between CMD and Coil(+) passing below the relay must be cut. Relay takes 70 mA approx. The pin must be configured active_low.

![image](https://github.com/fablab-bergamo/rfid-arduino/assets/6236243/75bbd24b-5090-47d4-a61c-b619b16dba42)


- Buzzer must be 3V3 low current. However common Arduino buzzers require 5V and high current. When using this model, the ESP32 resets due to voltage dropping. There are two solutions: use a 3 pin Arduino buzzer with VIN/GND/Signal (the PCB is compatible with such buzzers), or use a low current buzzer.

- LCD RW pin must be grounded (fix with soldering wire). Otherwise the LCD does not accept commands from ESP32.

![image](https://github.com/fablab-bergamo/rfid-arduino/assets/6236243/5ca70f12-2f2f-4102-b2e6-33797c79def8)

Changes during assembly:
- Variable resistor for brightness adjustment replaced with 1k resistor
- C2 and C3 were not mounted because their pads were damaged during ESP32 soldering. 
- Zener protection diode D7 and F1 not mounted (overvoltage protection)
- D4 replaced with a wire (reverse polarity protection) because voltage drop was too high (need 0.3V)

## Revision 0.3

Changes:
- fixed errata of REV 0.2 (added 2N3906 for through hole/smd)
- smaller footprints for capacitors
- added values on silkscreen for all components
- slight repositioning of RFID module to avoid overlap with default switch
- added 220 uF capacitor on 5V rail for LCD
- removed terminal blocks IN+/OUT+ as there are enough connection ports on the board

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
