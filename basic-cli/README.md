# basic-cli

Minimal FreeRTOS + serial CLI example for ESP32. Demonstrates a header-only command-line interface library that reads line-buffered commands from USB serial and dispatches them to handler functions.

## Building and flashing

```bash
# From this directory or from the workspace root
pio run -e basic-cli-esp32-s3-super-mini
pio run -t upload -e basic-cli-esp32-s3-super-mini

# Or use the default env (S3 Super Mini)
pio run
pio run -t upload
```

## Using the CLI

Connect a serial monitor at 115200 baud. Type commands and press Enter.

**Demo commands:**
- `help` or `?` - List available commands
- `echo <args...>` or `e <args...>` - Echo back arguments
- `uptime` - Print milliseconds since boot

Example:
```
> help
Commands:
  help
  echo
  uptime
> echo hello world
hello world
> uptime
Uptime: 12345 ms
> ?
Commands:
  help
  echo
  uptime
```

**Line editing:** Backspace and Delete erase characters. No history or arrow keys. Max line length 64 bytes, max 8 arguments.

## Adding commands

Define a handler function and add it to the command table in `application.cpp`:

```cpp
void cmd_led(int argc, char **argv) {
  if (argc < 2) {
    Serial.println("Usage: led <on|off|blink>");
    return;
  }
  if (strcmp(argv[1], "on") == 0) {
    setMode(LedMode::GOOD);
    Serial.println("LED on");
  } else if (strcmp(argv[1], "off") == 0) {
    setMode(LedMode::OFF);
    Serial.println("LED off");
  }
  // ... etc
}

const CliCommand commands[] = {
  {"help", cmd_help},
  {"echo", cmd_echo},
  {"uptime", cmd_uptime},
  {"led", cmd_led},  // Add here
};
```

**Aliases:** Use the separate `aliases[]` table to avoid duplication:

```cpp
const CliAlias aliases[] = {
  {"?", cmd_help},
  {"h", cmd_help},
  {"e", cmd_echo},
  {"l", cmd_led},  // Shorthand
};
```

## How it works

- **cli.h**: Header-only library. No dependencies beyond Arduino.h. Handles character buffering, line termination, tokenization, and command dispatch.
- **cli_task**: Dedicated FreeRTOS task (core 1, 10ms poll) runs `cli.poll()` to read serial input and dispatch commands. Does not block the main `loop()`.
- **led_task**: Separate task (core 0) manages LED state via a queue. Commands can change LED mode via `setMode()` without interfering with the blink state machine.

See `CLAUDE.md` for more details on conventions and architecture.
