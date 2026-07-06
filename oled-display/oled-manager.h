#pragma once

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

// Preprocessor-friendly identifiers for display selection
#define DISPLAY_SSD1306_64X32 0
#define DISPLAY_SSD1306_128X32 1
#define DISPLAY_SSD1306_128X64 2
#define DISPLAY_SH1106_128X64 3

// Active display index is normally supplied by a per-environment build flag
// (see the [oled_*] sections in platformio.ini); this is just the fallback.
#ifndef ACTIVE_DISPLAY_INDEX
#define ACTIVE_DISPLAY_INDEX DISPLAY_SSD1306_128X64
#endif

// 1. Simple, clean names as an enum mapped to macro values
enum DisplayType {
  SSD1306_64X32 = DISPLAY_SSD1306_64X32,    // 64x32 OLED, standard I2C mini screen
  SSD1306_128X32 = DISPLAY_SSD1306_128X32,  // 128x32 OLED, standard horizontal ribbon screen
  SSD1306_128X64 = DISPLAY_SSD1306_128X64,  // 128x64 OLED, standard square-ish layout
  SH1106_128X64 = DISPLAY_SH1106_128X64,    // 128x64 OLED, variant driver layout
  DISPLAY_TYPE_COUNT                        // Keeps track of total array size automatically
};

// 2. Self-contained conditional instantiation using 'inline'
#if ACTIVE_DISPLAY_INDEX == DISPLAY_SSD1306_64X32
inline U8G2_SSD1306_64X32_1F_F_HW_I2C oled(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
#elif ACTIVE_DISPLAY_INDEX == DISPLAY_SSD1306_128X32
inline U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C oled(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
#elif ACTIVE_DISPLAY_INDEX == DISPLAY_SSD1306_128X64
inline U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
#elif ACTIVE_DISPLAY_INDEX == DISPLAY_SH1106_128X64
inline U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
#endif

// Data structure to track descriptive metadata matching the enum index
struct DisplayMetadata {
  DisplayType type;
  const char* simpleName;
  const char* u8gClass;
  const char* description;
  int defaultSda;
  int defaultScl;
  uint8_t i2cAddress;
};

// The mapping array indexed directly by the DisplayType enum values
// clang-format off
inline const DisplayMetadata displayTable[DISPLAY_TYPE_COUNT] = {
  {
    SSD1306_64X32, 
    "SSD1306_64X32", 
    "U8G2_SSD1306_64X32_1F_F_HW_I2C", 
    "64x32 I2C OLED (Very Small Form Factor)",
    8, 7, 0x3C
  },
  {
    SSD1306_128X32, 
    "SSD1306_128X32", 
    "U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C", 
    "128x32 I2C OLED (Standard Narrow Screen)",
    8, 7, 0x3C
  },
  {
    SSD1306_128X64, 
    "SSD1306_128X64", 
    "U8G2_SSD1306_128X64_NONAME_F_HW_I2C", 
    "128x64 I2C OLED (Standard Square Screen)",
    21, 22, 0x3C 
  },
  {
    SH1106_128X64, 
    "SH1106_128X64", 
    "U8G2_SH1106_128X64_NONAME_F_HW_I2C", 
    "128x64 I2C OLED (Alternate chip variant)",
    21, 22, 0x3C
  }
};
// clang-format on

// Handles Wire initialization and screen boot sequence dynamically using the active index parameters.
inline void initDisplay() {
  const DisplayMetadata& config = displayTable[ACTIVE_DISPLAY_INDEX];
  Wire.begin(config.defaultSda, config.defaultScl);
  oled.setI2CAddress(config.i2cAddress * 2);
  oled.begin();
}

inline void initDisplay(int sda_pin, int scl_pin, int addr) {
  Wire.begin(sda_pin, scl_pin);
  oled.setI2CAddress(addr * 2);
  oled.begin();
}

struct FontData {
  const uint8_t* font;
  const char* name;
  const char* long_name;
};

// clang-format off
inline const FontData fonts[] = {
  {u8g2_font_helvR12_tr,       "helvR12",            "u8g2_font_helvR12_tr"},      // 2 lines of  7 (17px high)
  {u8g2_font_helvB12_tr,       "helvB12",            "u8g2_font_helvB12_tr"},      // 2 lines of  7 (17px high)
  {u8g2_font_8x13_mr,          "8x13",               "u8g2_font_8x13_mr"},         // 2 lines of  8 (13px high)
  {u8g2_font_helvR08_tr,       "helvR08",            "u8g2_font_helvR08_tr"},      // 3 lines of 10 (11px high)
  {u8g2_font_5x8_mr,           "5x8 Terminal Mono",  "u8g2_font_5x8_mr"},          // 4 lines of 13 ( 8px high)
  {u8g2_font_4x6_mr,           "4x6",                "u8g2_font_4x6_mr"},          // 5 lines of 16 ( 6px high)
};
// clang-format on

inline void centre(U8G2& display, int y, const char* str) {
  int x = (display.getWidth() - display.getStrWidth(str)) / 2;
  display.drawStr(x, y, str);
}

/// @brief  Draw a title screen
///   functions like this can be placed in any module if they
//    include the #include <U8g2lib.h> header
/// @param canvas
/// @param title
inline void drawHeader(U8G2& canvas, const char* title) {
  canvas.setFont(u8g2_font_helvB12_tr);
  int w = canvas.getStrWidth(title);
  int h = canvas.getMaxCharHeight();
  int x = (canvas.getDisplayWidth() - w) / 2;
  int y = (canvas.getDisplayHeight() + h) / 2;
  canvas.clearBuffer();
  canvas.drawStr(x, y, title);
  canvas.setDrawColor(1);
  canvas.drawHLine(0, y + 2, canvas.getWidth());
  canvas.drawHLine(0, y - h + 2, canvas.getWidth());
  canvas.sendBuffer();
}