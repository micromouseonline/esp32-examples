// ----------------------------------------------------------------------------
//  button-style.h — Unified "set this button's colour" API across all three
//  input producers (touch, gpio, neokey). Every ButtonID has an associated
//  colour that can be set regardless of which producer(s) a given board has
//  active; inactive producers absorb the call as a no-op, same "always safe
//  to call" contract as poll_gpio_buttons()/poll_touch_buttons()/
//  poll_neokey_buttons() already have.
//
//  Not called from application.cpp yet -- nothing in the app has state to
//  reflect in colour yet (Step 6+'s job, see INPUT-SKELETON-PLAN.md).
// ----------------------------------------------------------------------------
#pragma once

#include "gpio-buttons.h"
#include "gui-button.h"  // ButtonID, ButtonColour
#include "neokey-buttons.h"
#include "touch-buttons.h"

// Only call from the main task (app_loop / a future render context) -- never
// from the Core-1 input polling task -- set_touch_button_style() may
// redraw, and that task must never draw (see Step 4 of
// INPUT-SKELETON-PLAN.md).
inline void set_button_style(ButtonID id, ButtonColour colour) {
  set_gpio_button_style(id, colour);
  set_touch_button_style(id, colour);
  set_neokey_button_style(id, colour);
}
