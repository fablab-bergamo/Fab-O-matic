# rfid-arduino

[![PlatformIO CI](https://github.com/PBrunot/rfid-arduino/actions/workflows/platformio.yml/badge.svg)](https://github.com/PBrunot/rfid-arduino/actions/workflows/platformio.yml)

## Hardware requirements

- ESP32
- mfrc522 RFID reader
- LCD driver

## Build environment

- Language: C++ with ArduinoFramework
- IDE: VSCode + Platform.io extension
- Rename secrets.h.example to secrets.h

## Configuration

- See pins.h to set the GPIO pins for LCD, relay outputs, RFID reader.
- See conf.h to configure LCD dimensions and whiteliste
- See secrets.h to whitelist RFID tags without server connection
