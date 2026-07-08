// ----------------------------------------------------------------------------
//  application.cpp — cyd-font-demo boots straight into the font browser.
//  PREV/NEXT touch buttons cycle through every embedded font so you can pick
//  one for use in another application -- no menu, no other modes.
// ----------------------------------------------------------------------------
#include <Arduino.h>

#include "application.h"
#include "display.h"
#include "font-demo.h"
#include "touch-calibration.h"

static LGFX lcd;

// Font demo may leave any font active on `lcd` between frames -- reset it
// before drawing/redrawing a button, same rule cerberus-gate-controller's
// gui-button.h::CustomButton follows.
class NavButton : public LGFX_Button {
 public:
  void init(LGFX& gfx, int16_t x, int16_t y, uint16_t w, uint16_t h, const char* label) {
    // initButton() is templated on a single colour type shared by all three
    // args -- TFT_WHITE and Color::ACCENT are different underlying types, so
    // they must be cast to a common one or template deduction fails.
    uint32_t outline = TFT_WHITE;
    uint32_t fill = Color::ACCENT;
    uint32_t text = TFT_WHITE;
    initButton(&gfx, x, y, w, h, outline, fill, text, label, 1);
  }
  void draw(LGFX& gfx, bool inverted) {
    const lgfx::IFont* previous = gfx.getFont();
    gfx.setFont(&fonts::DejaVu18);
    drawButton(inverted);
    gfx.setFont(previous);
  }
};

static NavButton prev_button;
static NavButton next_button;

void app_setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.setRotation(LCD_ROTATION);
  lcd.fillScreen(TFT_BLACK);

#if HAS_TOUCH_INPUT && TOUCH_NEEDS_CALIBRATION
  calibrate(lcd);
#endif

  int16_t button_y = FONT_DEMO_CONTENT_HEIGHT + 20;
  int16_t button_w = lcd.width() / 2 - 20;
  prev_button.init(lcd, lcd.width() / 4, button_y, button_w, 36, "PREV");
  next_button.init(lcd, 3 * lcd.width() / 4, button_y, button_w, 36, "NEXT");
  prev_button.draw(lcd, false);
  next_button.draw(lcd, false);

  font_demo_render(lcd);

  Serial.println(F("CYD FONT DEMO"));
}

void app_loop() {
  int32_t touchX = 0;
  int32_t touchY = 0;
  bool touched = lcd.getTouch(&touchX, &touchY);

  prev_button.press(touched && prev_button.contains(touchX, touchY));
  next_button.press(touched && next_button.contains(touchX, touchY));

  if (prev_button.justPressed()) {
    prev_button.draw(lcd, true);
    font_demo_prev();
  }
  if (prev_button.justReleased()) {
    prev_button.draw(lcd, false);
  }
  if (next_button.justPressed()) {
    next_button.draw(lcd, true);
    font_demo_next();
  }
  if (next_button.justReleased()) {
    next_button.draw(lcd, false);
  }

  if (font_demo_dirty) {
    font_demo_render(lcd);
  }
  delay(20);
}
