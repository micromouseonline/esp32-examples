// ----------------------------------------------------------------------------
//  neokey-buttons.h — NeoKey 1x4 input producer. Detects key-press edges via
//  neokey-driver.h's shared device instance and posts InputEvents. Polled
//  from the Local Input Polling Task (Core 1, see application.cpp), same
//  contract as gpio-buttons.h and touch-buttons.h.
// ----------------------------------------------------------------------------
#pragma once

#include "config.h"
#include "gui-button.h"
#include "input-events.h"

#if HAS_NEOKEY_BUTTONS

#include "neokey-driver.h"
#include "neokey-pixels.h"

inline void init_neokey_buttons() {
  init_neokey_device();
}

inline void poll_neokey_buttons() {
  neokey_device.update();
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (neokey_device.wasPressed(i)) {
      Serial.printf("[NEOKEY] key %d pressed -> %s\n", i, BUTTON_MENU[i].label);
      input_queue_post(static_cast<ButtonID>(i), InputSource::NEOKEY_BUTTON);
    }
  }
}

inline void set_neokey_button_style(ButtonID id, ButtonColour colour) {
  if (id < NUM_BUTTONS) {
    neokey_set_colour(static_cast<uint8_t>(id), colour);
  }
}

#else

inline void init_neokey_buttons() {}
inline void poll_neokey_buttons() {}  // no NeoKey on this board
inline void set_neokey_button_style(ButtonID, ButtonColour) {}

#endif
