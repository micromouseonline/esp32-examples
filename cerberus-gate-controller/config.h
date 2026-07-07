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

// ----- UART Header Pins (Diagnostics / Debugging) -----
constexpr int PIN_UART_TX = 43;
constexpr int PIN_UART_RX = 44;
constexpr int UART_TAP_BAUD = 115200;

// ============================================================================
//  2. APPLICATION SETTINGS & BEHAVIOR
// ============================================================================

// ----- BLE Parameters -----
constexpr const char *BIKE_NAME_PREFIX = "REEVO";
constexpr uint32_t BIKE_PAIR_PASSKEY = 111111;
constexpr int BLE_SCAN_DURATION_S = 8;
constexpr int BLE_RECONNECT_BACKOFF_MIN_MS = 1500;
constexpr int BLE_RECONNECT_BACKOFF_MAX_MS = 12000;

// ISSC Transparent UART (Reevo's BLE service)
#define REEVO_SVC_UUID "49535343-fe7d-4ae5-8fa9-9fafd205e455"
#define REEVO_NOTIFY_UUID "49535343-1e4d-4bd9-ba61-23c647249616"
#define REEVO_WRITE_UUID "49535343-8841-43f4-a8d4-ecbe34729bb3"
#define REEVO_FLOW_UUID "49535343-4c8a-39b3-2f49-511cff073b7e"

// NVS keys for persistent state across reboots
#define NVS_NAMESPACE "reevo"
#define NVS_KEY_PAIRED "paired"
#define NVS_KEY_BIKE_ADDR "bike_addr"

// ----- App Behavior Tunables -----
constexpr int TOP_SPEED_MIN = 1;
constexpr int TOP_SPEED_MAX = 20;
constexpr int TOP_SPEED_DEFAULT = 15;
constexpr float MAX_WHEEL_PULSE = 30.0f;

// Sleep / wake state machine
constexpr int SLEEP_TIMEOUT_DEFAULT_S = 60;
constexpr int SLEEP_TIMEOUT_MIN_S = 20;
constexpr int SLEEP_TIMEOUT_MAX_S = 300;

// Brightness 1..10 mapped to backlight 25..255
constexpr int BRIGHTNESS_DEFAULT = 8;
constexpr int BRIGHTNESS_MIN = 1;
constexpr int BRIGHTNESS_MAX = 10;
inline int brightness_to_pwm(int b) {
  return b * 25 + 5;
}

// ----- Colors (RGB565 helpers) -----
#define RGB565(r, g, b) (((r)&0xF8) << 8 | ((g)&0xFC) << 3 | ((b)&0xF8) >> 3)
namespace Color {
constexpr uint16_t BG = RGB565(15, 15, 20);
constexpr uint16_t PANEL = RGB565(28, 28, 36);
constexpr uint16_t FG = RGB565(240, 240, 240);
constexpr uint16_t DIM = RGB565(110, 110, 120);
constexpr uint16_t MUTED = RGB565(60, 60, 70);
constexpr uint16_t ACCENT = RGB565(60, 200, 255);
constexpr uint16_t WARN = RGB565(255, 80, 80);
constexpr uint16_t OK = RGB565(60, 220, 100);
constexpr uint16_t SIGNAL = RGB565(255, 165, 0);
constexpr uint16_t REEVO_BLUE = RGB565(43, 87, 220);   // matches the logo
constexpr uint16_t SEG_YELLOW = RGB565(255, 224, 30);  // 7-seg / calculator yellow
}  // namespace Color
#undef RGB565