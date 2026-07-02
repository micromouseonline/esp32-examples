
# precision-sensor-timing-freertos

An ESP32 instrumentation-grade engine designed to capture high-precision sensor event timestamps using an event-driven FreeRTOS pipeline and the Wi-Fi TSF network clock.

## Purpose

This architecture addresses two main requirements for distributed hardware logging:

1. **Deterministic Edge Capture** – Intercepts the leading edge of a logic signal instantly via raw register access inside a hardware ISR, bypassing standard framework latency to preserve microsecond-accurate processor timestamps.
2. **Network-Synchronized Timing** – Anchors individual local timestamps to an 802.11 beacon-synchronized Wi-Fi TSF clock, allowing microsecond-accurate event correlation across multiple separate edge nodes without requiring an external PTP or GPS time source.

## Hardware & Signal Handling

The system is optimized for clean logic transitions from industrial sensors (e.g., optical break-beam light gates). 

* **Default Mapping:** GPIO 3 (Blue) and GPIO 4 (Yellow) configured with internal pull-ups (`INPUT_PULLUP`), triggering on logic transitions (`CHANGE`).
* **Silicon Architecture Awareness:** The ISR dynamically shifts its hardware mask between `GPIO.in` (Pins 0–31) and `GPIO.in1.val` (Pins 32–48), allowing any arbitrary GPIO on the target ESP32 variant to be utilized safely without risk of memory corruption or bit truncation.
* **Optical Chatter Handling:** To account for real-world physical anomalies—such as light gates encountering translucent spots on a target object's body—the system incorporates a localized blanking period (`LOCKOUT_WINDOW_US`). 

## Architecture


```

[Physical Light Gate]
│  (Clean logic transition, but prone to translucent chatter)
▼
[Hardware ISR]
│  • Immediately grabs esp_timer_get_time()
│  • Reads GPIO split registers directly (GPIO.in / GPIO.in1.val) for maximum determinism
│  • Pushes a ButtonEvent snapshot into xRawQueue
▼
[Input Broker Task]
│  • Blocks perfectly on portMAX_DELAY (0% CPU consumption when idle)
│  • Reconstructs Wi-Fi TSF network time baseline against the original raw ISR mark
│  • Enforces a localized 50ms lock-out window per channel to filter physical chatter
│  • Dispatches validated ACTION_PRESS events into xActionQueue
▼
[Action Handler Task]
│  • Backed by an expanded queue (100 slots) to survive Wi-Fi stack latency spikes
│  • Acts as a non-blocking network serialization layer to stream events to the server
▼
[Central Server Node]
│  • Operates an Interlocking State Machine reflecting the last activated channel
│  • Deduplicates sequential duplicate triggers; records the true first timestamp

```

Each stage is completely decoupled by a FreeRTOS queue. The hardware ISR exits in microseconds, and the network/logging layer can block or retry transmissions without introducing jitter into the signal capture pipeline.

## Timing Engine

### Latency-Isolated Time Stamping

To achieve absolute determinism, `esp_timer_get_time()` is executed on the very first line of the hardware ISR. This eliminates the execution overhead of core library logic wrappers, ensuring a consistent latency offset.

### TSF Reconstruction

When the Wi-Fi interface is active and connected, the broker task dynamically calculates scheduling latency and back-clocks the Wi-Fi hardware TSF counter to match the exact microsecond the physical edge hit the silicon:


```

task_latency           = current_proc_time - msg.processor_time
exact_leading_edge_tsf = current_tsf_time  - task_latency

```

Because the TSF clock is maintained at the hardware level by the Wi-Fi baseband and synchronized via 802.11 beacon frames, timestamps generated across multiple independent ESP32 nodes can be correlated chronologically on the central server. When offline, the TSF field defaults to `0`, and local processor time is used exclusively.

## Telemetry Events

Unlike classic human-machine interfaces that delay event dispatches to evaluate debouncing, this system uses an **Instant-Lock, Deferred-Filter** approach:

* **Leading-Edge Dispatch:** The very first `EVT_PRESSED` edge bypasses all delays and is sent down the pipeline instantly.
* **Chronological Blanking:** Upon capturing a valid event, a `LOCKOUT_WINDOW_US` (default: 50,000 µs) is enforced inside the broker task. Any subsequent high-frequency transitions or chatter caused by reflections or translucent bodies are silently dropped at the edge.
* **Trailing-Edge Reset:** `EVT_RELEASED` edges update the channel state immediately without requiring polling loops or timed wake-ups, resetting the channel for the next genuine sensor object.

## Telemetry Format & Server Integration

The `action_handler_task` serializes and transmits lean, telemetry-focused event payloads. The console logs present data in the following format:


```

LOGGED TRIGGER | Pin: 3 | TSF: 123456789012 | Proc: 9876543210
LOGGED TRIGGER | Pin: 4 | TSF: 123456795123 | Proc: 9876549321

```

### Server Deduplication
The central recording server acts as the final safety net for the system. It tracks the macro-state of the physical line using a "last-wins" interlocking strategy. If sequential `ACTION_PRESS` messages arrive from the same sensor without an interleaved trigger from another channel, the server treats the duplicates as physical chatter and discards them, locking onto the very first microsecond-accurate timestamp recorded by the edge node.

## Dependencies

* `esp32-info` (local library) – hardware board identification mapping.
* `Adafruit NeoPixel` – drives status indicators on specialized S3/C6 development targets.
* ESP-IDF Wi-Fi Stack (`esp_wifi.h`) – raw access to hardware beacon registers.

