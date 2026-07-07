// ----------------------------------------------------------------------------
//  touch-buttons.h — Touchscreen input producer (on-screen button bar).
//  Detects taps against BUTTON_MENU zones and posts InputEvents. Polled from
//  the Local Input Polling Task (Core 1, see application.cpp) -- this task
//  must never draw, so press-flash feedback is dropped here (documented loss
//  per Step 4 of INPUT-SKELETON-PLAN.md; may return later via a
//  PRESSED/RELEASED event type consumed by the main/render task instead).
// ----------------------------------------------------------------------------
#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "config.h"
#include "display.h"
#include "gui-button.h"
#include "input-events.h"

#if HAS_TOUCH_INPUT

inline CustomButton touch_buttons[NUM_BUTTONS];
inline bool touch_was_active = false;

// Guards lcd.getTouch() against the main task's draw calls on boards where
// touch and the display panel share one physical SPI bus. A no-op on every
// board today (all five set TOUCH_SHARES_DISPLAY_SPI_BUS to 0 -- touch is
// either I2C or its own dedicated SPI pins) but kept real infrastructure for
// the day a shared-bus board shows up.
#if TOUCH_SHARES_DISPLAY_SPI_BUS
inline SemaphoreHandle_t touch_spi_bus_mutex = nullptr;
inline void touch_bus_lock() {
  if (touch_spi_bus_mutex != nullptr) {
    xSemaphoreTake(touch_spi_bus_mutex, portMAX_DELAY);
  }
}
inline void touch_bus_unlock() {
  if (touch_spi_bus_mutex != nullptr) {
    xSemaphoreGive(touch_spi_bus_mutex);
  }
}
#else
inline void touch_bus_lock() {}
inline void touch_bus_unlock() {}
#endif

inline void init_touch_buttons(LGFX &lcd) {
#if TOUCH_SHARES_DISPLAY_SPI_BUS
  touch_spi_bus_mutex = xSemaphoreCreateMutex();
#endif
  for (int i = 0; i < NUM_BUTTONS; i++) {
    touch_buttons[i].init(lcd, BUTTON_MENU[i]);
    touch_buttons[i].draw(false);  // Draw the button unpressed
  }
}

inline void poll_touch_buttons(LGFX &lcd) {
  int32_t touchX = 0;
  int32_t touchY = 0;
  touch_bus_lock();
  bool touched = lcd.getTouch(&touchX, &touchY);
  touch_bus_unlock();
  // Log every raw touch-down, independent of whether it lands inside any
  // button's hit-box -- lets uncalibrated/misaligned touch (e.g. resistive
  // XPT2046 before calibration) show up on the serial log even when it
  // never reaches input_queue_post().
  if (touched && !touch_was_active) {
    Serial.printf("[TOUCH] raw touch at x=%d, y=%d\n", touchX, touchY);
  }
  touch_was_active = touched;
  for (int i = 0; i < NUM_BUTTONS; i++) {
    touch_buttons[i].press(touched && touch_buttons[i].contains(touchX, touchY));
    if (touch_buttons[i].justPressed()) {
      input_queue_post(static_cast<ButtonID>(i), InputSource::TOUCH);
    }
  }
}

#else

inline void init_touch_buttons(LGFX &lcd) {}
inline void poll_touch_buttons(LGFX &lcd) {}  // no touchscreen on this board

#endif
