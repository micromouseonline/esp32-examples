#pragma once

#include <Arduino.h>

#include "../common/fonts/DSEG7Classic_Bold20pt7b.h"
#include "display.h"

// ADD THIS LINE HERE: It defines GFXglyph and GFXfont
#include <lgfx/v1/panel/Panel_Device.hpp>

struct FontEntry {
  const lgfx::IFont* font;
  const char* name;
};

// Complete LovyanGFX Embedded Font Array
const FontEntry demoFonts[] = {
    // ==========================================
    // 1. CORE FIXED / BITMAP FONTS
    // ==========================================
    {&DSEG7Classic_Bold20pt7b, "DSEG7Classic_Bold20pt7b"},
    {&fonts::Font0, "Font 0 (Standard 8px) monospace"},
    {&fonts::Font2, "Font 2 (Small 16px) not mono"},
    {&fonts::Font4, "Font 4 (Medium 26px) not mono"},
    {&fonts::Font6, "Font 6 (Large Numeric 48px)"},
    {&fonts::Font7, "Font 7 (7-Segment Numbers 48px)"},
    {&fonts::Font8, "Font 8 (Large Numeric 75px)"},
    {&fonts::DejaVu9, "DejaVu9 not mono"},
    {&fonts::DejaVu12, "DejaVu12 not mono"},
    {&fonts::DejaVu18, "DejaVu18 not mono"},
    {&fonts::DejaVu24, "DejaVu24 not mono"},
    {&fonts::DejaVu40, "DejaVu40 not mono"},
    {&fonts::DejaVu56, "DejaVu56 not mono"},
    {&fonts::DejaVu72, "DejaVu72 not mono"},
    {&fonts::AsciiFont8x16, "AsciiFont8x16 (Fixed 8x16px) mono"},
    {&fonts::AsciiFont24x48, "AsciiFont24x48 (Fixed 24x48px) mono"},
    {&fonts::Font8x8C64, "Commodore 64 8x8 mono"},

    // ==========================================
    // 4. ADAFRUIT FREEFONTS EXTENSION PACK
    // (Uncomment these if your build profile includes them!)
    // ==========================================

    // --- FreeMono Series ---

    {&fonts::FreeMono9pt7b, "FreeMono 9pt"},
    {&fonts::FreeMono12pt7b, "FreeMono 12pt"},
    {&fonts::FreeMono18pt7b, "FreeMono 18pt"},
    {&fonts::FreeMono24pt7b, "FreeMono 24pt"},
    {&fonts::FreeMonoBold9pt7b, "FreeMono Bold 9pt"},
    {&fonts::FreeMonoBold12pt7b, "FreeMono Bold 12pt"},
    {&fonts::FreeMonoBold18pt7b, "FreeMono Bold 18pt"},
    {&fonts::FreeMonoBold24pt7b, "FreeMono Bold 24pt"},

    // --- FreeSans Series ---

    {&fonts::FreeSans9pt7b, "FreeSans 9pt not mono"},
    {&fonts::FreeSans12pt7b, "FreeSans 12pt not mono"},
    {&fonts::FreeSans18pt7b, "FreeSans 18pt not mono"},
    {&fonts::FreeSans24pt7b, "FreeSans 24pt not mono"},
    {&fonts::FreeSansBold9pt7b, "FreeSans Bold 9pt not mono"},
    {&fonts::FreeSansBold12pt7b, "FreeSans Bold 12pt not mono"},
    {&fonts::FreeSansBold18pt7b, "FreeSans Bold 18pt not mono"},
    {&fonts::FreeSansBold24pt7b, "FreeSans Bold 24pt not mono"},

};

constexpr int FONT_DEMO_TOTAL = sizeof(demoFonts) / sizeof(demoFonts[0]);

// Content stays above y=200 -- the shared touch button bar (BUTTON_MENU,
// gui-button.h) is drawn once at y=220 and never redrawn here, so
// font_demo_render() must not paint over it. PREV/NEXT/ACTION navigation is
// the shared bar's job now (Step 6's on_button_event dispatcher), not this
// file's -- this replaces the old show_fonts_structured()'s own blocking
// touch loop and hand-drawn nav buttons entirely.
constexpr int FONT_DEMO_CONTENT_HEIGHT = 200;

inline int font_demo_index = 0;
inline bool font_demo_dirty = true;

inline void font_demo_next() {
  if (font_demo_index < FONT_DEMO_TOTAL - 1) {
    font_demo_index++;
    font_demo_dirty = true;
  }
}

inline void font_demo_prev() {
  if (font_demo_index > 0) {
    font_demo_index--;
    font_demo_dirty = true;
  }
}

// Only call from the main task -- this draws, same rule as
// set_touch_button_style() (touch-buttons.h).
inline void font_demo_render(LGFX& display) {
  // A fillRect alone only bounds the background -- glyphs from large fonts
  // (e.g. the 75px numeric fonts) still paint past FONT_DEMO_CONTENT_HEIGHT
  // and over the button bar below. setClipRect() makes that physically
  // impossible: nothing drawn below can write outside this rect, regardless
  // of font size, so oversized samples get cropped instead of overwriting
  // the bar.
  display.setClipRect(0, 0, display.width(), FONT_DEMO_CONTENT_HEIGHT);
  display.fillRect(0, 0, display.width(), FONT_DEMO_CONTENT_HEIGHT, TFT_BLACK);

  FontEntry current = demoFonts[font_demo_index];

  // Header
  display.setTextDatum(textdatum_t::top_left);
  display.setFont(&fonts::Font2);
  display.setTextColor(TFT_CYAN, TFT_BLACK);
  display.drawString(current.name, 10, 5);

  const int topZoneHeight = 30;
  display.drawFastHLine(0, topZoneHeight, display.width(), TFT_DARKGRAY);

  // Font samples
  display.setFont(current.font);
  display.setTextColor(TFT_WHITE, TFT_BLACK);

  int sampleY = topZoneHeight + 15;
  display.drawCenterString("01:23.678", 160, sampleY);
  sampleY += display.fontHeight() + 5;  // spacing padding
  display.drawCenterString("888888888", 160, sampleY);

  sampleY += display.fontHeight() + 0;  // spacing padding
  display.drawString("The five boxing wizards jump quickly.", 10, sampleY);
  sampleY += display.fontHeight() + 0;  // spacing padding
  display.drawString("Pack my box with five dozen liquor jugs.", 10, sampleY);

  display.clearClipRect();
  font_demo_dirty = false;
}