// ----------------------------------------------------------------------------
//  app-modes.h — Central button dispatcher + Supervisor state machine. Every
//  producer (touch, gpio, neokey) ultimately funnels through
//  input_queue_drain() -> BUTTON_MENU[id].onPress() -> on_button_event() ->
//  here, regardless of which physical input generated the event.
//
//  SUPERVISOR's mode table is producer-agnostic and open-ended -- adding a
//  real mode later is just another table entry, no dispatcher changes
//  needed. Unimplemented entries use the generic PLACEHOLDER state
//  (enter_placeholder()) until they become real sub-applications.
// ----------------------------------------------------------------------------
#pragma once

#include <Arduino.h>

#include "display.h"  // LGFX
#include "gui-button.h"  // ButtonID

enum class AppState { SUPERVISOR, RECALIBRATE_TOUCH, PLACEHOLDER };

// Content stays above y=200 -- the shared touch button bar (BUTTON_MENU,
// gui-button.h) is drawn once at y=220 and never redrawn here.
constexpr int SUPERVISOR_CONTENT_HEIGHT = 200;

inline AppState app_state = AppState::SUPERVISOR;
inline bool supervisor_dirty = true;
inline int supervisor_selected = 0;

struct ModeEntry {
  const char* name;
  void (*enter)();
};

// Generic stand-in for a supervisor menu entry that isn't a real
// sub-application yet -- clears the screen, shows `label` so you can tell
// which entry you're in, and sits idle until ACTION returns to Supervisor
// (on_button_event() PLACEHOLDER case). One state shared by every
// placeholder entry rather than one AppState per letter.
inline const char* placeholder_label = "";
inline bool placeholder_dirty = true;

inline void enter_placeholder(const char* label) {
  placeholder_label = label;
  app_state = AppState::PLACEHOLDER;
  placeholder_dirty = true;
}

inline void enter_placeholder_b() {
  enter_placeholder("Placeholder B");
}

inline void enter_placeholder_c() {
  enter_placeholder("Placeholder C");
}

inline void enter_placeholder_d() {
  enter_placeholder("Placeholder D");
}

inline void enter_placeholder_e() {
  enter_placeholder("Placeholder E");
}

// Only call from the main task -- draws, same rule as supervisor_render().
inline void placeholder_render(LGFX& display) {
  display.setClipRect(0, 0, display.width(), SUPERVISOR_CONTENT_HEIGHT);
  display.fillRect(0, 0, display.width(), SUPERVISOR_CONTENT_HEIGHT, TFT_BLACK);

  display.setTextDatum(textdatum_t::top_left);
  display.setFont(&fonts::Font2);
  display.setTextColor(TFT_CYAN, TFT_BLACK);
  display.drawString(placeholder_label, 10, 5);
  display.drawFastHLine(0, 30, display.width(), TFT_DARKGRAY);

  display.setFont(&fonts::DejaVu18);
  display.setTextColor(TFT_WHITE, TFT_BLACK);
  display.drawString("Not implemented yet.", 10, 50);
  display.drawString("Press ACTION to return.", 10, 80);

  display.clearClipRect();
  placeholder_dirty = false;
}

// Actual recalibration (re_calibrate(), which needs the LGFX instance) runs
// from app_loop() in application.cpp on the RECALIBRATE_TOUCH state -- this
// header has no access to `lcd`, same reason enter_placeholder() only flips
// state/dirty flags and leaves drawing to app_loop().
inline void enter_recalibrate_touch() {
  app_state = AppState::RECALIBRATE_TOUCH;
}

inline const ModeEntry MODE_TABLE[] = {
#if HAS_TOUCH_INPUT && TOUCH_NEEDS_CALIBRATION
    {"Recalibrate Touch", enter_recalibrate_touch},
#else
    {"Placeholder B", enter_placeholder_b},
#endif
    {"Placeholder C", enter_placeholder_c},
    {"Placeholder D", enter_placeholder_d},
    {"Placeholder E", enter_placeholder_e},
};
constexpr int NUM_MODES = sizeof(MODE_TABLE) / sizeof(MODE_TABLE[0]);

// Only call from the main task -- this draws, same rule as
// set_touch_button_style() (touch-buttons.h).
inline void supervisor_render(LGFX& display) {
  display.setClipRect(0, 0, display.width(), SUPERVISOR_CONTENT_HEIGHT);
  display.fillRect(0, 0, display.width(), SUPERVISOR_CONTENT_HEIGHT, TFT_BLACK);

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
          supervisor_selected = (supervisor_selected + NUM_MODES - 1) % NUM_MODES;
          supervisor_dirty = true;
          break;
        case BTN_START:
          supervisor_selected = (supervisor_selected + 1) % NUM_MODES;
          supervisor_dirty = true;
          break;
        case BTN_GOAL:
          MODE_TABLE[supervisor_selected].enter();
          break;
        case BTN_RESET:
          break;
        default:
          break;
      }
      break;

    case AppState::PLACEHOLDER:
      switch (id) {
        case BTN_GOAL:
          app_state = AppState::SUPERVISOR;
          supervisor_dirty = true;
          break;
        case BTN_ARM:
        case BTN_START:
        case BTN_RESET:
          break;
        default:
          break;
      }
      break;
  }
}
