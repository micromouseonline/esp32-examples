#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include "application.h"

#include "../common/board-config.h"
#include "../common/board-id.h"
#include "../common/esp32_info.h"
#include "../common/secrets.h"
#include "../common/wifi-manager.h"

#include "display.h"

#include "display-basics.h"
#include "font-demo.h"
#include "gpio-buttons.h"
#include "gui-button.h"
#include "input-events.h"
#include "touch-calibration.h"

StatusLED statusIndicator;
static LGFX lcd;
static LGFX_Sprite sprite(&lcd);  // Create an instance of LGFX_Sprite if you plan to use sprites.

CustomButton buttons[NUM_BUTTONS];

void init_ui(LGFX &lcd) {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    buttons[i].init(lcd, BUTTON_MENU[i]);
    buttons[i].draw(false);  // Draw the button unpressed
  }
}

void loop_ui(LGFX &lcd) {
  int32_t touchX = 0;
  int32_t touchY = 0;
  bool touched = lcd.getTouch(&touchX, &touchY);
  for (int i = 0; i < NUM_BUTTONS; i++) {
    buttons[i].press(touched && buttons[i].contains(touchX, touchY));
    if (buttons[i].justPressed()) {
      buttons[i].draw(true);  // Visual feedback (inverted)
      input_queue_post(static_cast<ButtonID>(i), InputSource::TOUCH);
    }
    if (buttons[i].justReleased()) {
      buttons[i].draw(false);  // Visual feedback (normal)
    }
  }
}

void app_setup() {
  // get the serial connection kicked off.
  Serial.begin(115200);
  delay(1000);
  Serial.println("Well, hello there");
  statusIndicator.begin();
  lcd.init();          // setting up the display takes 500ms
  lcd.setRotation(LCD_ROTATION);
// calibrate(lcd);
#if defined(BOARD_S3_CYD_TOUCH_FREENOVE)
  // M5 Core has no touch controller (_touch is nullptr); LovyanGFX's
  // setCalibrate() dereferences it unconditionally (unlike touchCalibrate(),
  // which null-checks), so only call this where touch hardware exists.
  uint16_t touchCalData[8] = {239, 319, 239, 1, 1, 319, 1, 1};
  lcd.setTouchCalibrate(touchCalData);
#endif
  lcd.fillScreen(TFT_BLACK);
  lcd.setFont(&fonts::DejaVu56);
  lcd.setTextSize(1);
  lcd.setTextDatum(textdatum_t::middle_center);
  lcd.setTextColor(Color::REEVO_BLUE);
  lcd.drawString("READY!", lcd.width() / 2, lcd.height() / 2);
  uint32_t start_time = millis();
  // it may take anything up to 2000ms altogether to get  a serial connection
  while (!Serial && (millis() < 2000)) {
    delay(10);
  }
  uint32_t ready_time = millis();
  // just because the hardware is ready, does not mean the terminal is ready
  // so allow time for that as well
  while (millis() < 2500) {
    yield();
  }
  // Finally ready but at least we used the time to do important initialisation
  Serial.println(F("CERBERUS: gate controller"));
  Serial.printf("ready after %dms (display ready at:%dms)\n", ready_time, start_time);
  input_queue_init();
  gpio_buttons_init();
  init_ui(lcd);
#if defined(BOARD_S3_CYD_TOUCH_FREENOVE)
  // Touch-only demo utility (its internal loop only exits via getTouch()).
  // M5 Core has no touch hardware, so calling this unconditionally would
  // spin forever here and app_loop() (GPIO button polling) would never run.
  show_fonts_structured(lcd);
#endif
}

void app_loop() {
  yield();
  loop_ui(lcd);
  poll_gpio_buttons();
  input_queue_drain();
  delay(50);
}