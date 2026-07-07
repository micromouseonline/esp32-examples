// ----------------------------------------------------------------------------
//  touch-buttons.h — Touchscreen input producer (on-screen button bar).
//  Detects taps against BUTTON_MENU zones and posts InputEvents. Still called
//  synchronously from app_setup()/app_loop() for now (Step 4 of
//  INPUT-SKELETON-PLAN.md moves polling to its own task).
// ----------------------------------------------------------------------------
#pragma once

#include "config.h"
#include "display.h"
#include "gui-button.h"
#include "input-events.h"

#if HAS_TOUCH_INPUT

inline CustomButton touch_buttons[NUM_BUTTONS];
inline bool touch_was_active = false;

inline void init_touch_buttons(LGFX &lcd) {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    touch_buttons[i].init(lcd, BUTTON_MENU[i]);
    touch_buttons[i].draw(false);  // Draw the button unpressed
  }
}

inline void poll_touch_buttons(LGFX &lcd) {
  int32_t touchX = 0;
  int32_t touchY = 0;
  bool touched = lcd.getTouch(&touchX, &touchY);
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
      touch_buttons[i].draw(true);  // Visual feedback (inverted)
      input_queue_post(static_cast<ButtonID>(i), InputSource::TOUCH);
    }
    if (touch_buttons[i].justReleased()) {
      touch_buttons[i].draw(false);  // Visual feedback (normal)
    }
  }
}

#else

inline void init_touch_buttons(LGFX &lcd) {}
inline void poll_touch_buttons(LGFX &lcd) {}  // no touchscreen on this board

#endif
