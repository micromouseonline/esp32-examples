#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <nvs_flash.h>
#include "display.h"

inline Preferences prefs;
inline uint16_t touchCalData[8];
const char* PREFS_NAMESPACE = "touch-cal";

inline void factory_reset() {
  Serial.println("\n=========================================");
  Serial.println("WARNING: Initiating complete NVS Flash Wipe...");
  Serial.println("=========================================");

  // Erase the default NVS partition
  esp_err_t err = nvs_flash_erase();

  if (err == ESP_OK) {
    Serial.println("[SUCCESS] Entire NVS storage has been formatted!");
    Serial.println("All old project namespaces and settings are GONE.");
  } else {
    Serial.printf("[ERROR] NVS Erase failed! Code: 0x%X\n", err);
  }

  // Reinitialize it so it's a completely blank, healthy slate
  nvs_flash_init();

  Serial.println("=========================================");
  Serial.println("Done! You can now upload your touch screen project safely.");
  Serial.println("=========================================");
}

/**
 * Triggers the library's built-in calibration wizard, saves the resulting
 * calibration data array directly to NVS, and applies it.
 */
inline void re_calibrate(LGFX& lcd) {
  // 1. Wipe old calibration from NVS
  prefs.begin(PREFS_NAMESPACE, false);
  prefs.clear();

  Serial.println("Starting native LovyanGFX calibration wizard...");

  // 2. Clear screen and display basic instructions
  lcd.fillScreen(TFT_BLACK);
  lcd.setTextColor(TFT_WHITE);
  lcd.setTextSize(2);
  lcd.drawCenterString("Touch Calibration", lcd.width() / 2, lcd.height() / 2 - 20);

  // 3. Let LovyanGFX run its native 4-corner calibration routine
  // Parameters: (data_storage_array, color_of_target, color_of_bg, size_of_target)
  lcd.calibrateTouch(touchCalData, TFT_RED, TFT_BLACK, 15);

  // 4. Activate the calibration immediately
  lcd.setTouchCalibrate(touchCalData);

  // 5. Save the calibration data block to Preferences
  prefs.putBytes("cal_data", touchCalData, sizeof(touchCalData));
  prefs.putBool("calibrated", true);
  prefs.end();

  lcd.fillScreen(TFT_BLACK);
  lcd.drawCenterString("Calibration Saved!", lcd.width() / 2, lcd.height() / 2);
  delay(1000);
}

/**
 * Checks for existing calibration. Loads it if found,
 * otherwise kicks off a fresh calibration process.
 */
inline void calibrate(LGFX& lcd) {
  prefs.begin(PREFS_NAMESPACE, true);  // Open in read-only mode
  bool isCalibrated = prefs.getBool("calibrated", false);

  if (isCalibrated) {
    // Read the array block directly back into memory
    prefs.getBytes("cal_data", touchCalData, sizeof(touchCalData));
    prefs.end();

    // Apply it to the display instance
    lcd.setTouchCalibrate(touchCalData);
    Serial.println("Native touch calibration loaded successfully.");
  } else {
    prefs.end();
    Serial.println("No calibration data found. Launching wizard...");
    re_calibrate(lcd);
  }
}

inline void show_touch_point(LGFX& lcd) {
  lgfx::touch_point_t raw_point;
  int32_t screenX, screenY;

  // 1. Fetch RAW values coming off the FT6336 registers
  // This bypasses offset_rotation, mapping arrays, and min/max calibration clips.
  uint32_t raw_count = lcd.getTouchRaw(&raw_point, 1);  // 1 = maximum number of fingers to scan

  // 2. Fetch the standard translated layout coordinates
  bool screen_is_touched = lcd.getTouch(&screenX, &screenY);

  if (raw_count > 0 && screen_is_touched) {
    lcd.setTextColor(TFT_YELLOW, TFT_BLACK);
    lcd.setCursor(10, 40);
    lcd.setFont(&fonts::DejaVu9);
    lcd.println("--- TOUCH DETECTED ---");
    lcd.printf("RAW CHIP REGISTERS -> X: %3d,   Y: %3d\n", raw_point.x, raw_point.y);
    int x = map(raw_point.y, 0, 320, 0, 320);
    int y = map(raw_point.x, 240, 0, 0, 240);
    lcd.printf("     MAPPED SCREEN -> X: %3d,   Y: %3d\n", x, y);
    lcd.printf(" TRANSLATED SCREEN -> X: %3d,   Y: %3d\n", screenX, screenY);
    delay(10);  // Small debounce window to keep the stream readable
  }
}