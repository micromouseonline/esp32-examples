# precision-sensor-timing-freertos

FreeRTOS precision sensor event pipeline with microsecond-accurate timestamps and optional WiFi TSF anchoring.

## Structure

- `buttons-freertos.ino` -- Arduino entry point; delegates to `app_setup()`/`app_loop()`
- `application.cpp` -- all app logic: ISR, FSM, queues, broker task, action handler
- `application.h` -- declares `app_setup()` and `app_loop()`
- `platformio.ini` -- three envs extending base configs in `../shared_boards.ini`
- `../common/` -- shared board config, status LED abstraction, wifi manager, secrets

## Pipeline

```
button_isr()          ISR -- reads GPIO register directly, timestamps on first line, sends to xRawQueue
input_broker_task()   lockout filter, TSF reconstruction, FSM state; sends to xActionQueue
action_handler_task() pure serialization layer -- logs/transmits validated events
```

## Key design points

- Input source is a **sensor** (e.g. optical light gate), not a mechanical button. Assumed clean transitions.
- ISR reads GPIO hardware registers directly (`GPIO.in` / `GPIO.in1.val`) instead of `digitalRead()` to minimize latency. Handles split register boundary at pin 32.
- `esp_timer_get_time()` is called on the very first line of the ISR, before any other logic.
- Channels are registered in `active_channels[]`; `get_source_by_pin()` iterates it -- adding a channel only requires adding an entry here and in `app_setup()`.
- FSM is 2-state per channel: `STATE_RELEASED` <-> `STATE_PRESSED`. Trailing edges update state instantly with no cooldown period.
- `LOCKOUT_WINDOW_US` (50 ms) is enforced in the **broker task** against `last_valid_press_time` stored in `SourceParams`. Events that arrive within the lockout are dropped before TSF reconstruction and before entering `xActionQueue`. This is the sole filter; `action_handler_task` does no validation.
- Broker blocks on `portMAX_DELAY` -- no polling, 0% CPU when idle.
- TSF reconstruction computes task scheduling latency (`current_proc_time - msg.processor_time`) and subtracts it from the current TSF to back-clock the network timestamp to the original ISR edge. Requires WiFi up; `tsf_time` is 0 offline.
- Both queues are 100 slots (`EVENT_QUEUE_LENGTH`) to absorb WiFi stack latency spikes in the handler.
- `InputBroker` runs at priority 3; `ActionHandler` at priority 2.
- `is_button_pressed()` takes a critical section over `fsm_mux`; safe to call from any task.

## Board conventions

- Signal inputs on GPIO 3 (blue) and GPIO 4 (yellow), active-low with `INPUT_PULLUP`.
- Board identity via build flags (`BOARD_S3_ZERO`, `BOARD_C3_SUPER_MINI`, `BOARD_C6_SUPER_MINI`) from `shared_boards.ini`.
- Serial uses USB CDC (`ARDUINO_USB_CDC_ON_BOOT=1`); baud rate is `SERIAL_BAUD` from `board-config.h`.
- Do not modify files under `.pio/` or `../common/` without checking impact on sibling projects.
