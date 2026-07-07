// ----------------------------------------------------------------------------
//  config.h — Unified hardware profile & application tunables.
// ----------------------------------------------------------------------------
#pragma once

#include <stdint.h>

// ============================================================================
//  1. HARDWARE CONFIGURATION (DISPLAY & TOUCH)
// ============================================================================

#ifdef BOARD_M5_CORE

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

#else  // Default: Freenove FNK0104B Configuration

// ----- Panel Driver & Touch Controller Selection -----
// Uncomment exactly one of each to match the hardware on your board.
#define DISPLAY_PANEL_ILI9341
// #define DISPLAY_PANEL_ST7789
// #define DISPLAY_PANEL_ILI9488
// #define DISPLAY_PANEL_GC9A01    // round 1.28" CYDs

#define DISPLAY_TOUCH_FT5X06  // FT5x06 driver handles FT6x36 family (FT6236, etc.)
// #define DISPLAY_TOUCH_XPT2046   // resistive boards
// #define DISPLAY_TOUCH_GT911

// ----- Panel Pinout (SPI) -----
constexpr int PIN_LCD_MOSI = 11;
constexpr int PIN_LCD_MISO = 13;
constexpr int PIN_LCD_SCLK = 12;
constexpr int PIN_LCD_CS = 10;
constexpr int PIN_LCD_DC = 46;
constexpr int PIN_LCD_RST = -1;  // -1 if tied to system reset
constexpr int PIN_LCD_BL = 45;   // Backlight pin
constexpr int SPI_WRITE_HZ = 40000000;
constexpr bool BL_ACTIVE_HIGH = true;

// ----- Panel Orientation & Dimensions -----
// Native panel is portrait 240x320. Display rotation swaps width/height.
constexpr int PANEL_NATIVE_WIDTH = 240;
constexpr int PANEL_NATIVE_HEIGHT = 320;
constexpr int DISPLAY_WIDTH = 320;   // Width after rotation
constexpr int DISPLAY_HEIGHT = 240;  // Height after rotation
constexpr int DISPLAY_ROTATION = 1;
constexpr bool INVERT_COLORS = true;  // Common for ILI9341 IPS modules
constexpr bool BGR_ORDER = true;      // true = BGR, false = RGB
constexpr int PANEL_X_OFFSET = 0;
constexpr int PANEL_Y_OFFSET = 0;

// ----- Touch Configuration -----
constexpr int PIN_TOUCH_SDA = 16;
constexpr int PIN_TOUCH_SCL = 15;
constexpr int PIN_TOUCH_INT = 17;
constexpr int PIN_TOUCH_RST = 18;
constexpr int TOUCH_I2C_ADDR = 0x38;  // FT6336U default
constexpr int TOUCH_I2C_HZ = 400000;

// ----- Input Capability Flags -----
#define HAS_TOUCH_INPUT 1
#define HAS_GPIO_BUTTONS 0
#define HAS_NEOKEY_BUTTONS 0

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
#define RGB565(r, g, b) (((r) &0xF8) << 8 | ((g) &0xFC) << 3 | ((b) &0xF8) >> 3)
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

// ============================================================================
//  3. ALTERNATIVE HARDWARE PROFILES
// ============================================================================
// To use one of these alternative layouts, replace the values in Section 1.
//
// ----- Sunton CYD ESP32-2432S028R (ILI9341, XPT2046 resistive touch) -----
// PIN_LCD_MOSI   = 13;
// PIN_LCD_MISO   = 12;
// PIN_LCD_SCLK   = 14;
// PIN_LCD_CS     = 15;
// PIN_LCD_DC     =  2;
// PIN_LCD_RST    = -1;
// PIN_LCD_BL     = 21;
// PIN_TOUCH_SDA  = 33;
// PIN_TOUCH_SCL  = 25;
// PIN_TOUCH_INT  = 36;
// PIN_TOUCH_RST  = 32;
//
// ----- Sunton CYD ESP32-2432S028C (capacitive variant, GT911 touch) -----
// (Same display pins as the R variant. GT911 not currently supported here.)
//
// ----- Generic ST7789 240x320 CYD -----
// Define DISPLAY_PANEL_ST7789 in Section 1.
// Note: most ST7789 boards use BGR_ORDER = false (RGB, not BGR).