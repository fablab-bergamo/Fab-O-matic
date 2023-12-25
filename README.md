# rfid-arduino

Build status : [![PlatformIO CI](https://github.com/fablab-bergamo/rfid-arduino/actions/workflows/platformio.yml/badge.svg)](https://github.com/fablab-bergamo/rfid-arduino/actions/workflows/platformio.yml)

## Hardware requirements

- ESP32, ESP32-S2 or ESP32-S3 chip
- RFID reader (using mfrc522 compatible chip)
- LCD driver (using Hitachi HD44780 compatible chip)
- 3.3V Relay (or [Shelly](https://www.shellyitalia.com/shelly-plus-1-mini-gen3/) MQTT device)
- 3.3V Buzzer
- A LED or NeoPixel
- RFID tags or cards for user authentication

## Other requirements

- MQTT Broker on WiFi network. Board can work in offline mode with whitelisted RFID tags.

> Tested with Mosquitto. See the backend side project in [Github rfid-backend](https://github.com/fablab-bergamo/rfid-backend)

- In the <code>shelly</code> branch an experimental test with smart plugs (Shelly) over MQTT is available.

## Build environment

- Language: C++17 with ArduinoFramework for ESP32
- IDE: VSCode + Platform.io extension as a minimun
- To build, rename <code>secrets.h.example</code> to <code>secrets.h</code>.

> Platform IO can be used from command-line with <code>pio run</code>

> CMakeList.txt is generated from platform.io, do not use other build tools directly (ESP, cmake...)

- To use hostname for MQTT server. mDNS is used by the Arduino stack built over ESP-IDF 4.4. When Arduino core for ESP will move to ESP-IDF 5.0+ an additional dependency to mDNS will be needed as mDNS is now an independent component.

### Debugging without hardware with Wokwi ESP32 emulator

This is a facultative but very helpful setup to shorten the development workflow.

- Install ESP-IDF extension, Wokwi extension with evaluation license

> Make sure ESP-IDF platform is esp32s2

- Build PlatformIO wokwi project, and start simulation with command <code>Wokwi: Start Simulator</code>, you shall see the program running:

![image](https://github.com/fablab-bergamo/rfid-arduino/assets/6236243/dfdf33e3-74ac-4246-9c92-4631e0009034)

> See files wokwi.toml and diagram.json

- To configure GDB connection to the Wokwi simulator, edit .vscode\launch.json and add the following fragment inside <code>"configurations"</code> array

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

- To test debugging, first start Wokwi with <code>Wokwi: Start Simulator and wait for debugger</code>
- You can then run the application, setup breakpoints, inspect variables from the Wokwi debugger:

![image](https://github.com/fablab-bergamo/rfid-arduino/assets/6236243/55f926b5-eec8-49d9-b217-628e07f7e3b8)

## Configuration steps

- See <code>pins.h</code> to set the GPIO pins for LCD parallel interface, relay, buzzer and RFID reader SPI interface.
- See <code>conf.h</code> to configure LCD dimensions, timeouts, debug logs and some behaviours (e.g. time before to power off the machine)
- See <code>secrets.h</code> to configure network SSID/Password, MQTT topics/credentials/server, whitelisted RFID tags

Key settings:

> Configure MQTT broker IP and credentials. A sample broker file is available in the rfid-backend project.

```c++
namespace secrets::mqtt
{
  static constexpr std::string_view client = "BOARD";          /* Name of with MQTT client */
  static constexpr std::string_view user = "user";             /* Change with MQTT user */
  static constexpr std::string_view password = "password";     /* Change with MQTT password */
  static constexpr std::string_view server = "192.168.1.1";    /* IP of MQTT broker*/
  static constexpr std::string_view topic = "/machine";        /* Initial part of the topic, machine ID will be added */
  static constexpr std::string_view response_topic = "/reply"; /* Server reply (sub-topic of the full machine topic) */
} // namespace secrets::mqtt
```

> To add a white-listed RFID card, edit the tuples list <code>whitelist</code>. These RFID tags will be always authorized, even without server connection.

```c++
  static constexpr WhiteList whitelist /* List of RFID tags whitelisted, regardless of connection */
      {
          std::make_tuple(0xAABBCCD1, FabUser::UserLevel::FABLAB_ADMIN, "ABCDEFG"),
          ...
          std::make_tuple(0xAABBCCDA, FabUser::UserLevel::FABLAB_USER, "USER1")
      };
```

> To map the switch Shelly control MQTT topic, edit the <code>machine_topic</code> under <code>secrets::machine</code>

```c++
namespace secrets::machine
{
  ...
  static constexpr std::string_view machine_topic = "shelly/command/switch:0";
  ...
} // namespace secrets::machine
```

## DEMO - Testing in the browser

- Download latest <code>esp32-wokwi.zip</code> file from Github Actions / platformio.yml / Artifacts
- Extract <code>esp32-wokwi.bin</code> file from artifact ZIP
- Open WOKWI Circuit https://wokwi.com/projects/363448917434192897
- In code editor, press F1 > Upload firmware ... and pick the <code>esp32-wokwi.bin</code> file

![image](https://github.com/fablab-bergamo/rfid-arduino/assets/6236243/5c41092e-f8bf-451a-95ec-8dc6d7e07824)

- When the preprocessor constant <code>WOKWI_SIMULATION=true</code> is set:
  - RFID chip is replaced with a mockup simulating random RFID tags from whitelist from time to time (<code>MockRFIDWrapper</code> class).
  - A simple MQTT broker (<code>MockMQTTBroker</code> class) is run in a separate thread on esp32s2
