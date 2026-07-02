# blinky-freertos

Minimal FreeRTOS blink demo for ESP32 multi-target template.

## Structure

- `blinky-freertos.ino` -- Arduino entry point; delegates to `app_setup()`/`app_loop()`
- `application.cpp` -- all app logic: task definition, task creation, serial init
- `application.h` -- declares `app_setup()` and `app_loop()`
- `platformio.ini` -- three envs extending base configs in `../shared_boards.ini`
- `../common/` -- shared board config, status LED abstraction, wifi manager, secrets

## Key conventions

- Board identity is set via build flags (`BOARD_S3_ZERO`, `BOARD_C3_SUPER_MINI`, `BOARD_C6_SUPER_MINI`) defined in `shared_boards.ini`; `board-config.h` maps these to pin numbers and LED driver types.
- GPIO 18/19 carry native USB D-/D+; Serial uses USB CDC (`ARDUINO_USB_CDC_ON_BOOT=1`), so UART0 pins are free.
- Do not modify files under `.pio/` or `../common/` without checking impact on other projects in the template.
