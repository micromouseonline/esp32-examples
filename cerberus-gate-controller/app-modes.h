// ----------------------------------------------------------------------------
//  app-modes.h — Central button dispatcher. Every producer (touch, gpio,
//  neokey) ultimately funnels through input_queue_drain() ->
//  BUTTON_MENU[id].onPress() -> here, regardless of which physical input
//  generated the event. No state machine yet (Step 9) -- PREV/NEXT page the
//  font demo (the only mode that exists so far); ACTION/unused just log.
// ----------------------------------------------------------------------------
#pragma once

#include <Arduino.h>

#include "font-demo.h"
#include "gui-button.h"  // ButtonID

inline void on_button_event(ButtonID id) {
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
      Serial.println("[APP] ACTION");
      break;
    case BTN_RESET:
      Serial.println("[APP] unused");
      break;
    default:
      break;
  }
}
