# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build and flash

PlatformIO project targeting the ESP32-C3. One environment is defined in [platformio.ini](platformio.ini):

- `wifi-scanner-esp32-c3` -- ESP32-C3 DevKitM-1 (configured as C3 Super Mini)

```bash
# Build
pio run -e wifi-scanner-esp32-c3

# Build and upload
pio run -e wifi-scanner-esp32-c3 --target upload

# Serial monitor (460800 baud)
pio device monitor -e wifi-scanner-esp32-c3

# Regenerate VSCode IntelliSense config after adding/removing libraries
pio init --ide vscode
```

Upload/monitor port auto-detected via USB CDC. If needed, uncomment `upload_port` / `monitor_port` in `platformio.ini`.

## Key build flags (wifi-scanner-esp32-c3)

- `LED_PIN=8` -- onboard LED GPIO (also resolved via `STATUS_LED` in `board_config.h`)
- `BOARD_C3_SUPER_MINI` -- selects the C3 Super Mini pin profile in `board_config.h`
- `ARDUINO_USB_MODE=1` / `ARDUINO_USB_CDC_ON_BOOT=1` -- Serial goes over USB CDC, not UART
- `-Wl,--allow-multiple-definition` -- suppresses linker errors from duplicate symbols in Arduino WiFi libs
- CPU: 160 MHz, 4 MB flash, QIO mode
- `CORE_DEBUG_LEVEL` commented out in ini; options: 5 verbose ... 0 none

## Source layout

- `wifi-beacon-spammer.ino` -- Arduino entry point; calls `app_setup()` / `app_loop()`
- `application.cpp` -- WiFi scan logic: scans every 2 s, prints SSID/RSSI/channel/encryption to Serial
- `application.h` -- declares `app_setup()` / `app_loop()`
- `../common/board-config.h` -- multi-board LED pin mapping (`STATUS_LED`)

## Local library

- `lib/esp32-info` -- utility to print chip/flash/heap info to Serial (`getInfo()`, `printFactoryMac()`, `printFlashChipMode()`); not currently called in `application.cpp`

## Do not read or modify

- `.pio/` -- generated build artifacts and downloaded library dependencies
