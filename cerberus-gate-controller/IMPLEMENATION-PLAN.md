# Comprehensive Development & Testing Plan: CYD Race Timer

Execute this plan sequentially. Do not move to a subsequent step until the "Verification Test" for the current step passes completely.

---

## Phase 1: Core Framework & Inter-Task Communication
The goal of this phase is to establish the thread-safe foundation of the application without any physical I/O or graphics hardware.

### Step 1.1: Data Structures & The Main App Task
* **Action:** Define the `AppEvent` tagged union structure (incorporating dummy fields for button, touch, network, and serial payloads). Initialize the `Main Event Queue` with a depth of 32. Create the `Main App Task` on Core 1 at a high priority (`configMAX_PRIORITIES - 1`). Inside this task, loop indefinitely, blocking on the queue, and print the received event details to the primary Serial monitor.
* **Verification Test:** Write a temporary loop in `setup()` that pushes 10 mock events of alternating types into the queue. Verify via the Serial Monitor that the Main App Task pops and prints all 10 events sequentially in perfect FIFO order.

### Step 1.2: The Supervisory State Machine Core
* **Action:** Implement the top-level supervisor state logic (`READY`, `RACING`, `MAINTENANCE`) inside the Main App Task. Add parsing logic so that when specific command events are popped from the queue, the internal state updates. 
* **Verification Test:** Manually push a sequence of mock command events into the queue (e.g., `CMD_START_RACE`, `CMD_TRIGGER_LAP`, `CMD_ENTER_MAINTENANCE`). Verify via Serial prints that the system transitions correctly between states and that timing events are processed during `RACING` but explicitly ignored or dropped during `MAINTENANCE`.

---

## Phase 2: Input Layer Integration
The goal of this phase is to bring asynchronous and synchronous input sources online, directing them all into the Core 1 queue.

### Step 2.1: Local Input Polling Task (Core 1)
* **Action:** Create a low-priority polling task on Core 1 that loops every 15ms. Implement the basic hardware logic to poll a physical GPIO button and the Adafruit NeoKey via I2C. Add software debouncing logic (state integration over 2-3 cycles). When a valid press is confirmed, package it into an `AppEvent` and use `xQueueSend()` to push it to the main queue.
* **Verification Test:** Press the physical buttons and the NeoKey buttons. Watch the Serial Monitor to ensure the Main App Task intercepts the events instantly, showing the correct button IDs with zero bounce or double-triggering.

### Step 2.2: Asynchronous HTTP Input Handler (Core 0)
* **Action:** Spin up the `ESPAsyncWebServer` library, configuring it to pin its internal infrastructure to Core 0. Implement a basic HTTP GET route (e.g., `/lap?time=64bitValue&id=1`). In the request callback, parse the query parameters directly into an `AppEvent` struct and push it to the Main Event Queue.
* **Verification Test:** Use a tool like `curl` or a script to fire 5 HTTP requests separated by only 20 milliseconds. Verify that the Main App Task on Core 1 captures and prints all 5 events without dropping data or crashing the network stack.

### Step 2.3: Serial Monitor Task (Core 1)
* **Action:** Create a dedicated task on Core 1 that listens to the primary UART interface. Use a non-blocking read mechanism that accumulates incoming characters into a local 64-byte buffer until an EOL (`\n`) character is discovered. Parse the resulting string command, map it to an `AppEvent`, and push it to the main queue.
* **Verification Test:** Type commands directly into your serial terminal (e.g., `START\n`). Confirm that the Serial Task processes the string and that the Main App Task transitions states accordingly.

---

## Phase 3: Storage, NVS, & Logging Infrastructure
The goal of this phase is to safely establish file system routines and offload blocking disk I/O to a lower-priority worker thread.

### Step 3.1: NVS Boot Counter & Storage Initialization
* **Action:** Initialize the ESP32 Non-Volatile Storage (NVS) flash partition. Write a routine that reads an integer key named `boot_count`, increments it by 1, saves it back to NVS, and initializes the SD Card over the SPI bus. Use this incremented number to construct a unique, persistent filename (e.g., `/logs/RACE_NUM_[count].CSV`). Create a file named `/logs/LATEST.TXT` and write the active filename string inside it.
* **Verification Test:** Power-cycle the CYD board 5 times consecutively. Pull the SD card, insert it into a computer, and verify that 5 empty files (`RACE_NUM_1.CSV` through `RACE_NUM_5.CSV`) exist, and that `LATEST.TXT` correctly contains the text string `RACE_NUM_5.CSV`.

### Step 3.2: The Logging Queue & Task (Core 1 - Low Priority)
* **Action:** Initialize a 64-item `Logging Queue` passing fixed-size `LogMessage` structs by value. Create a low-priority `Logging Task` on Core 1. This task blocks on the logging queue, pops incoming data packets, formats them into a CSV row string, and appends them to the active SD card file handle. 
* **Verification Test:** Modify the Main App Task so that when a simulated lap trigger event occurs, it passes a data struct to the Logging Queue. Fire a dense burst of inputs. Verify that the system writes data continuously to the SD card without causing any latency hiccups in the Main App Task loop.

---

## Phase 4: UI, Maintenance, & Extraction
The final phase glues the visual components to the system and safely implements the file delivery mechanism.

### Step 4.1: Exclusive Display Updates
* **Action:** Initialize the display hardware on Core 1 utilizing the `TFT_eSPI` library. Grant the Main App Task exclusive ownership over writing to the screen screen. Update the UI layout dynamically inside the Main App Task based on state machine shifts (e.g., showing rolling lap times during `RACING`, and static menus during `READY`).
* **Verification Test:** Run the timer while bombarding the network stack with HTTP requests. Ensure the display updates fluently with zero graphical glitching, artifact tearing, or multi-core memory panics.

### Step 4.2: Maintenance Guarding & HTTP Log Streaming
* **Action:** Implement the `MAINTENANCE` behavior routine. When a `CMD_ENTER_MAINTENANCE` event is processed, the Main App Task must update the display to read "Syncing Data...", and command the Logging Task to explicitly flush its buffers and close its open SD card file handle. Then, expose an HTTP GET route `/download`. The web server callback on Core 0 opens the targeted CSV file in read-only mode and streams it to the client using a chunked wrapper, inserting a `vTaskDelay(1)` between blocks to satisfy the Task Watchdog Timer (TWDT).
* **Verification Test:** Generate a 500KB log file. Trigger `MAINTENANCE` mode, and download the file via a web browser. Confirm that the download finishes with zero errors, the contents perfectly match the card data, the device watchdog does not trip, and the display remains functional throughout the transfer.