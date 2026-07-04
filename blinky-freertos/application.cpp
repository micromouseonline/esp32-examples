#include <Arduino.h>

#include "../common/board-config.h"
#include "../common/board-id.h"
#include "application.h"

StatusLED statusIndicator;

enum class LedMode : uint8_t {
    SYSTEM_STARTING,    //
    SERIAL_CONNECTING,  //
    SERIAL_CONNECTED,   //
    SYSTEM_READY,       //
    SYSTEM_ERROR        //
};

// Global queue handle so other files/tasks can post to it
QueueHandle_t ledModeQueue = nullptr;

void setMode(LedMode mode) {
    xQueueSend(ledModeQueue, &mode, 0);
}

static void led_task(void *pvParameters) {
    (void) pvParameters;
    constexpr uint8_t BRIGHTNESS = 50;

    LedMode currentMode = LedMode::SYSTEM_STARTING;
    uint32_t frameDelayMs = 125;
    int idx = 0;
    int state = 1;

    for (;;) {
        // Check the queue for a new mode.
        // Instead of freezing, wait a maximum of 1 tick (almost instantaneous).
        LedMode newMode;
        if (xQueueReceive(ledModeQueue, &newMode, pdMS_TO_TICKS(1)) == pdTRUE) {
            currentMode = newMode;
            idx = 0;  // Reset animation state on mode switch
        }

        // State Machine: Update behavior based on the current mode
        switch (currentMode) {
            case LedMode::SYSTEM_STARTING:
                statusIndicator.setRGB(0, 0, BRIGHTNESS);
                break;

            case LedMode::SERIAL_CONNECTING:
                frameDelayMs = 50;  // Fast blink/cycle
                statusIndicator.setRGB(BRIGHTNESS, 0, 0);
                if (idx++ % 2 == 0) {
                    statusIndicator.turnOff();
                }
                break;

            case LedMode::SERIAL_CONNECTED:
                frameDelayMs = 1000;  // Low priority steady state
                statusIndicator.setRGB(BRIGHTNESS, BRIGHTNESS, 0);
                break;

            case LedMode::SYSTEM_ERROR:
                frameDelayMs = 125;  // Intense rapid warning
                statusIndicator.setRGB(0, 0, 0);
                if (idx++ % 8 == 0)
                    statusIndicator.setRGB(BRIGHTNESS, 0, 0);
                break;

            case LedMode::SYSTEM_READY:
                frameDelayMs = 1000 / BRIGHTNESS;  // aim for a 1 second cycle
                // Update brightness index based on direction
                idx += state;
                // Reverse direction at boundaries
                if (idx >= BRIGHTNESS) {
                    idx = BRIGHTNESS;
                    state = -1;
                } else if (idx <= 0) {
                    idx = 0;
                    state = 1;
                }
                statusIndicator.setRGB(0, idx, 0);
                break;
        }

        // Dynamic frame delay based on the current active state
        vTaskDelay(pdMS_TO_TICKS(frameDelayMs));
    }
}

void system_setup() {
    statusIndicator.begin();

    // Create a queue capable of holding 5 items of type LedMode
    ledModeQueue = xQueueCreate(5, sizeof(LedMode));

    if (ledModeQueue != nullptr) {
        xTaskCreatePinnedToCore(led_task, "led", 2048, NULL, 1, NULL, 0);
    } else {
        // Handle queue creation failure (extremely rare, usually out of memory)
    }
}

static constexpr uint32_t SERIAL_TIMEOUT_MS = 2000;
static constexpr uint32_t SERIAL_SETTLE_MS = 500;
void app_setup() {
    // start this straight away. It will take a while to be ready.
    Serial.begin(SERIAL_BAUD);
    Serial.setTxTimeoutMs(0);  // allow drops if nothing gets connected.
    // we can do other stuff while waiting.
    system_setup();
    setMode(LedMode::SYSTEM_STARTING);
    // it may take anything up to 2000ms altogether to get  a serial connection

    while (!Serial && (millis() < SERIAL_TIMEOUT_MS)) {
        delay(10);
    }
    setMode(LedMode::SERIAL_CONNECTING);
    uint32_t serial_ready_time = millis();
    if (Serial) {
        // just because the hardware is ready, does not mean the terminal is ready
        // so allow time for that as well or buffered output may be lost.
        delay(SERIAL_SETTLE_MS);
        setMode(LedMode::SERIAL_CONNECTED);
        uint32_t system_ready_time = millis();
        // By now it should be safe to start sending.
        log_i("Board: %s", get_board_name());
        log_i("Serial port took %lu ms to establish.", serial_ready_time);
        log_i("System running after %lu ms.", system_ready_time);
    }
    setMode(LedMode::SYSTEM_READY);
}

void app_loop() {
    delay(50);
}
