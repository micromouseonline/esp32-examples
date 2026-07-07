
# System Specification: ESP32 Race Timer Application for CYD (Cheap Yellow Display)

## Hardware Target
*   **MCU:** ESP32 or ESP32-S3 (Dual-core execution mode).
*   **Display & Touch:** Cheap Yellow Display (CYD) board featuring an SPI-driven TFT screen and an SPI-driven XPT2046 touch controller.
*   **Storage:** Onboard SD card slot sharing the SPI bus with the display and touch controller.
*   **External Inputs:** Adafruit NeoKey I2C expander for external front-panel physical buttons.

---

## Core Architecture & Execution Model
The system uses an event-driven, decoupled, asynchronous architecture managed by FreeRTOS. Tasks communicate strictly via thread-safe FreeRTOS queues. To prevent race conditions and SPI bus panics, a Supervisory Control state structure assigns strict resource ownership.

### Core Assignment
*   **Core 0:** Reserved for the network stack, Wi-Fi connectivity, and the Asynchronous HTTP Server engine.
*   **Core 1:** Runs the Main Application Task (Supervisor/State Machine), the Local Input Polling Task, and the Serial Driver Task.

### Safe Memory & Data Structures
1.  **`AppEvent` Struct:** A fixed-size tagged union struct passed strictly **by value** into the Main Event Queue. Maximum size is 64 bytes. No dynamic allocation (`malloc`/`free`) is permitted to completely eliminate heap fragmentation.
2.  **`LogMessage` Struct:** A fixed-size struct passed **by value** into the Logging Queue for sequential serialization.

---

## Task Breakdown & Functional Specifications

### 1. Input Layer & Event Generation
*   **Local Input Polling Task (Core 1):** Polls the hardware GPIO buttons, the I2C NeoKey expander, and the SPI touch screen sequentially every 15ms. It handles debouncing entirely in software. Valid inputs are packaged into an `AppEvent` and pushed to the Main Event Queue.
*   **Asynchronous HTTP Listener (Core 0):** Runs an async web server. It parses incoming simple HTTP GET requests containing up to two 64-bit values. Upon parsing, it constructs an `AppEvent` and pushes it to the Main Event Queue. The implementation must be non-blocking and capable of handling back-to-back request spikes separated by as little as 20ms.
*   **Serial Monitor Task (Core 1):** Monitors the UART RX buffer. It blocks until a complete line terminates with an EOL (`\n`) marker. Incoming messages are guaranteed to be under 64 bytes. It parses the command string, populates the event fields, and pushes an `AppEvent`.

### 2. Main Processing, Supervisor & State Machine (Core 1 - High Priority)
*   **Resource Ownership:** The Main Application Task holds exclusive ownership over the Display driver (TFT_eSPI/LVGL) and the core Race Timing State Machine. No other task may write directly to the screen.
*   **Operation:** Loops continuously, blocking on the Main Event Queue. When a message is retrieved, it advances the state machine, writes directly to the display, and dispatches corresponding string metrics to the Logging Queue.
*   **Supervisory States:** The application governs behavior through three top-level modes:
    *   `READY`: System is idle. Local inputs and remote HTTP triggers are actively monitored.
    *   `RACING`: System is capturing precise timing events, calculating laps, updating the UI, and streaming data to the log queue.
    *   `MAINTENANCE`: Triggered by a specific system command (e.g., log retrieval request). In this state, the task completely ignores all incoming race/lap timing triggers. It commands the Logging Task to close active file handles, releases file-system locks, and paints a "File Transfer Active" screen on the display.

### 3. Logging Infrastructure (Core 1 - Low Priority)
*   **Operation:** Blocks on a 64-item FIFO Logging Queue. 
*   **Serialization:** Pops raw event data, formats it into a human-readable CSV string, and appends it to the active file on the SD card over the SPI bus.
*   **Bus Safety:** When the Supervisor state transitions to `MAINTENANCE`, the logging task completely flushes its buffer, closes its open file descriptor, and pauses all SD card SPI operations to completely yield the physical bus to the Wi-Fi file-streaming server.

### 4. File, Session, and Retrieval Management
*   **Boot Sequencing:** On startup, the system attempts to sync time via NTP. If an internet connection is unavailable (e.g., standalone Access Point mode), the system falls back to reading an auto-incrementing boot counter stored in NVS (Non-Volatile Storage) flash memory.
*   **File Naming Rules:** Log files are safely separated by session and named sequentially: `/logs/RACE_NUM_[Counter].CSV` or timestamped via NTP if available. On every boot, a pointer file located at `/logs/LATEST.TXT` is rewritten with the string name of the currently active CSV log file.
*   **HTTP Log Streaming:** While the system is explicitly locked in the `MAINTENANCE` state, the HTTP server reads requested CSV logs from the SD card. Data must be served to the network in chunks, embedding short `vTaskDelay` yields to prevent starving background tasks and triggering the ESP32 Task Watchdog Timer (TWDT).

---

## Coding Requirements
Generate clean, highly modular, thread-safe C/C++ code utilizing the Arduino-ESP32 core framework. Ensure all SPI bus transactions are properly guarded, FreeRTOS queue API interactions check for timeout constraints, and no dynamic memory configurations are used inside the execution path.