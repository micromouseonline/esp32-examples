#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <freertos/task.h>

#include "application.h"

#include "../common/board-config.h"
#include "../common/board-id.h"
#include "../common/esp32_info.h"
#include "../common/secrets.h"
#include "../common/wifi-manager.h"

#include "display.h"

#include "app-modes.h"
#include "display-basics.h"
#include "font-demo.h"
#include "gpio-buttons.h"
#include "input-events.h"
#include "neokey-buttons.h"
#include "touch-buttons.h"
#include "touch-calibration.h"

StatusLED statusIndicator;
static LGFX lcd;
static LGFX_Sprite sprite(&lcd);  // Create an instance of LGFX_Sprite if you plan to use sprites.
static TaskHandle_t input_poll_task_handle = nullptr;

// Local Input Polling Task (Core 1, per DESIGN-REQUIREMENT.md). Owns all
// input-device reads (GPIO + touch); the main task (app_setup/app_loop) owns
// the display and must never poll a device from here to avoid double-reads.
static void input_poll_task(void *) {
  const TickType_t period = pdMS_TO_TICKS(INPUT_POLL_PERIOD_MS);
  for (;;) {
    poll_gpio_buttons();
    poll_touch_buttons(lcd);
    poll_neokey_buttons();
    vTaskDelay(period);
  }
}

void app_setup() {
  // get the serial connection kicked off.
  Serial.begin(115200);
  delay(1000);
  Serial.println("Well, hello there");
  statusIndicator.begin();
  lcd.init();  // setting up the display takes 500ms
  lcd.setRotation(LCD_ROTATION);
#if HAS_TOUCH_INPUT && TOUCH_NEEDS_CALIBRATION
  // Only resistive touch (XPT2046, both CYD2USB boards) needs this --
  // capacitive touch (FT6336U, CST820) already reports screen-pixel
  // coordinates. Loads stored calibration from NVS, or launches the
  // interactive wizard if none is stored yet. Safe to call here: the input
  // polling task (below) hasn't started yet, so there's no concurrent
  // lcd.getTouch() to race against.
  calibrate(lcd);
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
  init_touch_buttons(lcd);
  init_neokey_buttons();
  supervisor_render(lcd);  // boot into Supervisor (app_state's default)
  xTaskCreatePinnedToCore(input_poll_task, "input_poll", 4096, nullptr, 1, &input_poll_task_handle, 1);
}

void app_loop() {
  yield();
  input_queue_drain();
  switch (app_state) {
    case AppState::SUPERVISOR:
      if (supervisor_dirty) {
        supervisor_render(lcd);
      }
      break;
    case AppState::FONT_DEMO:
      if (font_demo_dirty) {
        font_demo_render(lcd);
      }
      break;
    case AppState::RECALIBRATE_TOUCH:
      // On-demand recalibration (Supervisor menu), unlike the app_setup()
      // call, runs after input_poll_task has started -- suspend it first so
      // re_calibrate()'s lcd.calibrateTouch() doesn't race poll_touch_buttons()'s
      // lcd.getTouch() calls on Core 1 for the same touch driver.
      if (input_poll_task_handle) {
        vTaskSuspend(input_poll_task_handle);
      }
      re_calibrate(lcd);
      if (input_poll_task_handle) {
        vTaskResume(input_poll_task_handle);
      }
      app_state = AppState::SUPERVISOR;
      supervisor_dirty = true;
      break;
  }
  delay(50);
}