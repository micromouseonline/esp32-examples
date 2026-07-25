/***
 * Blinky for ESP32 and FreeRTOS
 *
 * This is an elaborate version of a blinky program that makes use of freeRTOS to
 * provide a background task which actually sets the status LED colour - always
 * assuming a suitable LED is available.
 *
 * Not only does the application illustrate how to set up and feed such a task, it
 * also shows how getting the Serial connection working on the more recent
 * processors can take time and needs a little extra care before it can be used.
 */
#include <Arduino.h>
#include <WiFi.h>

#include "../common/board-config.h"
#include "../common/board-id.h"
#include "application.h"

StatusLED statusIndicator;

enum class LedMode : uint8_t {
  SYSTEM_STARTING,    //
  SERIAL_CONNECTING,  //
  SERIAL_CONNECTED,   //
  SYSTEM_READY,       //
  SYSTEM_ERROR,       //
  GOOD,               //
  BAD                 //
};

// Global queue handle so other files/tasks can post to it
QueueHandle_t ledModeQueue = nullptr;

void setMode(LedMode mode) {
  xQueueSend(ledModeQueue, &mode, 0);
}

static void led_task(void *pvParameters) {
  (void)pvParameters;
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
        if (idx++ % 8 == 0) {
          statusIndicator.setRGB(0, BRIGHTNESS, 0);
        }
        break;

      case LedMode::GOOD:
        statusIndicator.setRGB(0, BRIGHTNESS, 0);
        break;

      case LedMode::BAD:
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

static constexpr uint32_t SERIAL_TIMEOUT_MS = 5000;
static constexpr uint32_t SERIAL_SETTLE_MS = 1000;
void app_setup() {
  // start this straight away. It will take a while to be ready.
  Serial.begin(SERIAL_BAUD);
  // we can do other stuff while waiting....
  // The status LED is managed in a task which takes mode settings from a queue
  // and uses them to set the led colour
  statusIndicator.begin();
  ledModeQueue = xQueueCreate(5, sizeof(LedMode));
  xTaskCreatePinnedToCore(led_task, "led", 2048, NULL, 1, NULL, 0);
  setMode(LedMode::SYSTEM_STARTING);
  // Now we can check the state of the Serial connection if we want.
  // Here we wait SERIAL_TIMEOUT_MS milliseconds to see if a connection
  // is established on the USB CDC link. Generally, it will take less than that but,
  // of course, there may be nothing there so we include the timeout just in case.
  while (!Serial.isConnected() && (millis() < SERIAL_TIMEOUT_MS)) {
    delay(10);
  }
  uint32_t serial_ready_time = millis();
  if (Serial.isConnected()) {
    // just because we have a connection, does not mean that the terminal is
    // actually ready for us so allow an extra delay for that.
    delay(SERIAL_SETTLE_MS);
    setMode(LedMode::SERIAL_CONNECTED);
    uint32_t system_ready_time = millis();
    Serial.println();
    Serial.printf("Board: %s\n", get_board_name());
    Serial.printf("Serial port took %lu ms to establish.\n", serial_ready_time);
    Serial.printf("System running after %lu ms.\n", system_ready_time);
    setMode(LedMode::SYSTEM_READY);
  }
}

// loop just flashes the status LED to confirm the code is not hung up and
// prints the millis() counter so we can see how long it took to get serial output
void app_loop() {
  delay(100);
  setMode(LedMode::GOOD);
  delay(100);
  setMode(LedMode::BAD);
  Serial.printf("%lu\n", millis());
}
