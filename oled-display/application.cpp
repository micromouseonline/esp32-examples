#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

#include "application.h"

#include "../common/board-config.h"
#include "../common/board-id.h"
#include "../common/esp32_info.h"
#include "../common/secrets.h"
#include "../common/wifi-manager.h"

#include "button.h"
#include "oled-manager.h"

StatusLED statusIndicator;

#define SDA_PIN 8
#define SCL_PIN 7

const int BUTTON_UP = 10;   // Pin to go to the next font
const int BUTTON_DOWN = 9;  // Pin to go to the previous font

// These default to Active Low (uses internal INPUT_PULLUP)
DebouncedButton buttonUp(BUTTON_UP);
DebouncedButton buttonDown(BUTTON_DOWN);

int currentFontIndex = 0;
const int totalFonts = sizeof(fonts) / sizeof(FontData);

void updateDisplay() {
  oled.clearBuffer();

  int screenWidth = oled.getWidth();
  int screenHeight = oled.getHeight();
  int footerHeight = 11;  // Give the bottom bar 11 pixels of room

  // === TOP VIEWPORT: THE SAMPLES ===
  oled.setClipWindow(0, 0, screenWidth, screenHeight - footerHeight);
  oled.setFont(fonts[currentFontIndex].font);

  int h = oled.getMaxCharHeight();
  char buf[20];
  oled.drawStr(0, h - 1, "01234567890123456789");
  centre(oled, (h - 1) * 2, "START");

  // === BOTTOM VIEWPORT: THE FONT LABEL ===
  oled.setClipWindow(0, screenHeight - footerHeight, screenWidth, screenHeight);

  // Clear just the footer background
  oled.setDrawColor(0);
  oled.drawBox(0, screenHeight - footerHeight, screenWidth, footerHeight);
  oled.setDrawColor(1);

  oled.setFont(u8g2_font_5x7_tr);
  snprintf(buf, sizeof(buf), "%2d %s", h, fonts[currentFontIndex].name);

  // Render string slightly above the absolute bottom to prevent cut-off
  oled.drawStr(0, screenHeight - 2, buf);

  oled.setMaxClipWindow();
  oled.sendBuffer();
}

void app_setup() {
  // Let the button class handle pin configuration internally
  buttonUp.begin();
  buttonDown.begin();
  statusIndicator.begin();
  statusIndicator.setRGB(120, 0, 0);
  delay(2000);
  Serial.begin(SERIAL_BAUD);
  statusIndicator.setRGB(0, 10, 0);

  initDisplay(SDA_PIN, SCL_PIN, 0x3C);
  drawHeader(oled, "OLED");
  delay(1000);
  statusIndicator.turnOff();
  updateDisplay();
}

void app_loop() {
  bool needsUpdate = false;

  if (buttonUp.wasPressed()) {
    currentFontIndex = (currentFontIndex + 1) % totalFonts;
    needsUpdate = true;
  }

  if (buttonDown.wasPressed()) {
    currentFontIndex = (currentFontIndex - 1 + totalFonts) % totalFonts;
    needsUpdate = true;
  }

  // Only rewrite to the display if the font index actually changed
  if (needsUpdate) {
    updateDisplay();
  }
}