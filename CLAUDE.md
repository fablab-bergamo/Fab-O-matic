# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

Fab-O-matic is an ESP32 firmware (PlatformIO + Arduino framework, C++23) for an RFID card reader that controls access to Fab Lab machines. Boards talk MQTT to a backend server (separate repo: fablab-bergamo/rfid-backend) and switch machine power via a relay or an MQTT switch (e.g. Shelly). The `hardware/` folder contains PCB designs only — all firmware lives in `src/`, `include/`, `conf/`.

## Build and test commands

First build requires `conf/secrets.hpp` — copy it from `conf/secrets.hpp.example`.

```shell
pio run -e wokwi                  # build (wokwi env needs no hardware; simulation flags on)
pio run -e hardware-rev0-en_US    # build a release hardware image
pio test -e wokwi --without-testing --without-uploading -f test_logic   # build one test image
pio test --environment esp32-s3 -f test_logic --verbose                 # run one test on attached hardware
pio check --environment wokwi --fail-on-defect=high                     # clang-tidy lint (as CI runs it)
```

Tests (Unity framework, in `test/test_*`) cannot run on the host: they execute either on real hardware over USB, or in the Wokwi ESP32 emulator. CI (tests.yml) builds each test image with `pio test -e wokwi --without-testing --without-uploading -f <test_name>` and runs it with wokwi-cli (requires a Wokwi token). Test suites: test_mqtt, test_logic, test_savedconfig, test_tasks, test_chrono.

## PlatformIO environments

Each environment selects GPIO pins via a `PINS_xx` define (mapped in `conf/pins.hpp`) and language via `FABOMATIC_LANG_xx_xx`. Key environments:

- `wokwi` — emulator/demo/tests; sets `MQTT_SIMULATION=true` and `RFID_SIMULATION=true`
- `hardware-rev0-it_IT` / `hardware-rev0-en_US` — release builds for real PCBs (ESP32-S3)
- `esp32-devboard`, `wrover-kit-it_IT` — prototyping/debugging boards

When `RFID_SIMULATION` is set, `RFIDWrapper<MockMrfc522>` replaces the real driver and fakes card taps from the whitelist; when `MQTT_SIMULATION` is set, a `MockMQTTBroker` runs in a separate thread on the ESP32. The tests rely on these mocks.

## Architecture

- **Cooperative scheduler, no RTOS tasks**: `Tasks::Scheduler`/`Tasks::Task` (`include/Tasks.hpp`). `src/main.cpp` declares all periodic tasks (RFID polling, MQTT refresh, watchdog, poweroff/logoff checks, LED, etc.) as namespace-scope objects that self-register with the scheduler; `loop()` just calls the scheduler.
- **Global singletons** live in namespace `fabomatic::Board` (`include/globals.hpp`), instantiated only by `main.cpp` — `rfid`, `lcd`, `logic`, `scheduler` (+ `broker` under simulation).
- **`BoardLogic`** (`include/BoardLogic.hpp`) is the central state machine (`Status` enum: LoggedIn, MachineFree, MaintenanceNeeded, …). It owns the `Machine`, `AuthProvider`, `FabBackend` and drives LCD/LED/buzzer/relay from status changes.
- **`FabBackend`** handles MQTT request/response with the backend. Message types are the `Query`/`Response` class hierarchies in `include/MQTTtypes.hpp` (payloads are hand-built JSON in `src/MQTTtypes.cpp`); `BufferedMsg` queues important events while the network is down and replays them later.
- **`AuthProvider`** authenticates a card UID: backend first, then `CachedCards` (recently seen cards persisted to flash), then the compile-time `whitelist` in `conf/secrets.hpp`.
- **`SavedConfig`** persists settings (WiFi, MQTT, machine id, cached cards) as JSON in flash; the `magic_number` field versions the layout — bump it when changing persisted fields, which discards stored settings on next boot.
- **`RFIDWrapper<Driver>`** is a template (implementation in `src/RFIDWrapper.tpp`) over `Mrfc522Driver` or `MockMrfc522`, chosen at compile time.
- **Configuration** is compile-time in `conf/`: `conf.hpp` (timeouts, behaviours), `pins.hpp` (per-board GPIO tables), `secrets.hpp` (credentials + RFID whitelist; gitignored). Some settings (grace period, autologoff) can be overridden at runtime by the backend, and WiFi/MQTT-broker/machine-id by the WiFiManager captive portal (CONFIG button long-press).
- **Localization**: `include/language/` has one header per locale selected by `FABOMATIC_LANG_xx_xx`; to add a language, add a header and update `lang.hpp`.
- **Build scripts**: `tools/git_version.py` (pre) injects the git version; `tools/metrics_firmware.py` (post) produces firmware size metrics.

## Gotchas

- Repository files use LF line endings, but `.gitattributes` has no EOL normalization and git treats the PDFs/DXF under `hardware/` as text. Editing from Windows tools can silently flip the whole tree to CRLF (and corrupt the binaries) — if `git status` suddenly shows ~90 modified files, check for that before committing (`git diff --stat --ignore-cr-at-eol` shows the real changes).
- `include/globals.hpp` defines (not just declares) the `Board` globals, so it must be included by exactly one translation unit (`main.cpp`).
