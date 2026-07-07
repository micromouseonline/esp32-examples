#pragma once

#include <Arduino.h>
#include <LovyanGFX.h>
#include "config.h"
#include "font-demo.h"
// Define a type for our button action callback function
typedef void (*ButtonCallback)();

inline void onArmPressed() {
  Serial.println("TX: CMD_ARM");
}

inline void onStartPressed() {
  Serial.println("TX: CMD_START");
}

inline void onGoalPressed() {
  Serial.println("TX: CMD_GOAL");
}

inline void onResetPressed() {
  Serial.println("TX: CMD_RESET");
}

struct ButtonConfig {
  int16_t x;
  int16_t y;
  uint16_t width;
  uint16_t height;
  uint32_t outlineColor;
  uint32_t fillColor;
  uint32_t textColor;
  int textSize;
  const lgfx::IFont* font;
  const char* label;
  ButtonCallback onPress;  // The function to execute when clicked
};

// 1. Create an enum so we can track the total button count easily
enum ButtonID { BTN_ARM, BTN_START, BTN_GOAL, BTN_RESET, NUM_BUTTONS };

// Canonical colour representation across all three input producers
// (touch, gpio, neokey) -- 0xRRGGBB. See button-style.h.
using ButtonColour = uint32_t;

// clang-format off
// 3. Define the static properties array
const int bw = 62;
const int bs = (320-NUM_BUTTONS*bw)/3;
inline const ButtonConfig BUTTON_MENU[NUM_BUTTONS] = {
  //  X,                Y,   W,  H,   Outline,   Fill,            Text,      size, font,            "Label",   Callback
  { bw/2 + 0 * (bw+bs), 220,  bw, 32, TFT_WHITE, Color::REEVO_BLUE, TFT_WHITE, 1,    &fonts::DejaVu12, "ARM",   onArmPressed},
  { bw/2 + 1 * (bw+bs), 220,  bw, 32, TFT_WHITE, Color::REEVO_BLUE, TFT_WHITE, 1,    &fonts::DejaVu12, "START", onStartPressed  },
  { bw/2 + 2 * (bw+bs), 220,  bw, 32, TFT_WHITE, Color::REEVO_BLUE, TFT_WHITE, 1,    &fonts::DejaVu12, "GOAL",  onGoalPressed },
  { bw/2 + 3 * (bw+bs), 220,  bw, 32, TFT_WHITE, Color::REEVO_BLUE, TFT_WHITE, 1,    &fonts::DejaVu12, "RESET",  onResetPressed }
};

// clang-format on

class CustomButton : public LGFX_Button {
 private:
  LovyanGFX* _lcdPtr = nullptr;  // Explicitly keep a pointer to the display driver context
  ButtonConfig _cfg;

 public:
  CustomButton() = default;

  CustomButton(const ButtonConfig& cfg) : _cfg(cfg) {}

  void init(LGFX& gfx, const ButtonConfig& cfg) {
    _cfg = cfg;
    _lcdPtr = &gfx;
    LGFX_Button::initButton(_lcdPtr, _cfg.x, _cfg.y, _cfg.width, _cfg.height, _cfg.outlineColor, _cfg.fillColor, _cfg.textColor, _cfg.label, _cfg.textSize);
  }

  // bool contains(int tx, int ty) {
  //   int top = _cfg.y - _cfg.height / 2;
  //   if (ty < top) {
  //     return false;
  //   }

  //   int bottom = _cfg.y + _cfg.height / 2;
  //   if (ty > bottom) {
  //     return false;
  //   }

  //   int left = _cfg.x - _cfg.width / 2;
  //   if (tx < left) {
  //     return false;
  //   }

  //   int right = _cfg.x + _cfg.width / 2;
  //   if (tx > right) {
  //     return false;
  //   }
  //   return true;
  // }

  void draw(bool inverted = false) {
    if (_lcdPtr) {
      const lgfx::IFont* previousFont = _lcdPtr->getFont();
      _lcdPtr->setFont(_cfg.font);
      drawButton(inverted);
      _lcdPtr->setFont(previousFont);
    }
  }

  // Only call from the main task, never from the Core-1 input polling task
  // -- this redraws (see button-style.h).
  void setStyle(ButtonColour colour) {
    setFillColor(colour);  // LGFX_Button API; stores as rgb888 == 0xRRGGBB
    draw(false);
  }

  // Safe wrapper execution to fire off the bound callback pointer
  void execute() {
    if (_cfg.onPress != nullptr) {
      _cfg.onPress();
    }
  }
};
