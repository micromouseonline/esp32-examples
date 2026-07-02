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

inline void show_fonts_structured(LGFX& display) {
  const int TOTAL_FONTS = sizeof(demoFonts) / sizeof(demoFonts[0]);
  int currentFontIndex = 0;

  // Screen Layout Layout Definitions
  const int buttonHeight = 40;
  const int topZoneHeight = 30;
  const int bottomZoneY = display.height() - buttonHeight;

  while (true) {
    display.fillScreen(TFT_BLACK);

    // Get the current active font entry
    FontEntry current = demoFonts[currentFontIndex];

    // ==========================================
    // 1. RENDER HEADER (Top of screen)
    // ==========================================
    display.setTextDatum(textdatum_t::top_left);
    display.setFont(&fonts::Font2);
    display.setTextColor(TFT_CYAN, TFT_BLACK);
    display.drawString(current.name, 10, 5);

    // Horizontal divider line below the header
    display.drawFastHLine(0, topZoneHeight, display.width(), TFT_DARKGRAY);

    // ==========================================
    // 2. RENDER FONT SAMPLES (Middle of screen)
    // ==========================================
    display.setFont(current.font);
    display.setTextColor(TFT_WHITE, TFT_BLACK);

    int sampleY = topZoneHeight + 15;

    // Line 1: Numerals
    display.drawCenterString("01:23.678", 160, sampleY);
    sampleY += display.fontHeight() + 5;  // spacing padding
    display.drawCenterString("888888888", 160, sampleY);

    sampleY += display.fontHeight() + 0;  // spacing padding
    display.drawString("The five boxing wizards jump quickly.", 10, sampleY);
    sampleY += display.fontHeight() + 0;  // spacing padding
    display.drawString("Pack my box with five dozen liquor jugs.", 10, sampleY);

    // ==========================================
    // 3. RENDER NAVIGATION BUTTONS (Bottom of screen)
    // ==========================================
    display.drawFastHLine(0, bottomZoneY, display.width(), TFT_DARKGRAY);

    int btnWidth = display.width() / 2;
    display.setFont(&fonts::FreeSansBold9pt7b);
    display.setTextDatum(textdatum_t::middle_center);

    // PREV Button (Left side)
    if (currentFontIndex > 0) {
      display.fillRect(0, bottomZoneY + 2, btnWidth - 2, buttonHeight - 2, TFT_NAVY);
      display.setTextColor(TFT_WHITE, TFT_NAVY);
      display.drawString("<< PREV", btnWidth / 2, bottomZoneY + (buttonHeight / 2));
    }

    // NEXT Button (Right side)
    if (currentFontIndex < TOTAL_FONTS - 1) {
      display.fillRect(btnWidth + 2, bottomZoneY + 2, btnWidth - 2, buttonHeight - 2, TFT_NAVY);
      display.setTextColor(TFT_WHITE, TFT_NAVY);
      display.drawString("NEXT >>", btnWidth + (btnWidth / 2), bottomZoneY + (buttonHeight / 2));
    }

    // ==========================================
    // 4. TOUCH INPUT INTERACTION HANDLING
    // ==========================================
    bool selectionMade = false;

    while (!selectionMade) {
      uint16_t tx, ty;
      if (display.getTouch(&tx, &ty)) {
        // Check if touch coordinates fall inside the bottom button zone
        if (ty > bottomZoneY) {
          // Left Side Tapped (PREV)
          if (tx < btnWidth) {
            if (currentFontIndex > 0) {
              currentFontIndex--;
              selectionMade = true;
            }
          }
          // Right Side Tapped (NEXT)
          else {
            if (currentFontIndex < TOTAL_FONTS - 1) {
              currentFontIndex++;
              selectionMade = true;
            }
          }
        }

        // Keep loop locked until the user lifts their finger off the glass
        while (display.getTouch(&tx, &ty)) {
          delay(10);
        }
      }
      delay(10);
    }
  }
}