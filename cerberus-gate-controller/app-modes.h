// ----------------------------------------------------------------------------
//  app-modes.h — Central button dispatcher + Supervisor state machine. Every
//  producer (touch, gpio, neokey) ultimately funnels through
//  input_queue_drain() -> BUTTON_MENU[id].onPress() -> on_button_event() ->
//  here, regardless of which physical input generated the event.
//
//  Two states: SUPERVISOR (a mode-select list) and FONT_DEMO (Step 8's font
//  browser). SUPERVISOR's mode table is producer-agnostic and open-ended --
//  adding a real mode later is just another table entry, no dispatcher
//  changes needed.
// ----------------------------------------------------------------------------
#pragma once

#include <Arduino.h>

#include "display.h"  // LGFX
#include "font-demo.h"
#include "gui-button.h"     // ButtonID
#include "neokey-pixels.h"  // neokey_set_all -- no-op stub when !HAS_NEOKEY_BUTTONS

enum class AppState { SUPERVISOR, FONT_DEMO, RECALIBRATE_TOUCH, NEOKEY_DEMO };

inline AppState app_state = AppState::SUPERVISOR;
inline bool supervisor_dirty = true;
inline int supervisor_selected = 0;

struct ModeEntry {
  const char* name;
  void (*enter)();
};

inline void enter_font_demo() {
  app_state = AppState::FONT_DEMO;
  font_demo_index = 0;
  font_demo_dirty = true;
}

inline void enter_placeholder_b() {
  Serial.println("[APP] Placeholder B selected (no-op)");
}

inline void enter_placeholder_c() {
  Serial.println("[APP] Placeholder C selected (no-op)");
}

// NeoKey colour demo: cycles all four pixels through a fixed palette on a
// timer, driven from app_loop() (main task) while poll_neokey_buttons()
// keeps reading key presses from the Core-1 input polling task at the same
// time -- exercises exactly the read/write bus-sharing case
// neokey_bus_mutex (neokey-driver.h) exists to guard, plus normal display
// updates continuing to run alongside both. Raw uint32_t 0xRRGGBB literals
// here rather than neokey-driver.h's NP_* constants -- those only exist
// under #if HAS_NEOKEY_BUTTONS, and this file (and neokey_set_all() itself)
// stays board-agnostic via neokey-pixels.h's no-op fallback.
constexpr uint32_t NEOKEY_DEMO_COLOURS[] = {0xFF0000, 0x00FF00, 0x0000FF, 0xFFFF00,
                                             0xFF00FF, 0x00FFFF, 0xFFFFFF, 0x000000};
constexpr int NEOKEY_DEMO_NUM_COLOURS = sizeof(NEOKEY_DEMO_COLOURS) / sizeof(NEOKEY_DEMO_COLOURS[0]);
constexpr uint32_t NEOKEY_DEMO_PERIOD_MS = 300;

inline bool neokey_demo_dirty = true;
inline int neokey_demo_colour_index = 0;
inline uint32_t neokey_demo_last_update = 0;

inline void enter_neokey_demo() {
  app_state = AppState::NEOKEY_DEMO;
  neokey_demo_colour_index = 0;
  neokey_demo_last_update = 0;  // forces an immediate first colour on the next update
  neokey_demo_dirty = true;
}

// Only call from the main task (app_loop) -- same rule as
// supervisor_render()/font_demo_render(): non-blocking, timer-gated so it
// never stalls app_loop()'s other per-tick work (input_queue_drain(),
// display redraw).
inline void neokey_demo_update() {
  uint32_t now = millis();
  if (now - neokey_demo_last_update >= NEOKEY_DEMO_PERIOD_MS) {
    neokey_demo_last_update = now;
    neokey_set_all(NEOKEY_DEMO_COLOURS[neokey_demo_colour_index]);
    neokey_demo_colour_index = (neokey_demo_colour_index + 1) % NEOKEY_DEMO_NUM_COLOURS;
  }
}

// Only call from the main task -- draws, same rule as supervisor_render().
inline void neokey_demo_render(LGFX& display) {
  display.setClipRect(0, 0, display.width(), FONT_DEMO_CONTENT_HEIGHT);
  display.fillRect(0, 0, display.width(), FONT_DEMO_CONTENT_HEIGHT, TFT_BLACK);

  display.setTextDatum(textdatum_t::top_left);
  display.setFont(&fonts::Font2);
  display.setTextColor(TFT_CYAN, TFT_BLACK);
  display.drawString("NEOKEY COLOUR DEMO", 10, 5);
  display.drawFastHLine(0, 30, display.width(), TFT_DARKGRAY);

  display.setFont(&fonts::DejaVu18);
  display.setTextColor(TFT_WHITE, TFT_BLACK);
  display.drawString("Cycling all 4 pixels.", 10, 50);
  display.drawString("Press ACTION to return.", 10, 80);

  display.clearClipRect();
  neokey_demo_dirty = false;
}

// Actual recalibration (re_calibrate(), which needs the LGFX instance) runs
// from app_loop() in application.cpp on the RECALIBRATE_TOUCH state -- this
// header has no access to `lcd`, same reason enter_font_demo() only flips
// state/dirty flags and leaves drawing to app_loop().
inline void enter_recalibrate_touch() {
  app_state = AppState::RECALIBRATE_TOUCH;
}

inline const ModeEntry MODE_TABLE[] = {
    {"Font Demo Viewer", enter_font_demo},
#if HAS_TOUCH_INPUT && TOUCH_NEEDS_CALIBRATION
    {"Recalibrate Touch", enter_recalibrate_touch},
#else
    {"Placeholder B", enter_placeholder_b},
#endif
#if HAS_NEOKEY_BUTTONS
    {"NeoKey Colour Demo", enter_neokey_demo},
#else
    {"Placeholder C", enter_placeholder_c},
#endif
};
constexpr int NUM_MODES = sizeof(MODE_TABLE) / sizeof(MODE_TABLE[0]);

// Only call from the main task -- this draws, same rule as
// font_demo_render()/set_touch_button_style(). Reuses
// FONT_DEMO_CONTENT_HEIGHT (font-demo.h): both screens share the same
// content band above the touch button bar.
inline void supervisor_render(LGFX& display) {
  display.setClipRect(0, 0, display.width(), FONT_DEMO_CONTENT_HEIGHT);
  display.fillRect(0, 0, display.width(), FONT_DEMO_CONTENT_HEIGHT, TFT_BLACK);

  display.setTextDatum(textdatum_t::top_left);
  display.setFont(&fonts::Font2);
  display.setTextColor(TFT_CYAN, TFT_BLACK);
  display.drawString("SUPERVISOR", 10, 5);
  display.drawFastHLine(0, 30, display.width(), TFT_DARKGRAY);

  display.setFont(&fonts::DejaVu18);
  int y = 50;
  for (int i = 0; i < NUM_MODES; i++) {
    bool selected = (i == supervisor_selected);
    display.setTextColor(selected ? TFT_YELLOW : TFT_WHITE, TFT_BLACK);
    display.drawString(selected ? "> " : "  ", 10, y);
    display.drawString(MODE_TABLE[i].name, 30, y);
    y += display.fontHeight() + 4;
  }

  display.clearClipRect();
  supervisor_dirty = false;
}

inline void on_button_event(ButtonID id) {
  switch (app_state) {
    case AppState::SUPERVISOR:
      switch (id) {
        case BTN_ARM:
          Serial.println("[APP] PREV");
          supervisor_selected = (supervisor_selected + NUM_MODES - 1) % NUM_MODES;
          supervisor_dirty = true;
          break;
        case BTN_START:
          Serial.println("[APP] NEXT");
          supervisor_selected = (supervisor_selected + 1) % NUM_MODES;
          supervisor_dirty = true;
          break;
        case BTN_GOAL:
          Serial.println("[APP] ACTION");
          MODE_TABLE[supervisor_selected].enter();
          break;
        case BTN_RESET:
          Serial.println("[APP] unused");
          break;
        default:
          break;
      }
      break;

    case AppState::FONT_DEMO:
      switch (id) {
        case BTN_ARM:
          Serial.println("[APP] PREV");
          font_demo_prev();
          break;
        case BTN_START:
          Serial.println("[APP] NEXT");
          font_demo_next();
          break;
        case BTN_GOAL:
          Serial.println("[APP] ACTION -> SUPERVISOR");
          app_state = AppState::SUPERVISOR;
          supervisor_dirty = true;
          break;
        case BTN_RESET:
          Serial.println("[APP] unused");
          break;
        default:
          break;
      }
      break;

    case AppState::NEOKEY_DEMO:
      switch (id) {
        case BTN_GOAL:
          Serial.println("[APP] ACTION -> SUPERVISOR");
          neokey_set_all(0x000000);  // turn pixels off on the way out
          app_state = AppState::SUPERVISOR;
          supervisor_dirty = true;
          break;
        case BTN_ARM:
        case BTN_START:
        case BTN_RESET:
          Serial.println("[APP] unused (colour cycles automatically)");
          break;
        default:
          break;
      }
      break;
  }
}
