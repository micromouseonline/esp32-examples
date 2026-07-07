// ----------------------------------------------------------------------------
//  app-modes.h — Central button dispatcher. Every producer (touch, gpio,
//  neokey) ultimately funnels through input_queue_drain() ->
//  BUTTON_MENU[id].onPress() -> here, regardless of which physical input
//  generated the event. No state machine yet (Step 9) -- for now this just
//  logs the button's new semantic name.
// ----------------------------------------------------------------------------
#pragma once

#include <Arduino.h>

#include "gui-button.h"  // ButtonID

inline void on_button_event(ButtonID id) {
  switch (id) {
    case BTN_ARM:
      Serial.println("[APP] PREV");
      break;
    case BTN_START:
      Serial.println("[APP] NEXT");
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
