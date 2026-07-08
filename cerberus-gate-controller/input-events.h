// ----------------------------------------------------------------------------
//  input-events.h — Generic input-event queue shared by every input producer
//  (touch, GPIO buttons, and eventually an I2C button device / WiFi messages).
//  Producers call input_queue_post(); app_loop() calls input_queue_drain()
//  once per iteration to dispatch pending events to BUTTON_MENU[id].onPress().
// ----------------------------------------------------------------------------
#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "gui-button.h"  // ButtonID, BUTTON_MENU

enum class InputSource {
  TOUCH,
  GPIO_BUTTON,
  NEOKEY_BUTTON,  // 4-button Adafruit NeoKey 1x4 (seesaw, I2C)
  // WIFI_MESSAGE,  // future: events synthesized from common/messages.h traffic
};

struct InputEvent {
  ButtonID id;
  InputSource source;
  uint32_t timestamp;
  // No `type` field in v1 -- PRESSED is the only event type today (matches
  // the app's current behavior, which only ever wires up BUTTON_MENU[i].onPress).
  // Add a `type` field here (PRESSED/RELEASED/HELD) if a producer ever needs
  // to distinguish them.
};

inline QueueHandle_t xInputQueue = nullptr;

inline void input_queue_init() {
  xInputQueue = xQueueCreate(8, sizeof(InputEvent));
}

// Thread-safe: usable from a plain polling loop, a FreeRTOS task, or an ISR
// context in the future (via xQueueSendFromISR) without changing consumers.
inline void input_queue_post(ButtonID id, InputSource source) {
  InputEvent evt{id, source, millis()};
  if (xInputQueue != nullptr) {
    xQueueSend(xInputQueue, &evt, 0);
  }
}

// Call once per app_loop() iteration. Drains all pending events (non-blocking)
// and dispatches each to its BUTTON_MENU callback.
inline void input_queue_drain() {
  if (xInputQueue == nullptr) {
    return;
  }
  InputEvent evt;
  while (xQueueReceive(xInputQueue, &evt, 0) == pdTRUE) {
    if (evt.id < NUM_BUTTONS && BUTTON_MENU[evt.id].onPress != nullptr) {
      BUTTON_MENU[evt.id].onPress();
    }
  }
}
