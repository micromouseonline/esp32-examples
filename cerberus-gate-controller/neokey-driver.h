// ----------------------------------------------------------------------------
//  neokey-driver.h — Low-level Adafruit NeoKey 1x4 (seesaw, I2C) hardware
//  driver. Owns the I2C bus and the one physical device instance; everything
//  else touching the NeoKey (neokey-buttons.h, neokey-pixels.h) reads/writes
//  through the single `neokey_device` object defined here rather than
//  re-initializing the seesaw chip themselves.
//
//  Project-local for now -- no other project in this repo uses a NeoKey yet.
//  Hoist to common/ if that changes.
// ----------------------------------------------------------------------------
#pragma once

#include "config.h"

#if HAS_NEOKEY_BUTTONS

#include <Arduino.h>
#include <Wire.h>

#include "Adafruit_NeoKey_1x4.h"
#include "gui-button.h"  // NUM_BUTTONS
#include "seesaw_neopixel.h"

// NeoPixel colour helpers (0xRRGGBB), used by neokey-pixels.h.
constexpr uint32_t NP_RED = 0xFF0000;
constexpr uint32_t NP_GREEN = 0x00FF00;
constexpr uint32_t NP_BLUE = 0x0000FF;  // pretty dark
constexpr uint32_t NP_YELLOW = 0xFFFF00;
constexpr uint32_t NP_MAGENTA = 0xFF00FF;
constexpr uint32_t NP_CYAN = 0x00FFFF;
constexpr uint32_t NP_WHITE = 0xFFFFFF;
constexpr uint32_t NP_OFF = 0x000000;

// NeoKey's fixed hardware default I2C address (not board-specific).
constexpr uint8_t NEOKEY_I2C_ADDR = 0x30;

class Neokey {
 public:
  // I2C port 1, not 0 -- LovyanGFX's I2C touch driver (display.h,
  // cfg.i2c_port = 0) owns port 0 directly through ESP-IDF, not through
  // Arduino's Wire object. A future touch+NeoKey board needs the NeoKey on
  // its own bus to avoid contention. PIN_NEOKEY_SDA/PIN_NEOKEY_SCL are owned
  // by whichever board profile in common/boards/*.h sets
  // HAS_NEOKEY_BUTTONS=1 (not defined here), same pattern as PIN_TOUCH_SDA.
  Neokey() : myWire(1), neokey(NEOKEY_I2C_ADDR, &myWire) {}

  void setup() {
    myWire.begin(PIN_NEOKEY_SDA, PIN_NEOKEY_SCL);
    if (!neokey.begin(NEOKEY_I2C_ADDR)) {
      // Deliberately not a while(1) hang: this runs from the shared Core-1
      // input polling task (see application.cpp), so a missing NeoKey must
      // not brick every other input producer sharing that task.
      Serial.println("[NEOKEY] not found, check wiring");
      return;
    }
    setAllColour(NP_OFF);  // Turn off all pixels initially
    raw_buttons = neokey.read();
  }

  uint8_t update() {
    raw_buttons = neokey.read();
    uint32_t now = millis();
    last_debounced = debounced_buttons;  // snapshot before any state changes

    for (uint8_t i = 0; i < NUM_KEYS; i++) {
      bool raw = (raw_buttons & (1 << i)) != 0;
      bool debounced = (debounced_buttons & (1 << i)) != 0;

      if (raw != debounced) {
        if (now - last_change_time[i] >= DEBOUNCE_INTERVAL) {
          if (raw) {
            press_start_time[i] = now;
          } else {
            release_time[i] = now;
          }

          // Commit the new stable state
          if (raw) {
            debounced_buttons |= (1 << i);
          } else {
            debounced_buttons &= ~(1 << i);
          }

          last_change_time[i] = now;
        }
      } else {
        last_change_time[i] = now;  // Still stable, reset change timer
      }
    }

    return raw_buttons;
  }

  uint8_t getButtons() {
    return debounced_buttons;
  }

  bool isPressed(uint8_t i) {
    return debounced_buttons & (1 << i);
  }

  bool wasPressed(uint8_t i) {
    return !(last_debounced & (1 << i)) && (debounced_buttons & (1 << i));
  }

  bool wasReleased(uint8_t i) {
    return (last_debounced & (1 << i)) && !(debounced_buttons & (1 << i));
  }

  bool wasReleasedAfter(uint8_t i, uint32_t hold_time_ms) {
    return wasReleased(i) && (millis() - press_start_time[i] >= hold_time_ms);
  }

  bool isLongPress(uint8_t i, uint32_t durationMs) {
    return isPressed(i) && getHoldTime(i) >= durationMs;
  }

  bool wasLongReleased(uint8_t i, uint32_t durationMs) {
    return wasReleased(i) && (release_time[i] - press_start_time[i] >= durationMs);
  }

  bool isDoublePress(uint8_t i, uint32_t windowMs = 300) {
    static uint32_t last_release[NUM_KEYS] = {0};
    if (wasReleased(i)) {
      uint32_t now = millis();
      bool doubleTap = (now - last_release[i]) <= windowMs;
      last_release[i] = now;
      return doubleTap;
    }
    return false;
  }

  bool isCombo(uint8_t mask) {
    return (getButtons() == mask);
  }

  void waitUntilReleased(uint8_t button) {
    while (isPressed(button)) {
      update();   // read fresh state
      delay(10);  // debounce-friendly sleep
    }
  }

  int waitForButtons(uint8_t mask) {
    while ((getButtons() & mask) == 0) {
      update();   // read fresh state
      delay(10);  // debounce-friendly sleep
    }
    uint8_t buttons = getButtons();
    waitUntilAllReleased();
    return buttons;
  }

  void waitUntilAllReleased() {
    while (getButtons() != 0x00) {
      update();
      delay(10);
    }
  }

  uint32_t getHoldTime(uint8_t button) {
    if (button >= NEOKEY_1X4_KEYS || !isPressed(button))
      return 0;
    return millis() - press_start_time[button];
  }

  bool setColour(uint8_t button, uint32_t colour) {
    if (button >= NEOKEY_1X4_KEYS) {
      return false;  // Invalid button index
    }
    neokey.pixels.setPixelColor(button, colour);
    neokey.pixels.show();
    return true;
  }

  bool setAllColour(uint32_t colour) {
    for (int i = 0; i < NEOKEY_1X4_KEYS; i++) {
      neokey.pixels.setPixelColor(i, colour);
    }
    neokey.pixels.show();
    return true;
  }

 private:
  TwoWire myWire;
  Adafruit_NeoKey_1x4 neokey;

  static const uint8_t NUM_KEYS = NEOKEY_1X4_KEYS;

  uint8_t raw_buttons = 0;        // From neokey.read()
  uint8_t debounced_buttons = 0;  // Stable debounced state
  uint8_t last_debounced = 0;

  uint32_t last_change_time[NUM_KEYS] = {0};
  uint32_t press_start_time[NUM_KEYS] = {0};
  uint32_t release_time[NUM_KEYS] = {0};

  const uint32_t DEBOUNCE_INTERVAL = 20;  // ms
};

inline Neokey neokey_device;

inline void init_neokey_device() {
  neokey_device.setup();
}

#endif  // HAS_NEOKEY_BUTTONS
