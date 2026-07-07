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
#include "gui-button.h"  // ButtonID

enum class AppState { SUPERVISOR, FONT_DEMO };

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

inline const ModeEntry MODE_TABLE[] = {
    {"Font Demo Viewer", enter_font_demo},
    {"Placeholder B", enter_placeholder_b},
    {"Placeholder C", enter_placeholder_c},
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
  }
}
