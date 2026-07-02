# blinky-freertos

Blinks two LEDs at different rates using FreeRTOS tasks on ESP32 (S3-Zero, C3 Super Mini, C6 Super Mini).
Also, blink the on-board LED, if present, using a simple delay in the main loop.

## What it illustrates

- **Reusable task function with parameters** -- one `blink_task()` function is launched twice by `xTaskCreate()`, each time with a different `BlinkParams` struct. FreeRTOS runs them as independent tasks; the parameter pointer is the only thing that differs between instances.
- **Multi-target board config** -- `platformio.ini` extends shared base environments from `../shared_boards.ini`. Board-specific pin and LED-driver selection is handled in `../common/board-config.h`.
- **Arduino + FreeRTOS coexistence** -- `app_loop()` yields via `vTaskDelay()` rather than busy-spinning, letting the FreeRTOS scheduler run the blink tasks unimpeded.

## Targets

| PlatformIO env                    | Board            |
|-----------------------------------|------------------|
| blinky-freertos-esp32-s3-zero     | ESP32-S3-Zero    |
| blinky-freertos-esp32-c3          | C3 Super Mini    |
| blinky-freertos-esp32-c6          | C6 Super Mini    |

## Build

```
pio run -e blinky-freertos-esp32-c3
pio run -e blinky-freertos-esp32-c3 -t upload
```

See the [workspace build guide](../BUILDING.md) for details on how to target different boards.