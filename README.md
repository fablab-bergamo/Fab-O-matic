# RFID-arduino

Build status : [![PlatformIO CI](https://github.com/fablab-bergamo/rfid-arduino/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/fablab-bergamo/rfid-arduino/actions/workflows/build.yml)
Test suite : [![Test suite](https://github.com/PBrunot/rfid-arduino-copy/actions/workflows/tests.yml/badge.svg)](https://github.com/PBrunot/rfid-arduino-copy/actions/workflows/tests.yml)

## What is this project?

* A low-cost RFID-card reader to control machine usage in a Fab Lab environment.

* Together with the [backend project](https://github.com/fablab-bergamo/rfid-backend) which can run on a Raspberry Pi Zero, it manages user authentication, track machine usage / maintenance needs.

* Assembled version:

![image](https://github.com/fablab-bergamo/rfid-arduino/assets/6236243/9898c6a5-cc16-4479-851a-b326ad31a4d6)

* It uses MQTT to talk to the backend. Machine control is achieved through an external relay.

* View hardware project (WORK IN PROGRESS) on EASYEDA : [Electrical scheme, BOM, PCB](https://oshwlab.com/pascal.brunot/rfid-arduino-fablab-bg)

## Hardware components

* ESP32, ESP32-S2 or ESP32-S3 chips
* WiFi connection to the backend project
* RFID reader (using mfrc522 compatible chip)
* LCD driver (using Hitachi HD44780 compatible chip)
* 3.3V Relay (or [Shelly](https://www.shellyitalia.com/shelly-plus-1-mini-gen3/) MQTT device)
* 3.3V Buzzer
* A LED or NeoPixel
* RFID tags or cards for user authentication

## PCB Details

* See <code>hardware\README.md</code> folder for Gerber files and schematics.
* First version uses modules for RFID, LCD, Power DC-DC buck converter and ESP32-S3 and a mix of SMD and through-hole components.
* Cost estimate per board < 30 eur: PCB around 1 eur, components 11 eur + modules (LCD,RFID,Buck,Relay) approx 8 eur

## Build environment

* Language: C++20 with ArduinoFramework for ESP32
* IDE: VSCode + Platform.io extension as a minimun
* To build, rename <code>conf/secrets.hpp.example</code> to <code>conf/secrets.hpp</code>.

> Platform IO can be used from command-line without VSCode <code>pio run</code>

## Testing Suite

* A set a test scripts based on Platformio+Unity is included in the project.
* There are two ways to run the tests:

1. Use real hardware (esp32s3, esp32 wroverkit) connected over USB with Platform.io command

```shell
pio test --environment esp32-s3
```

2. Use Wokwi-CLI with test images built by Platform.io. It requires a wokwi access token (free as per Jan 2024). The Github action "tests.yml" uses this mechanism.

## DEMO - View it in the browser

* Download latest <code>esp32-wokwi.zip</code> file from Github Actions / platformio.yml / Artifacts
* Extract <code>esp32-wokwi.bin</code> file from artifact ZIP
* Open WOKWI Circuit [link](https://wokwi.com/projects/363448917434192897)
* In code editor, press F1 > Upload firmware ... and pick the <code>esp32-wokwi.bin</code> file

![image](https://github.com/fablab-bergamo/rfid-arduino/assets/6236243/5c41092e-f8bf-451a-95ec-8dc6d7e07824)

* When the preprocessor constants <code>MQTT_SIMULATION</code> or <code>RFID_SIMULATION</code> are set to true:
  * RFID chip is replaced with a mockup simulating random RFID tags from whitelist from time to time (<code>MockMrfc522</code> class).
  * A simple MQTT broker (<code>MockMQTTBroker</code> class) is run in a separate thread on esp32s2

## Configuration steps (/conf folder)

* See <code>conf/pins.hpp</code> to set the GPIO pins for LCD parallel interface, relay, buzzer and RFID reader SPI interface.
* See <code>conf/conf.hpp</code> to configure LCD dimensions, timeouts, debug logs and some behaviours (e.g. time before to power off the machine)
* See <code>conf/secrets.hpp</code> to configure network SSID/Password credentials, MQTT credentials and whitelisted RFID tags
  
  * A configuration portal based on WiFiManager allows to configured WiFi credentials, MQTT Broker address and Shelly topic (facultative). This makes editing <code>conf/conf.hpp</code> required only for MQTT Broker credentials settings.

> To add a white-listed RFID card, edit the tuples list <code>whitelist</code>. These RFID tags will be always authorized, even without server connection.

```c++
  static constexpr WhiteList whitelist /* List of RFID tags whitelisted, regardless of connection */
      {
          std::make_tuple(0xAABBCCD1, FabUser::UserLevel::FABLAB_ADMIN, "ABCDEFG"),
          ...
          std::make_tuple(0xAABBCCDA, FabUser::UserLevel::FABLAB_USER, "USER1")
      };
```

> To map the switch Shelly control MQTT topic, edit the <code>machine_topic</code> under <code>conf::default_config</code> in conf.hpp file

```c++
namespace conf::default_config
{
  static constexpr std::string_view mqtt_server = "127.0.0.1";
  static constexpr std::string_view machine_topic = "shelly/command/switch:0"; // Set to empty to disable Shelly integration
  static constexpr MachineID machine_id{1};
  static constexpr std::string_view machine_name = "MACHINE1";
  static constexpr MachineType machine_type = MachineType::LASER;
}
```

## Configuration guide - debugging with Wokwi ESP32 emulator integrated with VSCode

This is a facultative but very helpful setup to shorten the development workflow.

* Install ESP-IDF extension, Wokwi extension with community evaluation license

> Make sure ESP-IDF platform is esp32s2 (used by wokwi Platformio environment)

* Build PlatformIO wokwi project, and start simulation with command <code>Wokwi: Start Simulator</code>, you shall see the program running:

![image](https://github.com/fablab-bergamo/rfid-arduino/assets/6236243/dfdf33e3-74ac-4246-9c92-4631e0009034)

> See files wokwi.toml and diagram.json

* To configure GDB connection to the Wokwi simulator, edit .vscode\launch.json and add the following fragment inside <code>"configurations"</code> array

```json
  {
    "name": "Wokwi GDB",
    "type": "cppdbg",
    "request": "launch",
    "program": "${workspaceFolder}/.pio/build/wokwi/firmware.elf",
    "cwd": "${workspaceFolder}",
    "MIMode": "gdb",
    "miDebuggerPath": "${command:espIdf.getXtensaGdb}",
    "miDebuggerServerAddress": "localhost:3333"
  }
```

* To test debugging, first start Wokwi with <code>Wokwi: Start Simulator and wait for debugger</code>
* You can then run the application, setup breakpoints, inspect variables from the Wokwi debugger:

![image](https://github.com/fablab-bergamo/rfid-arduino/assets/6236243/55f926b5-eec8-49d9-b217-628e07f7e3b8)

## OTA procedure

* Edit platform.io configuration file for the build with the following under the right environmnet

```ini
upload_protocol = espota
upload_port = IP_ADDRESS_HERE or mDNS_NAME.local
```

* Set serial port to AUTO in VSCODE
* Build & Deploy from VSCode. Upload takes 1-2 minutes. Board will reboot automatically when the machine is idle.

## Version history

| Version | Date | Notable changes |
|--|--|--|
|none  | 2021 | Initial version |
|0.1.x | August 2023 | Implemented MQTT communication with backend |
|0.1.x | December 2023 | Added test cases, mqtt broker simulation |
|0.2.x | January 2024 | Added over-the-air updates, WiFi portal for initial config, first deploy |
|0.3.x | February 2024 | Added factory defaults button, power grace period config from backend, PCB draft |
|0.4.x | March 2024 | 1st PCB manufactured (rev0.2), FW +IP address announced over MQTT |
|0.5.x | April 2024 | Fully tested on PCB rev0.2 |
