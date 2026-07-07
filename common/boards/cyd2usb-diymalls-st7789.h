// ----------------------------------------------------------------------------
//  boards/cyd2usb-diymalls-st7789.h — Sunton "CYD2USB" hardware profile,
//  DIYMALLS variant with an ST7789 panel driver (ESP32-2432S028R family,
//  micro-USB + USB-C).
//  Shared across every project that targets this board -- add new boards as
//  sibling files here, not inside a project's own config.h.
//
//  VERIFIED working (boots, displays, touch responds) on the board
//  purchased from https://www.amazon.co.uk/dp/B0CQX9Q68P -- see
//  base-boards.ini's base_cyd2usb_diymalls_st7789 section.
//
//  Started as a copy of cyd2usb-diymalls-ili9341.h, then corrected against
//  a known-working TFT_eSPI config for this exact physical board (a
//  separate maze-timer-cyd project). That reference confirmed: same pin
//  assignments as the ILI9341 variant (panel + touch), but a DIFFERENT
//  panel driver chip -- ST7789, not ILI9341. Visually-identical CYD2USB
//  boards apparently ship with different panel controllers between
//  batches/sellers; this is why the ILI9341 variant worked with ILI9341
//  but this one streaked with it.
// ----------------------------------------------------------------------------
#pragma once

#define DISPLAY_PANEL_ST7789
#define DISPLAY_TOUCH_XPT2046  // resistive touch, own dedicated SPI pins (NOT shared with the panel)

// ----- Panel Pinout (SPI) -----
// Pin assignments cross-confirmed against the maze-timer-cyd TFT_eSPI
// reference config -- same pins as the ILI9341 variant.
constexpr int PIN_LCD_MOSI = 13;
constexpr int PIN_LCD_MISO = 12;
constexpr int PIN_LCD_SCLK = 14;
constexpr int PIN_LCD_CS = 15;
constexpr int PIN_LCD_DC = 2;
constexpr int PIN_LCD_RST = -1;         // tied to system reset
constexpr int PIN_LCD_BL = 21;          // Backlight pin
constexpr int SPI_WRITE_HZ = 55000000;  // matches the reference config's SPI_FREQUENCY
constexpr bool BL_ACTIVE_HIGH = true;

// ----- Panel Orientation & Dimensions -----
constexpr int PANEL_NATIVE_WIDTH = 240;
constexpr int PANEL_NATIVE_HEIGHT = 320;
constexpr int DISPLAY_WIDTH = 320;
constexpr int DISPLAY_HEIGHT = 240;
constexpr int DISPLAY_ROTATION = 1;
// INVERT_COLORS=false and PANEL_OFFSET_ROTATION=0 are the confirmed-working
// values on real hardware (see the file header) under the ST7789 driver. If
// a different unit shows inverted colors or mirrored text/graphics, flip
// INVERT_COLORS or try PANEL_OFFSET_ROTATION values 2/4/6 (LovyanGFX
// MADCTL mirror offset).
constexpr bool INVERT_COLORS = false;
constexpr bool BGR_ORDER = true;
constexpr int PANEL_X_OFFSET = 0;
constexpr int PANEL_Y_OFFSET = 0;
constexpr int PANEL_OFFSET_ROTATION = 0;

// ----- Touch Configuration (XPT2046) -----
// Dedicated SPI pins, physically separate from the panel's bus (unlike the
// capacitive FT6336U board, this touch chip is NOT on shared MOSI/MISO/SCLK).
// GPIO36/39 are input-only on classic ESP32, used here for the two
// touch-chip-to-ESP32 signals (DOUT, IRQ). Pins cross-confirmed against the
// maze-timer-cyd TFT_eSPI reference config -- same as the ILI9341 variant.
constexpr int PIN_TOUCH_CLK = 25;
constexpr int PIN_TOUCH_MOSI = 32;  // T_DIN
constexpr int PIN_TOUCH_MISO = 39;  // T_DOUT (input-only pin)
constexpr int PIN_TOUCH_CS = 33;
constexpr int PIN_TOUCH_IRQ = 36;  // input-only pin
constexpr int TOUCH_SPI_HZ = 1000000;

// ----- Input Capability Flags -----
#define HAS_TOUCH_INPUT 1
#define HAS_GPIO_BUTTONS 0
#define HAS_NEOKEY_BUTTONS 0
#define TOUCH_SHARES_DISPLAY_SPI_BUS 0  // XPT2046 has its own dedicated SPI pins, separate from the panel

constexpr int LCD_ROTATION = 3;
