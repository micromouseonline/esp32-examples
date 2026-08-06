# wifi-soft-ap

ESP32 WiFi soft-AP demo. Copied from `template`; not yet built out beyond the base blinky app.

## Structure

- `application.cpp` -- all app logic: task definition, task creation, serial init
- `application.h` -- declares `setup()` and `loop()`
- `platformio.ini` -- envs extending base configs in `../base-boards.ini`
- `../common/` -- shared board config, status LED abstraction, wifi manager, secrets

## Key conventions

- Board identity is set via build flags (`BOARD_S3_ZERO`, `BOARD_C3_SUPER_MINI`, `BOARD_C6_SUPER_MINI`) defined in `shared_boards.ini`; `board-config.h` maps these to pin numbers and LED driver types.
- GPIO 18/19 carry native USB D-/D+; Serial uses USB CDC (`ARDUINO_USB_CDC_ON_BOOT=1`), so UART0 pins are free.
- Do not modify files under `.pio/` or `../common/` without checking impact on other projects in the template.
