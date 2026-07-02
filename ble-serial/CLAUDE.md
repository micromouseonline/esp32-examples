# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build and flash

PlatformIO project targeting the ESP32-C3 Super Mini board.

```bash
# Build
pio run -e ble-serial-esp32-c3

# Build and upload
pio run -e ble-serial-esp32-c3 --target upload

# Serial monitor (460800 baud)
pio device monitor -e ble-serial-esp32-c3

# Regenerate VSCode IntelliSense config after adding/removing libraries
pio init --ide vscode
```

Upload/monitor port auto-detected via USB CDC. If needed, uncomment `upload_port` / `monitor_port` in `platformio.ini`.

## Key build flags (ble-serial-esp32-c3)

- `LED_PIN=8` -- onboard LED GPIO (not used directly in application.cpp; LEDs are GPIO1/GPIO2)
- `BOARD_C3_SUPER_MINI` -- selects C3 Super Mini pin profile in `board_config.h`
- `ARDUINO_USB_MODE=1` / `ARDUINO_USB_CDC_ON_BOOT=1` -- Serial goes over USB CDC, not UART
- `-Wl,--allow-multiple-definition` -- suppresses linker errors from duplicate symbols
- CPU: 160 MHz, 4 MB flash, QIO mode
- `CORE_DEBUG_LEVEL` commented out in ini; options: 5 verbose ... 0 none


## Architecture

`ble-serial.ino` is the Arduino entry point; all logic is in `src/application.cpp`.

### Board identity (`src/board-id.h`)

`get_board_name()` reads the last 4 bytes of the eFuse MAC address and looks them up in a static table of known boards (GATE_01..GATE_05). Falls back to a hex string (`0xXXXXXXXX`) if unrecognised. The BLE device name is set from this at startup.

### BLE: Nordic UART Service (NUS)

Device name: result of `get_board_name()`. MTU: 517.

| Role | UUID | Property |
|------|------|----------|
| Service | `6E400001-B5A3-F393-E0A9-E50E24DCCA9E` | -- |
| RX characteristic (phone -> device) | `6E400002-...` | WRITE |
| TX characteristic (device -> phone) | `6E400003-...` | NOTIFY |

On disconnect, advertising restarts automatically via `MyServerCallbacks::onDisconnect`.

### FreeRTOS tasks

| Task | Pin | Interval | Stack |
|------|-----|----------|-------|
| `blink_blue` | GPIO1 | 431 ms | 2048 |
| `blink_yellow` | GPIO2 | 359 ms | 2048 |
| `ble_sender` | -- | 100 ms poll / queue-blocked | 3072 |

`app_loop()` acts as the sensor source: pushes `millis()/1000.0f` (fake temperature) to `temperature_queue` (capacity 10) every 100 ms. `ble_sender_task` blocks on `xQueueReceive` and notifies the TX characteristic when a client is connected.

## Source layout

- `ble-serial.ino` -- Arduino entry point; calls `app_setup()` / `app_loop()`
- `application.cpp` -- all BLE and FreeRTOS logic
- `application.h` -- declares `app_setup()` / `app_loop()`
- 
- `board-id.h` -- MAC-to-name lookup table and `get_board_name()` / `get_chip_id()`
- `board_config.h` -- multi-board LED pin mapping (`STATUS_LED`); present but not included by application.cpp

## Local library

- `lib/esp32-info` -- `printFactoryMac()`, `printFlashChipMode()`, `getInfo()`; header included in application.cpp but no functions called

## Dependencies

- `h2zero/NimBLE-Arduino@^1.4.1` -- lightweight NimBLE stack (declared in `platformio.ini`, installed to `.pio/libdeps/`)

## Do not read or modify

- `.pio/` -- generated build artifacts and downloaded library dependencies
