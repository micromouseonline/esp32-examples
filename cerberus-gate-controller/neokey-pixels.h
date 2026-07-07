// ----------------------------------------------------------------------------
//  neokey-pixels.h — NeoKey 1x4 LED-control facade. Independent of the
//  button/input-event side (neokey-buttons.h): this header never touches
//  ButtonID/InputSource/input_queue_post, purely arbitrary key-index colour
//  control, so status-colour feedback doesn't need to know anything about
//  button debounce internals and vice versa. Both facades read/write the
//  same shared `neokey_device` instance (neokey-driver.h) -- there's only
//  one physical seesaw chip on one I2C address, so there's exactly one
//  owner of its init (neokey-buttons.h's init_neokey_buttons(), the first
//  and only caller of init_neokey_device()). Calling the setters below
//  before that init has run is a no-op-ish failure (the underlying seesaw
//  write will simply not be acknowledged), same ordering assumption as
//  input_queue_init() has elsewhere in this codebase.
// ----------------------------------------------------------------------------
#pragma once

#include "config.h"

#if HAS_NEOKEY_BUTTONS

#include "neokey-driver.h"

inline bool neokey_set_colour(uint8_t key, uint32_t colour) {
  return neokey_device.setColour(key, colour);
}

inline bool neokey_set_all(uint32_t colour) {
  return neokey_device.setAllColour(colour);
}

#else

inline bool neokey_set_colour(uint8_t, uint32_t) {
  return false;
}
inline bool neokey_set_all(uint32_t) {
  return false;
}

#endif
