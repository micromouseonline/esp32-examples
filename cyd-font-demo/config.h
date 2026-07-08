// ----------------------------------------------------------------------------
//  config.h — Board hardware profile for cyd-font-demo. CYD-family boards
//  only (all four have touch) -- no M5 Core branch here, unlike
//  cerberus-gate-controller's config.h.
// ----------------------------------------------------------------------------
#pragma once

#include <stdint.h>

#if defined(BOARD_CYD2USB_DIYMALLS_ILI9341)
#include "../common/boards/cyd2usb-diymalls-ili9341.h"
#elif defined(BOARD_CYD2USB_DIYMALLS_ST7789)
#include "../common/boards/cyd2usb-diymalls-st7789.h"
#elif defined(BOARD_JC2432W328C)
#include "../common/boards/jc2432w328c.h"
#else  // Default: Freenove FNK0104B Configuration
#include "../common/boards/cyd-touch-freenove.h"
#endif

// ----- Colors (RGB565 helpers) -----
#define RGB565(r, g, b) (((r)&0xF8) << 8 | ((g)&0xFC) << 3 | ((b)&0xF8) >> 3)
namespace Color {
constexpr uint16_t ACCENT = RGB565(43, 87, 220);
}  // namespace Color
#undef RGB565
