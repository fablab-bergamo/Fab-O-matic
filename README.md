# rfid-arduino

Build status : [![PlatformIO CI](https://github.com/fablab-bergamo/rfid-arduino/actions/workflows/platformio.yml/badge.svg)](https://github.com/fablab-bergamo/rfid-arduino/actions/workflows/platformio.yml)

## Testing on WOKWI

- Download latest esp32-wokwi.zip file from Actions / platformio.yml / Artifacts
- Extract esp32-wokwi.bin file from artifact ZIP
- Open WOKWI Circuit https://wokwi.com/projects/363448917434192897
- In code editor, press F1 > Upload firmware ... and pick the esp32-wokwi.bin
- The simulator simulates random RFID tags from whitelist.

## Hardware requirements

- ESP32 or ESP32-S3
- RFID reader (using mfrc522 compatible chip)
- LCD driver (using Hitachi HD44780 compatible chip)
- 3.3V Relay
- 3.3V Buzzer
- A LED or NeoPixel
- RFID tags or cards for user authentication

## Other requirements

- MQTT Broker on WiFi network. Board can work in offline mode with whitelisted RFID tags.
- In the shelly branch an experimental test with smart plugs (Shelly) over MQTT is available.

## Build environment

- Language: C++17 with ArduinoFramework
- IDE: VSCode + Platform.io extension
- To build, rename secrets.h.example to secrets.h. CMakeList.txt is generated from platform.io
- To use hostname for MQTT server. mDNS is used by the Arduino stack built over ESP-IDF 4.4. When Arduino core for ESP will move to ESP-IDF 5.0+ an additional dependency to mDNS will be needed as mDNS is now an independent component.

## Configuration

- See pins.h to set the GPIO pins for LCD parallel interface, relay, buzzer and RFID reader SPI interface.
- See conf.h to configure LCD dimensions, timeouts, debug logs and some behaviours (e.g. time before to power off the machine)
- See secrets.h to configure network SSID/Password, MQTT topics/credentials/server, whitelisted RFID tags (without server connection)
