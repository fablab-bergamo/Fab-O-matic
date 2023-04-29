# rfid-arduino

Build status : [![PlatformIO CI](https://github.com/fablab-bergamo/rfid-arduino/actions/workflows/platformio.yml/badge.svg)](https://github.com/fablab-bergamo/rfid-arduino/actions/workflows/platformio.yml)

## Hardware requirements

- ESP32 or ESP32-S3
- RFID reader (using mfrc522 compatible chip)
- LCD driver (using Hitachi HD44780 compatible chip)
- 3.3V Relay
- 3.3V Buzzer
- RFID tags

## Other requirements

- MQTT Broker on WiFi network. Board can work in offline mode with whitelisted RFID tags.

## Build environment

- Language: C++17 with ArduinoFramework
- IDE: VSCode + Platform.io extension
- To build, rename secrets.h.example to secrets.h. CMakeList.txt is generated from platform.io.

## Configuration

- See pins.h to set the GPIO pins for LCD parallel interface, relay, buzzer and RFID reader SPI interface.
- See conf.h to configure LCD dimensions, timeouts, debug logs and some behaviours
- See secrets.h to configure network SSID/Password, MQTT topics/credentials/server, whitelisted RFID tags (without server connection)
