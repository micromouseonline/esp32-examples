// ----------------------------------------------------------------------------
//  config.h — Unified hardware profile & application tunables.
// ----------------------------------------------------------------------------
#pragma once

#include <stdint.h>

// ============================================================================
//  1. HARDWARE CONFIGURATION (DISPLAY & TOUCH)
//  Board wiring/pin facts live in common/boards/ -- shared across every
//  project that targets these boards. Add new boards as sibling files there,
//  not as another branch here.
// ============================================================================

#ifdef BOARD_M5_CORE
#include "../common/boards/m5-core.h"
#elif defined(BOARD_CYD2USB_DIYMALLS_ILI9341)
#include "../common/boards/cyd2usb-diymalls-ili9341.h"
#elif defined(BOARD_CYD2USB_DIYMALLS_ST7789)
#include "../common/boards/cyd2usb-diymalls-st7789.h"
#elif defined(BOARD_JC2432W328C)
#include "../common/boards/jc2432w328c.h"
#else  // Default: Freenove FNK0104B Configuration
#include "../common/boards/cyd-touch-freenove.h"
#endif

// ============================================================================
//  2. APPLICATION SETTINGS & BEHAVIOR
// ============================================================================

// Local Input Polling Task period (Core 1) -- DESIGN-REQUIREMENT.md specifies
// GPIO/NeoKey/touch are all polled sequentially every 15ms from one task.
constexpr int INPUT_POLL_PERIOD_MS = 15;

// ----- Colors (RGB565 helpers) -----
#define RGB565(r, g, b) (((r)&0xF8) << 8 | ((g)&0xFC) << 3 | ((b)&0xF8) >> 3)
namespace Color {
constexpr uint16_t ACCENT = RGB565(43, 87, 220);
}  // namespace Color
#undef RGB565