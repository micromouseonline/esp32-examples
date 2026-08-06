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

#include <stdint.h>

StatusLED statusIndicator;

enum class LedMode : uint8_t {
  SYSTEM_STARTING,    //
  SERIAL_CONNECTING,  //
  SERIAL_CONNECTED,   //
  SYSTEM_READY,       //
  SYSTEM_ERROR,       //
  GOOD,               //
  BAD,                //
  OFF                 //
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

      case LedMode::OFF:
        statusIndicator.turnOff();
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
void setup() {
  // 1. Initialize Serial and LED Task first
  Serial.begin(SERIAL_BAUD);
  statusIndicator.begin();
  ledModeQueue = xQueueCreate(5, sizeof(LedMode));
  xTaskCreatePinnedToCore(led_task, "led", 2048, NULL, 1, NULL, 0);
  setMode(LedMode::SYSTEM_STARTING);

  // 2. Initialize Wi-Fi hardware fully to ensure MAC address eFuse is populated
  delay(50);  // Give the driver 50ms to read eFuse MAC into RAM

  // 3. Wait for USB Serial CDC connection (with timeout)
  while (!Serial && (millis() < SERIAL_TIMEOUT_MS)) {
    delay(10);
  }

  uint32_t serial_ready_time = millis();

  if (Serial) {
    delay(SERIAL_SETTLE_MS);
    setMode(LedMode::SERIAL_CONNECTED);

    uint32_t system_ready_time = millis();
    Serial.println();
    Serial.printf("Serial port took %lu ms to establish.\n", serial_ready_time);
    Serial.printf("System running after %lu ms.\n", system_ready_time);

    // Buffer for fallback address string ("0xXXXXXXXX\0" = 11 bytes minimum)
    char board_name_buf[16];
    const char *name = get_board_name(board_name_buf);

    Serial.printf("Board Identified: %s\n", name);
    Serial.printf("Full MAC Address: %s\n", WiFi.macAddress().c_str());

    setMode(LedMode::SYSTEM_READY);
  }
}

// loop just flashes the status LED to confirm the code is not hung up and
// prints the millis() counter so we can see how long it took to get serial output
void loop() {
  setMode(LedMode::GOOD);
  delay(100);
  setMode(LedMode::OFF);
  delay(900);
}
