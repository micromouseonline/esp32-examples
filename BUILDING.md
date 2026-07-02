# Building the ESP32 Multi-Target Template

This repository contains a collection of ESP32 tutorial projects. Each project lives in its own
subdirectory and can be built with either PlatformIO (recommended) or the Arduino IDE.

---

## Repository Structure

```
multi-target-template/
  common/               Shared headers (board config, WiFi, status LED, secrets)
  base_boards.ini       Shared PlatformIO board definitions, extended by every project
  blinky-freertos/      Example project
  wifi-scanner/         Example project
  ...
```

Each project directory contains:
- A `.ino` sketch file (Arduino IDE entry point)
- `application.cpp` / `application.h` (all project logic)
- `platformio.ini` (extends `../base_boards.ini`)

---

## Supported Boards

| Board                          | `#define`                    | LED type  |
|--------------------------------|------------------------------|-----------|
| ESP32-S3 Zero                  | `BOARD_S3_ZERO`              | NeoPixel  |
| ESP32-S3 Super Mini            | `BOARD_S3_SUPER_MINI`        | NeoPixel  |
| Freenove ESP32-S3 CYD Touch    | `BOARD_S3_CYD_TOUCH_FREENOVE`| NeoPixel  |
| ESP32-C3 Super Mini            | `BOARD_C3_SUPER_MINI`        | GPIO LED  |
| Seeed XIAO ESP32-C3            | `BOARD_C3_XIAO`              | GPIO LED  |
| Seeed XIAO ESP32-C6            | `BOARD_C6_XIAO`              | GPIO LED  |
| ESP32-C6 Super Mini            | `BOARD_C6_SUPER_MINI`        | NeoPixel  |

---

## Libraries

All libraries are pulled automatically by PlatformIO. For Arduino IDE, install them via
**Sketch > Include Library > Manage Libraries**.

| Library                  | Used for                              |
|--------------------------|---------------------------------------|
| Adafruit NeoPixel        | S3 and C6 boards with RGB LED         |
| U8g2                     | OLED display projects                 |
| NimBLE-Arduino           | BLE projects                          |
| LovyanGFX                | Cerberus gate controller (CYD board)  |

---

## Building with PlatformIO

Open the workspace file `multi-target-template.code-workspace` in VSCode. Each project's
`platformio.ini` already references `../base_boards.ini` and defines one environment per board.

From a terminal inside any project directory:

```bash
# Build all board variants
pio run

# Build one variant
pio run -e <env-name>

# Build and upload
pio run -e <env-name> --target upload

# Serial monitor
pio device monitor -e <env-name>
```

Environment names follow the pattern `<project>-esp32-<board>`, for example:

```
blinky-freertos-esp32-s3-zero
blinky-freertos-esp32-c3
blinky-freertos-esp32-c6-xiao
```

No board selection is needed in source code -- PlatformIO injects the correct `BOARD_XXX`
flag automatically via `build_flags`.

---

## Building with Arduino IDE

### Workspace requirement

The projects in this repository share header files from the `common/` directory using relative
includes (`../common/`). Arduino IDE resolves these relative to the source file location, so
**the full repository must be present** -- you cannot open a single project folder copied out
of the repository.

Always open the sketch file (`.ino`) from within the downloaded repository. Opening
`multi-target-template/wifi-scanner/wifi-scanner.ino` works; opening a copy of
`wifi-scanner/` moved to another location does not.

### Making a project self-contained

If you want to move a project out of the repository:

1. Copy the `common/` directory into the project directory.
2. In `application.h` and `application.cpp`, change every `#include "../common/..."` to
   `#include "common/..."`.

The project will then compile independently with no dependency on the parent directory.

### Board selection

Open the project's `.ino` sketch file. Near the top, inside an `#ifndef PLATFORMIO` guard,
you will find a block like this:

```cpp
#ifndef PLATFORMIO
// Arduino IDE: uncomment ONE board. PlatformIO users: leave all commented.
// #define BOARD_S3_ZERO
// #define BOARD_S3_SUPER_MINI
// #define BOARD_C3_SUPER_MINI
// #define BOARD_C3_XIAO
// #define BOARD_C6_XIAO
// #define BOARD_C6_SUPER_MINI
#endif
```

Uncomment exactly one line to match your board. PlatformIO users leave all lines commented.

### Arduino IDE board settings

Install the board package **esp32 by Espressif Systems** via **Boards Manager** before building.

#### ESP32-S3 Zero (`BOARD_S3_ZERO`)

| Setting          | Value                   |
|------------------|-------------------------|
| Board            | ESP32S3 Dev Module      |
| USB Mode         | Hardware CDC and JTAG   |
| USB CDC On Boot  | Enabled                 |
| Flash Size       | 4MB (32Mb)              |
| Partition Scheme | Default 4MB with spiffs |
| PSRAM            | OPI PSRAM               |

#### ESP32-S3 Super Mini (`BOARD_S3_SUPER_MINI`)

| Setting          | Value                   |
|------------------|-------------------------|
| Board            | ESP32S3 Dev Module      |
| USB Mode         | Hardware CDC and JTAG   |
| USB CDC On Boot  | Enabled                 |
| Flash Size       | 4MB (32Mb)              |
| Partition Scheme | Default 4MB with spiffs |
| PSRAM            | QSPI PSRAM              |

#### Freenove ESP32-S3 CYD Touch (`BOARD_S3_CYD_TOUCH_FREENOVE`)

| Setting          | Value                   |
|------------------|-------------------------|
| Board            | ESP32S3 Dev Module      |
| USB Mode         | Hardware CDC and JTAG   |
| USB CDC On Boot  | Enabled                 |
| CPU Frequency    | 240 MHz                 |
| Flash Size       | 16MB (128Mb)            |
| PSRAM            | OPI PSRAM               |

#### ESP32-C3 Super Mini (`BOARD_C3_SUPER_MINI`)

| Setting         | Value              |
|-----------------|--------------------|
| Board           | ESP32C3 Dev Module |
| USB CDC On Boot | Enabled            |
| Flash Size      | 4MB (32Mb)         |

#### Seeed XIAO ESP32-C3 (`BOARD_C3_XIAO`)

| Setting         | Value        |
|-----------------|--------------|
| Board           | XIAO_ESP32C3 |
| USB CDC On Boot | Enabled      |

#### Seeed XIAO ESP32-C6 (`BOARD_C6_XIAO`)

| Setting         | Value              |
|-----------------|--------------------|
| Board           | ESP32C6 Dev Module |
| CPU Frequency   | 160 MHz            |
| Flash Size      | 8MB (64Mb)         |
| Flash Mode      | QIO                |

#### ESP32-C6 Super Mini (`BOARD_C6_SUPER_MINI`)

| Setting         | Value              |
|-----------------|--------------------|
| Board           | ESP32C6 Dev Module |
| CPU Frequency   | 160 MHz            |
| Flash Size      | 4MB (32Mb)         |
| Flash Mode      | QIO                |
| USB MSC On Boot | Disabled           |

---

## Troubleshooting: USB flashing problems

The USB CDC serial peripherals on ESP32-C3, C6, and S3 boards can leave the host USB stack
in a locked state if the connection is interrupted mid-flash.

**Solution 1 -- reset to bootloader:**
- Press and hold the BOOT (or IO0) button.
- Press and release the EN (or RST) button.
- Release the BOOT button.

This is not always effective if the host-side USB driver is also stuck.

**Solution 2 -- use a USB hub:**
Connect the board through a small USB hub. If the IDE cannot find the board, unplug the
**hub** (not just the board). This forces the OS to tear down and rebuild the entire device
subtree, which simply unplugging the board does not do.
