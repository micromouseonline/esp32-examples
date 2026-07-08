// ----------------------------------------------------------------------------
//  touch-buttons.h — Touchscreen input producer (on-screen button bar).
//  Detects taps against BUTTON_MENU zones and posts InputEvents. Polled from
//  the Local Input Polling Task (Core 1, see application.cpp) -- this task
//  must never draw, so press-flash feedback is dropped here (documented
//  loss, see USER-INPUT-SYSTEM.md; may return later via a PRESSED/RELEASED
//  event type consumed by the main/render task instead).
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
  for (int i = 0; i < NUM_BUTTONS; i++) {
    touch_buttons[i].press(touched && touch_buttons[i].contains(touchX, touchY));
    if (touch_buttons[i].justPressed()) {
      input_queue_post(static_cast<ButtonID>(i), InputSource::TOUCH);
    }
  }
}

// Only call from the main task (app_loop / a future render context) --
// never from inside poll_touch_buttons()'s Core-1 polling task -- this
// redraws, and that task must never draw (see USER-INPUT-SYSTEM.md).
inline void set_touch_button_style(ButtonID id, ButtonColour colour) {
  if (id < NUM_BUTTONS) {
    touch_buttons[id].setStyle(colour);
  }
}

#else

inline void init_touch_buttons(LGFX &lcd) {}
inline void poll_touch_buttons(LGFX &lcd) {}  // no touchscreen on this board
inline void set_touch_button_style(ButtonID, ButtonColour) {}

#endif
