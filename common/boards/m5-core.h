// ----------------------------------------------------------------------------
//  boards/m5-core.h — M5Stack Core hardware profile.
//  Shared across every project that targets this board -- add new boards as
//  sibling files here, not inside a project's own config.h.
//
//  Display/backlight are LovyanGFX-autodetected (see display.h's
//  LGFX_AUTODETECT branch), so only the non-autodetected specifics (button
//  pins, capability flags) live here.
// ----------------------------------------------------------------------------
#pragma once

// ----- M5Stack Core Native Specifications -----
#define DISPLAY_PANEL_M5STACK_CORE
constexpr int PANEL_NATIVE_WIDTH = 320;  // Core native is landscape (320x240)
constexpr int PANEL_NATIVE_HEIGHT = 240;
constexpr int DISPLAY_WIDTH = 320;
constexpr int DISPLAY_HEIGHT = 240;
constexpr int DISPLAY_ROTATION = 1;
constexpr bool INVERT_COLORS = true;
constexpr bool BGR_ORDER = false;
constexpr int PANEL_X_OFFSET = 0;
constexpr int PANEL_Y_OFFSET = 0;

// ----- Physical Buttons (A/B/C, active-LOW with onboard pull-ups) -----
constexpr int PIN_BUTTON_A = 39;
constexpr int PIN_BUTTON_B = 38;
constexpr int PIN_BUTTON_C = 37;
constexpr unsigned long BUTTON_C_LONG_PRESS_MS = 600;  // hold threshold for the double-duty button

// ----- Input Capability Flags -----
#define HAS_TOUCH_INPUT 0
#define HAS_GPIO_BUTTONS 1
#define HAS_NEOKEY_BUTTONS 0
#define TOUCH_SHARES_DISPLAY_SPI_BUS 0
#define TOUCH_NEEDS_CALIBRATION 0  // no touch controller on this board

constexpr int LCD_ROTATION = 1;
