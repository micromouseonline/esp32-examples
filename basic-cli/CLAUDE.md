# basic-cli

Minimal FreeRTOS + CLI demo. Blink LED status + serial line-buffered command dispatch to handler functions.

## Structure

- `application.cpp` -- app logic: LED task, CLI task, command handlers, serial init
- `application.h` -- board selection defines for Arduino IDE fallback
- `cli.h` -- header-only serial CLI library (character echo, line buffering, tokenization, command dispatch)
- `platformio.ini` -- envs extending base configs in `../base-boards.ini`
- `../common/` -- shared board config, status LED abstraction, wifi manager, secrets

## CLI conventions

- `cli.h` is header-only; instantiate as `Cli cli;`, call `cli.begin(commands, count)` in setup, call `cli.poll()` repeatedly (here: in dedicated `cli_task` every 10ms).
- Commands are defined as a `const CliCommand[]` array: `{name_string, handler_function}` pairs.
- Handler signature: `void handler(int argc, char **argv)` -- handler owns all argument validation/parsing. `argv[0]` is the command name.
- Line buffer: 64 bytes max, 8-arg cap. Overflow silently truncates. Backspace/DEL supported, characters echoed. Blank lines ignored. Unknown commands print error.

## Key conventions

- Board identity is set via build flags (`BOARD_S3_ZERO`, `BOARD_C3_SUPER_MINI`, `BOARD_C6_SUPER_MINI`) defined in `base-boards.ini`; `board-config.h` maps these to pin numbers and LED driver types.
- GPIO 18/19 carry native USB D-/D+; Serial uses USB CDC (`ARDUINO_USB_CDC_ON_BOOT=1`), so UART0 pins are free.
- LED task (core 0) and CLI task (core 1) run independently; no cross-task LED API beyond `setMode()` (posts to queue non-blocking).
- Do not modify files under `.pio/` or `../common/` without checking impact on other projects.
